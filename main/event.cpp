// event.cpp
//
// routines around events
//part of LISYclock

extern "C"
{
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "typedefs.h"
}

static const char *TAG = "event";

event_entry_t *Event_list; 

extern void set_tts_defaults(void);
extern void audio_play_tts(char* text);
extern void audio_play_mp3(const char *filename);
extern "C" void trim_str( char *org, char *trim, int maxlen);

extern tm timeinfo;
extern char _voice[21],_style[21],_sfxCharacter[21],_sfxEnvironment[21];
extern int _speed, _pitch;
extern float _gain;

extern bool display_enabled;
extern bool attract_leds_enabled;
extern "C" void gi_leds(bool activate);

extern "C" void obtain_time(int retries);

//sync event task
extern SemaphoreHandle_t event_task_Semaphore;

//maximum 122 chars per line, we may need to extend that!
#define LBUF_SZ 122 

void event_execute_file(char *fname) {
    FILE *fp;
    char key[41];
    char val[81];
    char lbuf[LBUF_SZ];
    char tmp_buf[81];
    int my_delay;

    //add '/sdcard/' to filename using tmp_buf
    sprintf(tmp_buf, "/sdcard/%s",fname);
  
    if ((fp = fopen(tmp_buf,"r")) == NULL) {        
        ESP_LOGW(TAG, "open file %s failed, errno = %d",tmp_buf,errno);
        return;
    }
    //read all lines
    while (fgets(lbuf,LBUF_SZ,fp) != NULL) {
        int k = sscanf(lbuf," %40[^#=] = %80[^#=]",key,val);
        if (k == 2) {
            if (strcmp(key,"TTS_Voice") == 0) { trim_str( val, _voice, 20); }
            else if (strcmp(key,"TTS_Style") == 0) { trim_str( val, _style, 20); }
            else if (strcmp(key,"TTS_Speed") == 0) { _speed = atoi(val); }
            else if (strcmp(key,"TTS_Pitch") == 0) { _pitch = atoi(val); }
            else if (strcmp(key,"TTS_Gain") == 0) { _gain = atof(val); _gain = _gain / (float)100; }
            else if (strcmp(key,"TTS_SFXChar") == 0) { trim_str( val, _sfxCharacter, 20); }
            else if (strcmp(key,"TTS_SFXEnv") == 0) { trim_str( val, _sfxEnvironment, 20); }
            else if (strcmp(key,"EVENT_PLAY_TTS") == 0) {
                trim_str( val, tmp_buf, 80);
                audio_play_tts(tmp_buf);
            }
            else if (strcmp(key,"EVENT_PLAY_MP3") == 0) {
                trim_str( val, tmp_buf, 80);
                audio_play_mp3(tmp_buf);
            }
            else if (strcmp(key,"EVENT_PAUSE") == 0) {
                my_delay = atoi(val);
                vTaskDelay(my_delay * 1000 / portTICK_PERIOD_MS);
            }
            else if (strcmp(key,"EVENT_DISPLAYS") == 0) {
                trim_str( val, val, 80);
                if (strcmp(val,"off") == 0) display_enabled = false;
                    else display_enabled = true;
            }
            else if (strcmp(key,"EVENT_GI_LEDS") == 0) {
                trim_str( val, val, 80);
                if (strcmp(val,"off") == 0) gi_leds(false);
                    else gi_leds(true);
            }
            else if (strcmp(key,"EVENT_ATTRACT_LEDS") == 0) {
                trim_str( val, val, 80);
                if (strcmp(val,"off") == 0) attract_leds_enabled = false;
                    else attract_leds_enabled = true;                
            }
            else {
                ESP_LOGW(TAG, "unknown key, cfg line: key:%s< val:%s<",key,val);  
            }
          //ESP_LOGI(TAG, "cfg line key:%s< val:%s<",key,val);  
        } //if k==2     
    } //while lines available
    fclose(fp);
}

void event_task(void *pvParameters) {

    bool time_match;
    event_entry_t *e;

  while(1) {
    // Wait until we can take semaphore
    if (xSemaphoreTake(event_task_Semaphore, portMAX_DELAY))
    {

    // start at top of list
    e = Event_list;
    time_match = false;

    ESP_LOGI(TAG, "event task started at %d:%d:%d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_wday);
    ESP_LOGI(TAG,"free heap size is %d",xPortGetFreeHeapSize());

    //search complete list for time match
    for( ; e != NULL ; e = e->next ) {        
        if (( e->tm_min != -1) & ( e->tm_min != timeinfo.tm_min)) {  time_match = false; }
        else if (( e->tm_hour != -1) & ( e->tm_hour != timeinfo.tm_hour)) {  time_match = false; }
        else if (( e->tm_wday != -1) & ( e->tm_wday != timeinfo.tm_wday)) {  time_match = false; }
        else if (( e->tm_mday != -1) & ( e->tm_mday != timeinfo.tm_mday)) {  time_match = false; }
        else if (( e->tm_mon != -1) & ( e->tm_mon != timeinfo.tm_mon)) {  time_match = false; }
        else if (( e->tm_year != -1) & ( e->tm_year != timeinfo.tm_year)) {  time_match = false; }
        else {  time_match = true; }

                if ( time_match) {
                ESP_LOGI(TAG, "we have a match: %s", e->text );
                ESP_LOGI(TAG, "Event type %d red: %d , %d , %d, %d, %d, %d, %s",e->event_type,e->tm_hour,e->tm_min,e->tm_mday,e->tm_wday,e->tm_mon,e->tm_year,e->text);
                switch(e->event_type) {
                    case EVENT_TYPE_BATCH: event_execute_file(e->text); break;
                    case EVENT_TYPE_MP3: audio_play_mp3(e->text); break;
                    case EVENT_TYPE_TTS: audio_play_tts(e->text); break;
                    case EVENT_TYPE_DISPLAYS:
                        if (strcmp(e->text,"off") == 0) display_enabled = false;
                        else display_enabled = true;
                        break;
                    case EVENT_TYPE_GI_LEDS:
                        if (strcmp(e->text,"off") == 0) gi_leds(false);
                        else gi_leds(true);
                        break;
                    case EVENT_TYPE_ATTRACT_LEDS:
                        if (strcmp(e->text,"off") == 0) attract_leds_enabled = false;
                        else attract_leds_enabled = true;
                        break;
                    case EVENT_TYPE_TIME:                        
                        obtain_time(atoi(e->text));
                        break;
                    default: ESP_LOGW(TAG, "unknown event type");
                }
        //reset TTS parameters
        set_tts_defaults();
        }
    }//for

    ESP_LOGI(TAG, "event task ended");

    }//if semaphore
  } //while(1)
} //task
