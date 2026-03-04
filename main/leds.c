// leds
//
// led strip control 
//part of LISYclock

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "typedefs.h"
#include "esp_log.h"
#include "gpiodefs.h"

extern Led_gi_t cfg_led_gi[];
extern Led_at_t cfg_led_at1[];
extern Led_at_t cfg_led_at2[];
extern Led_at_t cfg_led_at3[];
extern Led_at_t cfg_led_at4[];
extern Led_at_t cfg_led_at5[];
extern int cfg_at1_blink_rate; //attract mode blinkrate in multiple of 100ms
extern int cfg_at2_blink_rate; //attract mode blinkrate in multiple of 100ms
extern int cfg_at3_blink_rate; //attract mode blinkrate in multiple of 100ms
extern int cfg_at4_blink_rate; //attract mode blinkrate in multiple of 100ms
extern int cfg_at5_blink_rate; //attract mode blinkrate in multiple of 100ms
extern uint8_t cfg_at1_rand; //random LED activation in attract
extern uint8_t cfg_at2_rand; //random LED activation in attract
extern uint8_t cfg_at3_rand; //random LED activation in attract
extern uint8_t cfg_at4_rand; //random LED activation in attract
extern uint8_t cfg_at5_rand; //random LED activation in attract
extern led_strip_handle_t led_strip;
extern bool attract_leds_enabled;

// defines for the LED strip
// Set to 1 to use DMA for driving the LED strip, 0 otherwise
// Please note the RMT DMA feature is only available on chips e.g. ESP32-S3/P4
#define LED_STRIP_USE_DMA  0

#if LED_STRIP_USE_DMA
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 256
#define LED_STRIP_MEMORY_BLOCK_WORDS 1024 // this determines the DMA block size
#else
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 31
#define LED_STRIP_MEMORY_BLOCK_WORDS 0 // let the driver choose a proper memory block size automatically
#endif // LED_STRIP_USE_DMA

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));    
    return led_strip;
}

void gi_leds(bool activate) {
int i;
if (activate) {
  i=0;
  while ( cfg_led_gi[i].num != 0 ) { //0 indicates end of list
    //index starting at 0
    led_strip_set_pixel(led_strip, cfg_led_gi[i].num - 1, cfg_led_gi[i].r, cfg_led_gi[i].g, cfg_led_gi[i].b );
    i++;
  };
}
else {
   for (i=0; i<LED_STRIP_LED_COUNT; i++)
    led_strip_set_pixel(led_strip, i, 0, 0, 0 );
}
}

void init_leds(uint8_t activate) {

int i;

//GI LEDs
gi_leds(activate);

//attract mode LEDs initial OFF
i=0;
while ( cfg_led_at1[i].num != 0 ) { //0 indicates end of list
    //index starting at 0
    led_strip_set_pixel(led_strip, cfg_led_at1[i].num - 1, 0, 0, 0 );
    i++;
};
i=0;
while ( cfg_led_at2[i].num != 0 ) { //0 indicates end of list
    //index starting at 0
    led_strip_set_pixel(led_strip, cfg_led_at2[i].num - 1, 0, 0, 0 );
    i++;
};
i=0;
while ( cfg_led_at3[i].num != 0 ) { //0 indicates end of list
    //index starting at 0
    led_strip_set_pixel(led_strip, cfg_led_at3[i].num - 1, 0, 0, 0 );
    i++;
};
i=0;
while ( cfg_led_at4[i].num != 0 ) { //0 indicates end of list
    //index starting at 0
    led_strip_set_pixel(led_strip, cfg_led_at4[i].num - 1, 0, 0, 0 );
    i++;
};
i=0;
while ( cfg_led_at5[i].num != 0 ) { //0 indicates end of list
    //index starting at 0
    led_strip_set_pixel(led_strip, cfg_led_at5[i].num - 1, 0, 0, 0 );
    i++;
};


led_strip_refresh(led_strip);

}

