#pragma once
#include "esp_err.h"

// Start the HTTP REST server on port 80.
// Call once after WiFi is connected.
esp_err_t httpserver_start(void);
