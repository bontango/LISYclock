#include "usb_com.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

extern void lisyclock_set_rtc_time(struct tm *t);

#define USB_COM_UART         UART_NUM_0
#define USB_COM_BUF_SIZE     256
#define USB_COM_HANDSHAKE    0x55
#define HANDSHAKE_TIMEOUT_MS 3000

static const char *TAG = "usb_com";

// Pending SSID/PWD set via USB commands (before TEST)
static char pending_ssid[65] = {0};
static char pending_pwd[65] = {0};

// Event group for WiFi connection result
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static char s_test_ip[16] = {0};

static void usb_respond(const char *msg)
{
    uart_write_bytes(USB_COM_UART, msg, strlen(msg));
    uart_write_bytes(USB_COM_UART, "\r\n", 2);
}

// WiFi event handler for TEST connection
static void wifi_test_event_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_ip4addr_ntoa(&event->ip_info.ip, s_test_ip, sizeof(s_test_ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Try to connect to WiFi with given credentials.
// Initialises the WiFi stack on first call if app_main skipped it (no SSID at boot).
// Returns IP string (static buffer) or NULL on failure.
char *wifi_connect_with_creds(const char *ssid, const char *pwd)
{
    if (!ssid || strlen(ssid) == 0) return NULL;

    // If WiFi stack was never initialised (no SSID configured at boot), set it up now.
    wifi_mode_t mode;
    bool needs_start = false;
    if (esp_wifi_get_mode(&mode) == ESP_ERR_WIFI_NOT_INIT) {
        esp_netif_init();
        esp_err_t el_err = esp_event_loop_create_default();
        if (el_err != ESP_OK && el_err != ESP_ERR_INVALID_STATE) return NULL;
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
        if (esp_wifi_init(&wcfg) != ESP_OK) return NULL;
        esp_wifi_set_mode(WIFI_MODE_STA);
        needs_start = true;
    }

    s_wifi_event_group = xEventGroupCreate();
    memset(s_test_ip, 0, sizeof(s_test_ip));

    if (!needs_start) {
        // Disconnect current connection, keep WiFi stack intact
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    esp_event_handler_instance_t instance_wifi;
    esp_event_handler_instance_t instance_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_test_event_handler, NULL, &instance_wifi);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_test_event_handler, NULL, &instance_ip);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (needs_start) {
        esp_wifi_start();
    }
    esp_wifi_connect();

    // Wait up to 10 seconds for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE, pdFALSE,
                                            pdMS_TO_TICKS(10000));

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_wifi);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_ip);
    vEventGroupDelete(s_wifi_event_group);

    if (bits & WIFI_CONNECTED_BIT) {
        return s_test_ip;
    }

    esp_wifi_disconnect();
    return NULL;
}

// Update WIFI_* lines in /sdcard/config.txt
static void update_wifi_config_on_sd(const char *ssid, const char *pwd, int enabled)
{
    FILE *fp = fopen("/sdcard/config.txt", "r");
    if (!fp) {
        ESP_LOGW(TAG, "Cannot open config.txt for reading");
        return;
    }

    // Read entire file
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = malloc(fsize + 1);
    if (!content) { fclose(fp); return; }
    fread(content, 1, fsize, fp);
    content[fsize] = '\0';
    fclose(fp);

    // Write back with updated WIFI_* lines
    fp = fopen("/sdcard/config.txt", "w");
    if (!fp) { free(content); return; }

    char *line = content;
    int wifi_written = 0;
    while (line && *line) {
        char *next = strchr(line, '\n');
        int len = next ? (int)(next - line) : (int)strlen(line);

        // Check if this line is a WIFI_* key or its comment
        char trimmed[128] = {0};
        int ti = 0;
        for (int i = 0; i < len && i < 127; i++) {
            trimmed[ti++] = line[i];
        }
        trimmed[ti] = '\0';

        // Skip existing WIFI_* lines (both commented and uncommented)
        char *check = trimmed;
        while (*check == ' ' || *check == '#') check++;
        if (strncmp(check, "WIFI_ENABLE", 11) == 0 ||
            strncmp(check, "WIFI_SSID", 9) == 0 ||
            strncmp(check, "WIFI_PWD", 8) == 0) {
            // Skip this line, we'll write new ones
            if (!wifi_written) {
                fprintf(fp, "WIFI_ENABLE=%s\n", enabled ? "yes" : "no");
                fprintf(fp, "WIFI_SSID=\"%s\"\n", ssid ? ssid : "");
                fprintf(fp, "WIFI_PWD=\"%s\"\n", pwd ? pwd : "");
                wifi_written = 1;
            }
        } else {
            fwrite(line, 1, len, fp);
            if (next) fputc('\n', fp);
        }

        line = next ? next + 1 : NULL;
    }

    // If no WIFI_* lines existed, append them
    if (!wifi_written) {
        fprintf(fp, "WIFI_ENABLE=%s\n", enabled ? "yes" : "no");
        fprintf(fp, "WIFI_SSID=\"%s\"\n", ssid ? ssid : "");
        fprintf(fp, "WIFI_PWD=\"%s\"\n", pwd ? pwd : "");
    }

    fclose(fp);
    free(content);
}