// ---------------------
// attract
// ---------------------
void attract_leds_at1(void *pvParameters) {

int i;
int num = 0;

//how many leds?
while ( cfg_led_at1[num].num != 0 ) { num++; }//0 indicates end of list
//no LEDs configured 
if ( num == 0 ) vTaskDelete( NULL );

//Formel (rand() % (oberer_grenzwert - unterer_grenzwert + 1)) + unterer_grenzwert
while(true) { 
   if(attract_leds_enabled) { //attract LEDs active?
    if ( cfg_at1_rand != 0) { //random?
        i = rand() % num; // result is 0 to num-1 
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at1[i].num - 1, cfg_led_at1[i].r, cfg_led_at1[i].g, cfg_led_at1[i].b );
        vTaskDelay( cfg_at1_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at1[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at1_blink_rate  / portTICK_PERIOD_MS );
    }
    else {
        for( i=0; i<num; i++) {    
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at1[i].num - 1, cfg_led_at1[i].r, cfg_led_at1[i].g, cfg_led_at1[i].b );
        vTaskDelay( cfg_at1_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at1[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at1_blink_rate  / portTICK_PERIOD_MS );
        };
    }
   } 
   else { //attract LEDs disabled
    for( i=0; i<num; i++) {   
        led_strip_set_pixel(led_strip, cfg_led_at1[i].num - 1, 0, 0, 0); //LED off
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
   } 
} //endless loop
}//attract_leds_at1

void attract_leds_at2(void *pvParameters) {

int i;
int num = 0;

//how many leds?
while ( cfg_led_at2[num].num != 0 ) { num++; }//0 indicates end of list
//no LEDs configured 
if ( num == 0 ) vTaskDelete( NULL );

//Formel (rand() % (oberer_grenzwert - unterer_grenzwert + 1)) + unterer_grenzwert
while(true) {
   if(attract_leds_enabled) { //attract LEDs active?    
    if ( cfg_at2_rand != 0) { //random?
        i = rand() % num; // result is 0 to num-1 
        //ESP_LOGI("LEDS", "index %d LED %d",i, cfg_led_at2[i].num - 1);
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at2[i].num - 1, cfg_led_at2[i].r, cfg_led_at2[i].g, cfg_led_at2[i].b );
        vTaskDelay( cfg_at2_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at2[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at2_blink_rate  / portTICK_PERIOD_MS );
    }
    else {
        for( i=0; i<num; i++) {    
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at2[i].num - 1, cfg_led_at2[i].r, cfg_led_at2[i].g, cfg_led_at2[i].b );
        vTaskDelay( cfg_at2_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at2[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at2_blink_rate  / portTICK_PERIOD_MS );
        };
    }
   } 
   else { //attract LEDs disabled
    for( i=0; i<num; i++) {   
        led_strip_set_pixel(led_strip, cfg_led_at2[i].num - 1, 0, 0, 0); //LED off
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
   } 
} //endless loop
}//attract_leds_at2

void attract_leds_at3(void *pvParameters) {

int i;
int num = 0;

//how many leds?
while ( cfg_led_at3[num].num != 0 ) { num++; }//0 indicates end of list
//no LEDs configured 
if ( num == 0 ) vTaskDelete( NULL );

//Formel (rand() % (oberer_grenzwert - unterer_grenzwert + 1)) + unterer_grenzwert
while(true) {
   if(attract_leds_enabled) { //attract LEDs active?    
    if ( cfg_at3_rand != 0) { //random?
        i = rand() % num; // result is 0 to num-1 
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at3[i].num - 1, cfg_led_at3[i].r, cfg_led_at3[i].g, cfg_led_at3[i].b );
        vTaskDelay( cfg_at3_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at3[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at3_blink_rate  / portTICK_PERIOD_MS );
    }
    else {
        for( i=0; i<num; i++) {    
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at3[i].num - 1, cfg_led_at3[i].r, cfg_led_at3[i].g, cfg_led_at3[i].b );
        vTaskDelay( cfg_at3_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at3[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at3_blink_rate  / portTICK_PERIOD_MS );
        };
    }
   } 
   else { //attract LEDs disabled
    for( i=0; i<num; i++) {   
        led_strip_set_pixel(led_strip, cfg_led_at3[i].num - 1, 0, 0, 0); //LED off
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
   } 
} //endless loop
}//attract_leds_at3

void attract_leds_at4(void *pvParameters) {

int i;
int num = 0;

//how many leds?
while ( cfg_led_at4[num].num != 0 ) { num++; }//0 indicates end of list
//no LEDs configured 
if ( num == 0 ) vTaskDelete( NULL );

//Formel (rand() % (oberer_grenzwert - unterer_grenzwert + 1)) + unterer_grenzwert
while(true) {
   if(attract_leds_enabled) { //attract LEDs active?    
    if ( cfg_at4_rand != 0) { //random?
        i = rand() % num; // result is 0 to num-1 
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at4[i].num - 1, cfg_led_at4[i].r, cfg_led_at4[i].g, cfg_led_at4[i].b );
        vTaskDelay( cfg_at4_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at4[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at4_blink_rate  / portTICK_PERIOD_MS );
    }
    else {
        for( i=0; i<num; i++) {    
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at4[i].num - 1, cfg_led_at4[i].r, cfg_led_at4[i].g, cfg_led_at4[i].b );
        vTaskDelay( cfg_at4_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at4[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at4_blink_rate  / portTICK_PERIOD_MS );
        };
    }
   } 
   else { //attract LEDs disabled
    for( i=0; i<num; i++) {   
        led_strip_set_pixel(led_strip, cfg_led_at4[i].num - 1, 0, 0, 0); //LED off
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
   } 
} //endless loop
}//attract_leds_at4

void attract_leds_at5(void *pvParameters) {

int i;
int num = 0;

//how many leds?
while ( cfg_led_at5[num].num != 0 ) { num++; }//0 indicates end of list
//no LEDs configured 
if ( num == 0 ) vTaskDelete( NULL );

//Formel (rand() % (oberer_grenzwert - unterer_grenzwert + 1)) + unterer_grenzwert
while(true) {
   if(attract_leds_enabled) { //attract LEDs active?    
    if ( cfg_at5_rand != 0) { //random?
        i = rand() % num; // result is 0 to num-1 
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at5[i].num - 1, cfg_led_at5[i].r, cfg_led_at5[i].g, cfg_led_at5[i].b );
        vTaskDelay( cfg_at5_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at5[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at5_blink_rate  / portTICK_PERIOD_MS );
    }
    else {
        for( i=0; i<num; i++) {    
        //index starting at 0
        led_strip_set_pixel(led_strip, cfg_led_at5[i].num - 1, cfg_led_at5[i].r, cfg_led_at5[i].g, cfg_led_at5[i].b );
        vTaskDelay( cfg_at5_blink_rate / portTICK_PERIOD_MS );
        led_strip_set_pixel(led_strip, cfg_led_at5[i].num - 1, 0, 0, 0); //LED off
        vTaskDelay( cfg_at5_blink_rate  / portTICK_PERIOD_MS );
        };
    }
   } 
   else { //attract LEDs disabled
    for( i=0; i<num; i++) {   
        led_strip_set_pixel(led_strip, cfg_led_at5[i].num - 1, 0, 0, 0); //LED off
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
   } 
} //endless loop
}//attract_leds_at5

//call the five attract mode tasks
void attract_leds(void *pvParameter) {

//init rand
//srand(time(NULL));

xTaskCreatePinnedToCore(&attract_leds_at1, "attract_leds_at1", 2048, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  
xTaskCreatePinnedToCore(&attract_leds_at2, "attract_leds_at2", 2048, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  
xTaskCreatePinnedToCore(&attract_leds_at3, "attract_leds_at3", 2048, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  
xTaskCreatePinnedToCore(&attract_leds_at4, "attract_leds_at3", 2048, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  
xTaskCreatePinnedToCore(&attract_leds_at5, "attract_leds_at3", 2048, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  

//100ms refresh rate for attract LEDs
while(true) {
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    led_strip_refresh(led_strip);
} //endless loop

}