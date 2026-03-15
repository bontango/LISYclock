// httpserver.c
// Minimal HTTP REST server for LISYclock
//
// Endpoints:
//   GET  /status          -> {"status":"ok","version":"..."}
//   GET  /config          -> /sdcard/config.txt as text/plain (404 if missing)
//   POST /config          -> write request body to /sdcard/config.txt
//   PUT  /files/<name>    -> write request body to /sdcard/<name>
//   OPTIONS /*            -> CORS preflight, 204 No Content
//
// All responses include CORS headers for file:// browser access.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "gpiodefs.h"
#include "httpserver.h"

extern void lisyclock_set_rtc_time(struct tm *t);

static const char *TAG = "httpserver";

#define SDCARD_BASE   "/sdcard"
#define CONFIG_FILE   SDCARD_BASE "/config.txt"
#define RECV_BUF_SIZE 512

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void add_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin",          "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods",         "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers",         "Content-Type");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Private-Network", "true");
}

// Decode a percent-encoded URL segment into dst (null-terminated).
// Rejects embedded '/' and '..' to prevent path traversal.
static void url_decode(const char *src, char *dst, size_t dst_len)
{
    size_t i = 0, j = 0;
    while (src[i] && j < dst_len - 1) {
        if (src[i] == '%' && src[i+1] && src[i+2]) {
            char hex[3] = { src[i+1], src[i+2], '\0' };
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}

// Write the full request body to an already-open FILE*.
// Returns true on success, false on recv or write error.
static bool recv_to_file(httpd_req_t *req, FILE *f)
{
    char buf[RECV_BUF_SIZE];
    int remaining = (int)req->content_len;

    while (remaining > 0) {
        int to_recv = (remaining < (int)sizeof(buf)) ? remaining : (int)sizeof(buf);
        int n = httpd_req_recv(req, buf, to_recv);
        if (n <= 0) {
            ESP_LOGE(TAG, "recv error: %d", n);
            return false;
        }
        if ((int)fwrite(buf, 1, n, f) != n) {
            ESP_LOGE(TAG, "fwrite error, errno=%d", errno);
            return false;
        }
        remaining -= n;
    }
    return true;
}

// ---------------------------------------------------------------------------
// OPTIONS /* — CORS preflight
// ---------------------------------------------------------------------------

static esp_err_t options_handler(httpd_req_t *req)
{
    add_cors_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// GET /status
// ---------------------------------------------------------------------------

static esp_err_t status_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "status request received");
    add_cors_headers(req);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\",\"version\":\"" LISYCLOCK_VERSION "\",\"api_version\":" HTTP_API_VERSION_STR "}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// GET /config  — stream /sdcard/config.txt to client
// ---------------------------------------------------------------------------

static esp_err_t config_get_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "config.txt not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "text/plain");
    char buf[RECV_BUF_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, (ssize_t)n) != ESP_OK) {
            fclose(f);
            return ESP_FAIL;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0); // end chunked transfer
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// POST /config  — receive body and write to /sdcard/config.txt
// ---------------------------------------------------------------------------

static esp_err_t config_post_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open config.txt for writing, errno=%d", errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot write config.txt");
        return ESP_OK;
    }

    bool ok = recv_to_file(req, f);
    fclose(f);

    if (!ok) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// PUT /files/<filename>  — receive body and write to /sdcard/<filename>
// ---------------------------------------------------------------------------

static esp_err_t files_put_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    // Extract the filename segment after "/files/"
    const char *prefix = "/files/";
    const char *uri = req->uri;
    if (strncmp(uri, prefix, strlen(prefix)) != 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
        return ESP_OK;
    }
    const char *raw_name = uri + strlen(prefix);

    char name[64];
    url_decode(raw_name, name, sizeof(name));

    // Safety checks
    if (name[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty filename");
        return ESP_OK;
    }
    if (strchr(name, '/') || strstr(name, "..")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename");
        return ESP_OK;
    }

    char path[96];
    snprintf(path, sizeof(path), "%s/%s", SDCARD_BASE, name);

    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot create %s, errno=%d", path, errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot create file");
        return ESP_OK;
    }

    bool ok = recv_to_file(req, f);
    fclose(f);

    if (!ok) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// POST /reboot  — reboot the clock (body must contain {"confirm":"reboot"})
// ---------------------------------------------------------------------------

