#include <stdio.h>
#include "driver\gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>     
#include <iot_button.h>
#include <button_gpio.h>
#include "esp_err.h"
#include "gpiodefs.h"

static const char *TAG = "buttons";

extern uint8_t play_it;
extern uint8_t show_ip;
extern uint8_t set_time;

extern uint8_t wifi_enabled;
extern uint8_t attract_enabled;
extern uint8_t sound_disabled;

//functions
static void adj_single_click_cb(void *arg,void *usr_data)
{
    ESP_LOGI(TAG, "time adjust button pressed");
    play_it = 1;
}

static void set_single_click_cb(void *arg,void *usr_data)
{
    ESP_LOGI(TAG, "time set button pressed");
    show_ip = 1;
}

static void set_long_press_cb(void *arg,void *usr_data)
{
    ESP_LOGI(TAG, "time set button LONG pressed");
    set_time = 1;
}

// create gpio buttons and read DIP settings
esp_err_t button_init(void) {

//time adjust button    
const button_config_t btn_adj_cfg = {0};
const button_gpio_config_t btn_adj_gpio_cfg = {
    .gpio_num = ADJUST_GPIO,
    .active_level = 0,
};
button_handle_t gpio_adj_btn = NULL;
//time set button    
const button_config_t btn_set_cfg = {0};
const button_gpio_config_t btn_set_gpio_cfg = {
    .gpio_num = SET_GPIO,
    .active_level = 0,
};
button_handle_t gpio_set_btn = NULL;

gpio_set_direction(READ_DIP_GPIO, GPIO_MODE_INPUT);
gpio_set_pull_mode(READ_DIP_GPIO, GPIO_PULLUP_ONLY);
gpio_set_direction(DIP1_GPIO, GPIO_MODE_OUTPUT);
gpio_set_direction(DIP2_GPIO, GPIO_MODE_OUTPUT);
gpio_set_direction(DIP3_GPIO, GPIO_MODE_OUTPUT);
//read matrix
gpio_set_level(DIP1_GPIO, 0);
gpio_set_level(DIP2_GPIO, 1);
gpio_set_level(DIP3_GPIO, 1);
vTaskDelay( 100 / portTICK_PERIOD_MS );
wifi_enabled = gpio_get_level(READ_DIP_GPIO);
//ESP_LOGI(TAG, "DIP1 level is %d", dip1_level);
gpio_set_level(DIP1_GPIO, 1);
gpio_set_level(DIP2_GPIO, 0);
vTaskDelay( 100 / portTICK_PERIOD_MS );
attract_enabled = gpio_get_level(READ_DIP_GPIO);
//ESP_LOGI(TAG, "DIP2 level is %d", dip2_level);
gpio_set_level(DIP2_GPIO, 1);
gpio_set_level(DIP3_GPIO, 0);
vTaskDelay( 100 / portTICK_PERIOD_MS );
sound_disabled = gpio_get_level(READ_DIP_GPIO);
//ESP_LOGI(TAG, "DIP3 level is %d", dip3_level);
gpio_set_level(DIP1_GPIO, 0);
gpio_set_level(DIP2_GPIO, 0);
gpio_set_level(DIP3_GPIO, 0);
//deactivate pullup now
gpio_set_pull_mode(READ_DIP_GPIO, GPIO_FLOATING);

//------------------------------------
//push buttons
//------------------------------------
esp_err_t ret = iot_button_new_gpio_device(&btn_adj_cfg, &btn_adj_gpio_cfg, &gpio_adj_btn);
if(NULL == gpio_adj_btn) {
    ESP_LOGE(TAG, "adj Button create failed");
    return(ret);
}
iot_button_register_cb(gpio_adj_btn, BUTTON_SINGLE_CLICK, NULL, adj_single_click_cb,NULL);

ret = iot_button_new_gpio_device(&btn_set_cfg, &btn_set_gpio_cfg, &gpio_set_btn);
if(NULL == gpio_set_btn) {
    ESP_LOGE(TAG, "set Button create failed");
    return(ret);
}
iot_button_register_cb(gpio_set_btn, BUTTON_SINGLE_CLICK, NULL, set_single_click_cb,NULL);

button_event_args_t args = {
    .long_press.press_time = 1500,
};
iot_button_register_cb(gpio_set_btn, BUTTON_LONG_PRESS_START, &args, set_long_press_cb, NULL);

return(ret);

}