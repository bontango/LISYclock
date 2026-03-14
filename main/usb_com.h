#pragma once
#include <stdint.h>
void usb_com_init(void);

extern uint8_t wifi_cfg_enabled;
extern char wifi_cfg_ssid[65];
extern char wifi_cfg_pwd[65];

// Connect to WiFi with given credentials, returns IP string or NULL
// Caller must free() the returned string
char *wifi_connect_with_creds(const char *ssid, const char *pwd);