static esp_err_t reboot_post_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    char buf[64];
    int to_recv = (req->content_len > 0 && req->content_len < (int)sizeof(buf) - 1)
                  ? (int)req->content_len : (int)(sizeof(buf) - 1);
    int len = httpd_req_recv(req, buf, to_recv);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_OK;
    }
    buf[len] = '\0';

    if (!strstr(buf, "\"confirm\"") || !strstr(buf, "\"reboot\"")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Confirm required: {\"confirm\":\"reboot\"}");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    esp_restart();
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// POST /update  — write body as /sdcard/update.bin, then reboot
// ---------------------------------------------------------------------------

static esp_err_t update_post_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    FILE *f = fopen(SDCARD_BASE "/update.bin", "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot create update.bin, errno=%d", errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot create update.bin");
        return ESP_OK;
    }

    bool ok = recv_to_file(req, f);
    fclose(f);

    if (!ok) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    esp_restart();
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// GET /files  — list all files on SD card as JSON array
// ---------------------------------------------------------------------------

static esp_err_t files_list_get_handler(httpd_req_t *req)
{
    add_cors_headers(req);
    httpd_resp_set_type(req, "application/json");

    DIR *dir = opendir(SDCARD_BASE);
    if (!dir) {
        httpd_resp_sendstr(req, "{\"files\":[]}");
        return ESP_OK;
    }

    httpd_resp_send_chunk(req, "{\"files\":[", -1);

    struct dirent *entry;
    bool first = true;
    char chunk[300];

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;

        char path[264];
        snprintf(path, sizeof(path), "%s/%s", SDCARD_BASE, entry->d_name);
        struct stat st;
        long size = 0;
        long mtime = 0;
        if (stat(path, &st) == 0) { size = (long)st.st_size; mtime = (long)st.st_mtime; }

        snprintf(chunk, sizeof(chunk), "%s{\"name\":\"%s\",\"size\":%ld,\"mtime\":%ld}",
                 first ? "" : ",", entry->d_name, size, mtime);
        httpd_resp_send_chunk(req, chunk, -1);
        first = false;
    }
    closedir(dir);

    httpd_resp_send_chunk(req, "]}", -1);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// GET /files/<filename>  — download a file from SD card
// ---------------------------------------------------------------------------

static esp_err_t files_get_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    const char *prefix = "/files/";
    const char *uri = req->uri;
    if (strncmp(uri, prefix, strlen(prefix)) != 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
        return ESP_OK;
    }
    const char *raw_name = uri + strlen(prefix);

    char name[64];
    url_decode(raw_name, name, sizeof(name));

    if (name[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty filename");
        return ESP_OK;
    }
    if (strchr(name, '/') || strstr(name, "..")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename");
        return ESP_OK;
    }

    char path[96];
    snprintf(path, sizeof(path), "%s/%s", SDCARD_BASE, name);

    FILE *f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    char buf[RECV_BUF_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, (ssize_t)n) != ESP_OK) {
            fclose(f);
            return ESP_FAIL;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// POST /rename  — rename a file on SD card
//   Body: {"old_name":"...","new_name":"..."}
// ---------------------------------------------------------------------------

// Extract a JSON string value for key. Returns true on success.
static bool extract_json_str(const char *body, const char *key, char *out, size_t out_len)
{
    char search[72];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(body, search);
    if (!p) return false;
    p += strlen(search);
    size_t i = 0;
    while (*p && *p != '"' && i < out_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return (*p == '"');
}

static esp_err_t rename_post_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    char buf[256];
    int to_recv = (req->content_len > 0 && req->content_len < (int)sizeof(buf) - 1)
                  ? (int)req->content_len : (int)(sizeof(buf) - 1);
    int len = httpd_req_recv(req, buf, to_recv);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_OK;
    }
    buf[len] = '\0';

    char old_name[64] = {0};
    char new_name[64] = {0};
    if (!extract_json_str(buf, "old_name", old_name, sizeof(old_name)) ||
        !extract_json_str(buf, "new_name", new_name, sizeof(new_name))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing old_name or new_name");
        return ESP_OK;
    }

    if (old_name[0] == '\0' || new_name[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty filename");
        return ESP_OK;
    }
    if (strchr(old_name, '/') || strstr(old_name, "..") ||
        strchr(new_name, '/') || strstr(new_name, "..")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename");
        return ESP_OK;
    }

    char old_path[96], new_path[96];
    snprintf(old_path, sizeof(old_path), "%s/%s", SDCARD_BASE, old_name);
    snprintf(new_path, sizeof(new_path), "%s/%s", SDCARD_BASE, new_name);

    if (rename(old_path, new_path) != 0) {
        ESP_LOGE(TAG, "rename %s -> %s failed, errno=%d", old_path, new_path, errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Rename failed");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// POST /time  — set system + RTC time from JSON {"unix_timestamp": 1234567890}
// ---------------------------------------------------------------------------

static esp_err_t time_post_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    char buf[64];
    int to_recv = (req->content_len > 0 && req->content_len < (int)sizeof(buf) - 1)
                  ? (int)req->content_len : (int)(sizeof(buf) - 1);
    int len = httpd_req_recv(req, buf, to_recv);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_OK;
    }
    buf[len] = '\0';

    // Parse unix_timestamp from JSON
    const char *key = "\"unix_timestamp\"";
    const char *p = strstr(buf, key);
    if (!p) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing unix_timestamp");
        return ESP_OK;
    }
    p += strlen(key);
    while (*p == ' ' || *p == ':') p++;
    time_t ts = (time_t)atoll(p);
    if (ts <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid unix_timestamp");
        return ESP_OK;
    }

    struct timeval tv = { .tv_sec = ts, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    struct tm tm_info;
    localtime_r(&ts, &tm_info);
    tm_info.tm_year += 1900;
    lisyclock_set_rtc_time(&tm_info);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// DELETE /files/<filename>  — delete a file from SD card
// ---------------------------------------------------------------------------

static esp_err_t files_delete_handler(httpd_req_t *req)
{
    add_cors_headers(req);

    const char *prefix = "/files/";
    const char *uri = req->uri;
    if (strncmp(uri, prefix, strlen(prefix)) != 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
        return ESP_OK;
    }
    const char *raw_name = uri + strlen(prefix);

    char name[64];
    url_decode(raw_name, name, sizeof(name));

    if (name[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty filename");
        return ESP_OK;
    }
    if (strchr(name, '/') || strstr(name, "..")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename");
        return ESP_OK;
    }

    char path[96];
    snprintf(path, sizeof(path), "%s/%s", SDCARD_BASE, name);

    if (remove(path) != 0) {
        if (errno == ENOENT) {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        } else {
            ESP_LOGE(TAG, "delete %s failed, errno=%d", path, errno);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Delete failed");
        }
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// httpserver_start
// ---------------------------------------------------------------------------

esp_err_t httpserver_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn  = httpd_uri_match_wildcard;
    config.stack_size    = 6144;
    config.max_uri_handlers = 16;
    config.server_port   = 8080;  // kept at 8080 for API compatibility (port 80 now free)
    config.ctrl_port     = 32769; // kept for compatibility

    httpd_handle_t server = NULL;
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    static const httpd_uri_t status_get = {
        .uri     = "/status",
        .method  = HTTP_GET,
        .handler = status_get_handler,
    };
    static const httpd_uri_t config_get = {
        .uri     = "/config",
        .method  = HTTP_GET,
        .handler = config_get_handler,
    };
    static const httpd_uri_t config_post = {
        .uri     = "/config",
        .method  = HTTP_POST,
        .handler = config_post_handler,
    };
    static const httpd_uri_t files_put = {
        .uri     = "/files/*",
        .method  = HTTP_PUT,
        .handler = files_put_handler,
    };
    static const httpd_uri_t options_any = {
        .uri     = "/*",
        .method  = HTTP_OPTIONS,
        .handler = options_handler,
    };
    static const httpd_uri_t reboot_post = {
        .uri     = "/reboot",
        .method  = HTTP_POST,
        .handler = reboot_post_handler,
    };
    static const httpd_uri_t update_post = {
        .uri     = "/update",
        .method  = HTTP_POST,
        .handler = update_post_handler,
    };
    static const httpd_uri_t files_list = {
        .uri     = "/files",
        .method  = HTTP_GET,
        .handler = files_list_get_handler,
    };
    static const httpd_uri_t files_get = {
        .uri     = "/files/*",
        .method  = HTTP_GET,
        .handler = files_get_handler,
    };
    static const httpd_uri_t rename_post = {
        .uri     = "/rename",
        .method  = HTTP_POST,
        .handler = rename_post_handler,
    };
    static const httpd_uri_t time_post = {
        .uri     = "/time",
        .method  = HTTP_POST,
        .handler = time_post_handler,
    };
    static const httpd_uri_t files_delete = {
        .uri     = "/files/*",
        .method  = HTTP_DELETE,
        .handler = files_delete_handler,
    };

    httpd_register_uri_handler(server, &status_get);
    httpd_register_uri_handler(server, &config_get);
    httpd_register_uri_handler(server, &config_post);
    httpd_register_uri_handler(server, &files_list);
    httpd_register_uri_handler(server, &files_put);
    httpd_register_uri_handler(server, &files_get);
    httpd_register_uri_handler(server, &files_delete);
    httpd_register_uri_handler(server, &reboot_post);
    httpd_register_uri_handler(server, &update_post);
    httpd_register_uri_handler(server, &rename_post);
    httpd_register_uri_handler(server, &time_post);
    httpd_register_uri_handler(server, &options_any);

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}
