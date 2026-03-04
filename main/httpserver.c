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
#include "esp_http_server.h"
#include "esp_log.h"
#include "gpiodefs.h"
#include "httpserver.h"

static const char *TAG = "httpserver";

#define SDCARD_BASE   "/sdcard"
#define CONFIG_FILE   SDCARD_BASE "/config.txt"
#define RECV_BUF_SIZE 512

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void add_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin",  "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
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
// httpserver_start
// ---------------------------------------------------------------------------

esp_err_t httpserver_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn  = httpd_uri_match_wildcard;
    config.stack_size    = 6144;
    config.max_uri_handlers = 8;
    config.server_port   = 8080;  // port 80 is taken by wifi_manager captive portal
    config.ctrl_port     = 32769; // default 32768 is taken by wifi_manager's httpd instance

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

    httpd_register_uri_handler(server, &status_get);
    httpd_register_uri_handler(server, &config_get);
    httpd_register_uri_handler(server, &config_post);
    httpd_register_uri_handler(server, &files_put);
    httpd_register_uri_handler(server, &options_any);

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}