static void handle_wifi_command(const char *cmd)
{
    if (strncmp(cmd, "SET_SSID=", 9) == 0) {
        strncpy(pending_ssid, cmd + 9, sizeof(pending_ssid) - 1);
        pending_ssid[sizeof(pending_ssid) - 1] = '\0';
        usb_respond("OK:SSID_SET");
    }
    else if (strncmp(cmd, "SET_PWD=", 8) == 0) {
        strncpy(pending_pwd, cmd + 8, sizeof(pending_pwd) - 1);
        pending_pwd[sizeof(pending_pwd) - 1] = '\0';
        usb_respond("OK:PWD_SET");
    }
    else if (strcmp(cmd, "ENABLE") == 0) {
        wifi_cfg_enabled = 1;
        update_wifi_config_on_sd(wifi_cfg_ssid, wifi_cfg_pwd, 1);
        usb_respond("OK:WIFI_ENABLED");
    }
    else if (strcmp(cmd, "DISABLE") == 0) {
        wifi_cfg_enabled = 0;
        update_wifi_config_on_sd(wifi_cfg_ssid, wifi_cfg_pwd, 0);
        usb_respond("OK:WIFI_DISABLED");
    }
    else if (strcmp(cmd, "TEST") == 0) {
        char *ip = wifi_connect_with_creds(pending_ssid, pending_pwd);
        if (ip) {
            // Success: save credentials to globals and SD card
            strncpy(wifi_cfg_ssid, pending_ssid, sizeof(wifi_cfg_ssid) - 1);
            strncpy(wifi_cfg_pwd, pending_pwd, sizeof(wifi_cfg_pwd) - 1);
            wifi_cfg_enabled = 1;
            update_wifi_config_on_sd(wifi_cfg_ssid, wifi_cfg_pwd, 1);

            char resp[64];
            snprintf(resp, sizeof(resp), "OK:WIFI_CONNECTED=%s", ip);
            usb_respond(resp);
        } else {
            // Failure: clear credentials on SD card
            update_wifi_config_on_sd("", "", 0);
            memset(wifi_cfg_ssid, 0, sizeof(wifi_cfg_ssid));
            memset(wifi_cfg_pwd, 0, sizeof(wifi_cfg_pwd));
            wifi_cfg_enabled = 0;
            usb_respond("ERR:WIFI_CONNECT_FAILED");
        }
    }
    else if (strcmp(cmd, "STATUS") == 0) {
        // Check current WiFi status
        wifi_ap_record_t ap_info;
        esp_netif_ip_info_t ip_info;
        char resp[128];
        int connected = 0;
        char ip_str[16] = "none";

        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
                connected = 1;
                esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
            }
        }

        snprintf(resp, sizeof(resp), "OK:WIFI_STATUS=%s,%s,%s",
                 connected ? "yes" : "no",
                 wifi_cfg_ssid,
                 ip_str);
        usb_respond(resp);
    }
    else {
        usb_respond("ERR:UNKNOWN_CMD");
    }
}

static void handle_time_command(const char *cmd)
{
    if (strncmp(cmd, "SET=", 4) == 0) {
        time_t t = (time_t)atoll(cmd + 4);
        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        struct tm tm_info;
        localtime_r(&t, &tm_info);
        tm_info.tm_year += 1900;
        lisyclock_set_rtc_time(&tm_info);
        usb_respond("OK:TIME_SET");
    } else {
        usb_respond("ERR:UNKNOWN_CMD");
    }
}

static void usb_com_task(void *arg)
{
    uint8_t byte;
    uint8_t line[USB_COM_BUF_SIZE];

    // NOTE: log suppression is set in app_main() before initArduino()

    // Wait for handshake byte 0x55 (max 3 s)
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) * portTICK_PERIOD_MS < HANDSHAKE_TIMEOUT_MS) {
        if (uart_read_bytes(USB_COM_UART, &byte, 1, pdMS_TO_TICKS(50)) == 1) {
            if (byte == USB_COM_HANDSHAKE) {
                const char *ready = "OK:READY\r\n";
                uart_write_bytes(USB_COM_UART, ready, strlen(ready));
                break;
            }
        }
    }

    // Re-enable logs (handshake done or timeout — normal operation resumes)
    esp_log_level_set("*", ESP_LOG_INFO);

    // Command loop: read line, dispatch command
    int idx = 0;
    while (1) {
        if (uart_read_bytes(USB_COM_UART, &byte, 1, pdMS_TO_TICKS(100)) == 1) {
            if (byte == '\n' || byte == '\r') {
                if (idx > 0) {
                    line[idx] = '\0';
                    // Dispatch: check for WIFI: prefix
                    if (strncmp((char *)line, "WIFI:", 5) == 0) {
                        handle_wifi_command((char *)line + 5);
                    } else if (strncmp((char *)line, "TIME:", 5) == 0) {
                        handle_time_command((char *)line + 5);
                    } else {
                        usb_respond("ERR:UNKNOWN_CMD");
                    }
                    idx = 0;
                }
            } else if (idx < USB_COM_BUF_SIZE - 1) {
                line[idx++] = byte;
            }
        }
    }
}

void usb_com_init(void)
{
    // UART0 is used by ESP-IDF for logging; install driver only if not yet installed.
    // Note: CONFIG_ESP_CONSOLE_UART_NONE or CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG may be
    // required in sdkconfig to avoid conflicts with the IDF console driver.
    uart_driver_install(USB_COM_UART, USB_COM_BUF_SIZE * 2, 0, 0, NULL, 0);
    xTaskCreate(usb_com_task, "usb_com", 8192, NULL, 5, NULL);
}
