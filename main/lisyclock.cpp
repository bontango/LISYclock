// LISYclock main
// 


#include "Arduino.h"
#include "TM1637TinyDisplay6.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "led_strip.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "wifi_manager.h"
#include "typedefs.h"
#include "ds3231.h"
#include "gpiodefs.h"
#include "httpserver.h"
}


// Version ( HW v1.x & 2.x ) -> gpiodefs.h

// Firmware file name
#define FIRMWARE_NAME "update.bin"
// config file name
#define CONFIG_NAME "config.txt"

static const char *TAG = "LISYclock";

//----------------------------------------------------------------------------------------
//
// Main
//
//

#define CORE_0 0
#define CORE_1 1

extern void event_task(void *pvParameter);
SemaphoreHandle_t event_task_Semaphore;
extern esp_err_t audio_init(void);
extern void audio_play_mp3(const char *filename);
extern void audio_play_tts(char* text);

led_strip_handle_t led_strip;
extern "C" void init_leds(uint8_t activate);
extern "C" void attract_leds(void *pvParameter);
extern "C" led_strip_handle_t configure_led(void);

sdmmc_card_t *card;
extern "C" esp_err_t MountSDCard(void);

extern "C" esp_err_t CheckFWUpdate(char *fname);
extern "C" void doFWUpdate(char *fname);

extern "C" void cb_connection_ok(void *pvParameter);
int wifi_is_connected = 0;

extern "C" esp_err_t button_init(void);
uint8_t play_it = 0;
uint8_t show_ip = 0;
uint8_t set_time = 0;

//dip section
uint8_t wifi_enabled;
uint8_t attract_enabled;
uint8_t sound_disabled;

//config section, default enable, on off via events
bool display_enabled = true;
bool attract_leds_enabled = true;

#define MAX_LEDS 32
extern "C" void InitConfig(void);
extern "C" int ReadConfig(char *fname);
Led_gi_t cfg_led_gi[MAX_LEDS +1]; // GI LEDs
Led_at_t cfg_led_at1[MAX_LEDS +1]; //attract mode LEDs groups
Led_at_t cfg_led_at2[MAX_LEDS +1];
Led_at_t cfg_led_at3[MAX_LEDS +1];
Led_at_t cfg_led_at4[MAX_LEDS +1];
Led_at_t cfg_led_at5[MAX_LEDS +1];
int cfg_at1_blink_rate = 500; //attract mode blinkrate group1
int cfg_at2_blink_rate = 500; //attract mode blinkrate group2
int cfg_at3_blink_rate = 500; //attract mode blinkrate group3
int cfg_at4_blink_rate = 500; //attract mode blinkrate group4
int cfg_at5_blink_rate = 500; //attract mode blinkrate group5
uint8_t cfg_at1_rand=0; //default no random LED activation in attract
uint8_t cfg_at2_rand=0; //default no random LED activation in attract
uint8_t cfg_at3_rand=0; //default no random LED activation in attract
uint8_t cfg_at4_rand=0; //default no random LED activation in attract
uint8_t cfg_at5_rand=0; //default no random LED activation in attract
uint8_t cfg_disp_bright = 1; //display brightness ( 0..7 )

char weekday[7][7];
char timezone_str[31];
uint8_t ntp_time_received = 0;
struct tm timeinfo;
extern "C" void obtain_time(int retries);

extern "C" void ftp_task (void *pvParameters);
uint8_t ftp_server_enable;
char ftp_user[41];
char ftp_pass[41];

extern "C" void cb_set_time(void *pvParameter);
int set_time_phase = 0;
char set_time_display1[7];
char set_time_display2[7];
char set_time_display3[7];
char set_time_display4[7];
  
