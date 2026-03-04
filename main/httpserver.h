#pragma once
#include "esp_err.h"

// HTTP API version — increment only on breaking changes.
// Must match the version documented in ESP32_source/API.md.
#define HTTP_API_VERSION     2
#define HTTP_API_VERSION_STR "2"

// Start the HTTP REST server on port 8080.
// Call once after WiFi is connected.
esp_err_t httpserver_start(void);
