// set time
//
// handling of manual time set
//part of LISYclock

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "esp_log.h"
#include "typedefs.h"

//static const char *TAG = "set_time";

extern int set_time_phase;
extern char set_time_display1[7];
extern char set_time_display2[7];
extern char set_time_display3[7];
extern char set_time_display4[7];

void cb_set_time(void *pvParameter) {

    while(1) {set_time_phase = 1; };

//    vTaskDelete( NULL );
}