extern "C" void app_main()
{

  initArduino();
  
  int i;
  char fpath0[30];
  char str_display1[32];
  char str_display2[32];
  char str_display3[32];
  char str_display4[32];
  char my_ip_addr[4][7];
  time_t now;
  struct tm rtcinfo;
  int old_minute = 100;
  float temp;
  char delimeter = '-';
  char *str_ip;
  uint8_t sd_card_mounted = 0;
	i2c_dev_t dev;
  uint8_t ds3231_present = 0;
  int cfg_err_line;
  
  //init system
  InitConfig();
  button_init();
  audio_init();
  
//init displays
TM1637TinyDisplay6 display1(CLK1, DIO1); // 6-Digit Display Class
TM1637TinyDisplay6 display2(CLK2, DIO2); // 6-Digit Display Class
TM1637TinyDisplay6 display3(CLK3, DIO3); // 6-Digit Display Class
TM1637TinyDisplay6 display4(CLK4, DIO4); // 6-Digit Display Class

  display1.begin();
  display2.begin();
  display3.begin();
  display4.begin();  

  display1.setBrightness(cfg_disp_bright);
  display2.setBrightness(cfg_disp_bright);
  display3.setBrightness(cfg_disp_bright);
  display4.setBrightness(cfg_disp_bright);

  display1.showString(" LISY ");
  display2.showString(" CLOCK");
  display3.showString(LISYCLOCK_VERSION);
  display4.showString("boot");
  delay(3000);

//SD card, check for updates first
if (MountSDCard() != ESP_OK) {
  ESP_LOGI(TAG, "no SD card found");
  display2.showString(" no SD");
  }  
else {
  ESP_LOGI(TAG, "SD card mounted");
  sd_card_mounted = 1;
  display2.showString(" SD OK");
  delay(3000);
  //check for possible upate
  sprintf(fpath0, "/sdcard/%s",FIRMWARE_NAME);
  if (CheckFWUpdate(fpath0) != ESP_OK) {
    ESP_LOGI(TAG, "no update found");
  }
  else {
    ESP_LOGI(TAG, "update found and initiated");
    display4.showString("update");
    delay(3000);
    doFWUpdate(fpath0);
  }
  //read config
  sprintf(fpath0, "/sdcard/%s",CONFIG_NAME);
  if ( ( cfg_err_line = ReadConfig(fpath0)) != 0) {
    for ( i=1; i<=8; i++) {
    display3.showString("      ");
    display4.showString("      ");
    delay(1000);
    display3.showString("CFGERR");
    sprintf(str_display4,"L %04d",cfg_err_line);
    display4.showString(str_display4);
    delay(1000);
    }
  }
  else { 
    display3.showString("CFG OK");
    delay(3000);
  }

  //create semaphore for event task
  event_task_Semaphore = xSemaphoreCreateBinary();
  //start event task
  BaseType_t xReturned = xTaskCreate(&event_task, "event_task", 8096, NULL, (tskIDLE_PRIORITY + 3), NULL);  
  if( xReturned != pdPASS ) { ESP_LOGE(TAG, "event task create failed! (%d)",xReturned); }
 }

 // Set timezone to Berlin Standard Time
 setenv("TZ", timezone_str, 1);
 tzset();

  //configure LEDs
  led_strip = configure_led();
  //set GI & start loop to refresh LED strip
  init_leds(attract_enabled);
  // attract mode let some LEDs blink
  if ( attract_enabled) {
  xTaskCreatePinnedToCore(&attract_leds, "attract_leds", 4096, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  
  }
  
  //check if a ds3231 modul is inserted
  // Initialize RTC
	if (ds3231_init_desc(&dev, I2C_NUM_0, gpio_num_t(I2C_SDA), gpio_num_t(I2C_SCL)) != ESP_OK) {
		ESP_LOGW(TAG, "No RTC ds3231 device found");
    ds3231_present = 0;
	}
  else {
		ESP_LOGI(TAG, "RTC ds3231 device found and initialized");
    ds3231_present = 1;
  }
  
  if (wifi_enabled) { //if not disabled via DIP switch
      /* start the wifi manager */
      wifi_manager_start();
      // register a callback, called when connection to WiFi is established 
      // we do start time setting via ntp here
      wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
      //start AP to reset credentials ??
      //	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

      display1.showString(" WAIT ");
      display2.showString(" FOR  ");
      display3.showString(" WIFI ");
      display4.showString(" CONN ");
      
      while ( wifi_is_connected == 0) { };
      
      str_ip = wifi_manager_get_sta_ip_string();

      //we have a network connection, start ftp server if requested and SD card mounted
      if ((ftp_server_enable != 0) & (sd_card_mounted != 0)) {
        xTaskCreatePinnedToCore(&ftp_task, "FTP", 4096, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);
      }

      //start HTTP REST server for browser-based configuration
      httpserver_start();

      //split the ip to four displays
      sprintf(my_ip_addr[0], "%-6s", strtok(str_ip, "."));
      sprintf(my_ip_addr[1], "%-6s", strtok(NULL, "."));
      sprintf(my_ip_addr[2], "%-6s", strtok(NULL, "."));
      sprintf(my_ip_addr[3], "%-6s", strtok(NULL, "."));
      display1.showString(my_ip_addr[0]);
      display2.showString(my_ip_addr[1]);
      display3.showString(my_ip_addr[2]);
      display4.showString(my_ip_addr[3]);
      delay(5000);

      //wait for ntp time sync
      obtain_time(15);
      //while (ntp_time_received == 0) { vTaskDelay(1); }

      //write time to RTC modul if present
      time(&now);
      localtime_r(&now, &rtcinfo);
      rtcinfo.tm_year = rtcinfo.tm_year + 1900;
      if (ds3231_present) {
          if (ds3231_get_temp_float(&dev, &temp) != ESP_OK) {
            ESP_LOGE(pcTaskGetName(0), "Could not get temperature.");
          }
          if (ds3231_set_time(&dev, &rtcinfo) != ESP_OK) {
            ESP_LOGE(TAG, "Could not set ntp time to RTC.");
          }
          else {
          ESP_LOGI(TAG, "%04d-%02d-%02d %02d:%02d:%02d, %.2f deg Cel", 
            rtcinfo.tm_year, rtcinfo.tm_mon + 1,
            rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, temp);            
          }
      }
}
else { //no Wifi lets use real time clock
      

      display1.showString("  NO  ");
      display2.showString(" WIFI ");
      display3.showString("WE TRY");
      display4.showString("  RTC ");
      delay(5000);
      
      if (ds3231_present == 0) {
      display1.showString("  NO  ");
      display2.showString(" RTC  ");
      
      display3.showString("FOUND ");
      display4.showString("ERROR ");

      ESP_LOGE(TAG, "No ds3231 present but wifi disabled");
      while (1) { vTaskDelay(1); }
      }

  if (ds3231_get_time(&dev, &rtcinfo) != ESP_OK) {
      ESP_LOGE(TAG, "Could not get time.");
      while (1) { vTaskDelay(1); }
    }

  //fresh booted ds3231 ? -> set year to 2025
  if ( rtcinfo.tm_year == 2000) {
    rtcinfo.tm_year = 2025;
  if (ds3231_set_time(&dev, &rtcinfo) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not set time.");
		while (1) { vTaskDelay(1); }
	 }
  }
} //no Wifi, use RTC

//about to start loop, say that we are ready and play welcome.mp3 if exist
audio_play_mp3("welcome.mp3");
audio_play_tts("LISYclock ready");

//endless loop to show time
while(true)  {    
    //store current time to 'timeinfo', different sources available
    if (wifi_enabled) {
      time(&now);
      localtime_r(&now, &timeinfo);
    }
    else { //get time from RTC if no wifi
      if (ds3231_get_time(&dev, &rtcinfo) != ESP_OK) {
        ESP_LOGI(TAG, "Could not get time.");
        }
      rtcinfo.tm_year=rtcinfo.tm_year-1900;
    	time_t rtcnow = mktime(&rtcinfo);
	    localtime_r(&rtcnow, &timeinfo);
    }

    if (display_enabled) {
    if ( delimeter == '-') delimeter=' '; else delimeter='-';
    sprintf(str_display1," %02d%c%02d",timeinfo.tm_hour, delimeter, timeinfo.tm_min);
    display1.showString(str_display1);    
    sprintf(str_display2," %02d %02d",timeinfo.tm_mday, timeinfo.tm_mon + 1);
    display2.showString(str_display2);
    sprintf(str_display3,"%s",weekday[timeinfo.tm_wday]);
    display3.showString(str_display3);
    sprintf(str_display4," %04d ",timeinfo.tm_year + 1900);
    display4.showString(str_display4);
    }
    else {
      display1.showString("      ");
      display2.showString("      ");
      display3.showString("      ");
      display4.showString("      ");
    }

    // check each minute if we have an 'event'
    if ( timeinfo.tm_min != old_minute) {
        old_minute = timeinfo.tm_min;
        xSemaphoreGive(event_task_Semaphore);
    }
  
    vTaskDelay(1000 / portTICK_PERIOD_MS);
      
    //test actions

    if (( show_ip) & (wifi_enabled)) {
      show_ip = 0;
      display1.showString(my_ip_addr[0]);
      display2.showString(my_ip_addr[1]);
      display3.showString(my_ip_addr[2]);
      display4.showString(my_ip_addr[3]);
      delay(5000);
     }
     /*
     if (( set_time) & (dip_wifi == 0)) {
        set_time = 0;
        //start background routine
        xTaskCreatePinnedToCore(&cb_set_time, "set_time", 4096, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  

        switch ( set_time_phase ) {
            case 0:


        }
     }
        */
  }
}