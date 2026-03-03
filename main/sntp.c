// sntp
//
// sync system time to ntp
//from ESP_IDF_SNTP example
//part of LISYclock

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <esp_netif.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

static const char *TAG = "SNTP";

#define SNTP_TIME_SERVER "pool.ntp.org"

extern int wifi_is_connected;
extern uint8_t ntp_time_received;

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Notification of a time synchronization event");
    ntp_time_received = 1;
}

void obtain_time(int retries) {

    /*
     * This is the basic default config with one server and starting the service
     */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(SNTP_TIME_SERVER);

    config.sync_cb = time_sync_notification_cb; 

    esp_netif_sntp_init(&config);

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retries) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retries);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    esp_netif_sntp_deinit();

}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter){
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
    wifi_is_connected = 1; //signal to main 
	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    //obtain_time();
}
