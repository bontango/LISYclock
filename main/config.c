// config
//
// handling of config file
//part of LISYclock

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_err.h"
#include "typedefs.h"

static const char *TAG = "config";

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
extern uint8_t cfg_disp_bright; //display brightness ( 0..7 )
extern char weekday[7][7]; //weekday representation
extern char timezone_str[31];
extern uint8_t ftp_server_enable;
extern char ftp_user[41];
extern char ftp_pass[41];
extern char _witToken[41];
extern char d_voice[21],d_style[21],d_sfxCharacter[21],d_sfxEnvironment[21];
extern int d_speed, d_pitch;
extern float d_gain;

extern bool display_enabled;
extern bool gi_leds_enabled;
extern bool attract_leds_enabled;

extern event_entry_t *Event_list; 

//input is sring of one field in event time ( <MM:HH-DD.MM.YYYY>|<MM:HH:W>)
//check for wildcards and NULL pointer
// return value 
// -1 in case of wildcard
// value ( atoi) otherwise
int get_value_event_field(char *val) {

        int i = 0;

        while(val[i] != '\0') {
            if ( val[i] == '*') { return(-1); } //wildcard
            i++;
        }

        return (atoi(val));
}

// interprete and check date at EVENT and translate to internal format
//format:
//EVENT_TTS=<MM:HH-DD.MM.YYYY>|<MM:HH:W>,"text to speak"
esp_err_t append_event(char *val, event_entry_t **lst, int event_type) {

    char *str_time_and_date, *str_text;
    char *str_time, *str_date;
    int minute, hour, wday, day, month, year;
    char *str_minute, *str_hour, *str_wday, *str_day, *str_month, *str_year;
    
    // format is <HH:MM-DD.MM.YYYY>|<HH:MM:W>,"MP3 file| text for TTS | batch file"
    //split to two strings time/date & text
    str_time_and_date = strtok(val, ",");
    ESP_LOGI(TAG, "time_and_date is %s",str_time_and_date);  
    str_text = strtok(NULL, ",");
    ESP_LOGI(TAG, "text is %s",str_text);  

    //split time/date to two strings time & date
    //<HH:MM-DD.MM.YYYY>|<HH:MM:W>
    if ( (str_time = strtok(str_time_and_date, "-")) == NULL) return ESP_ERR_NOT_FOUND;
    ESP_LOGI(TAG, "time is %s",str_time); 
    if ( (str_date = strtok(NULL, "-")) == NULL ) {
        //<MM:HH:W>
        ESP_LOGI(TAG, "date missing! assuming wday");

        if ( (str_hour = strtok(str_time,":")) == NULL) return ESP_ERR_NOT_FOUND;
        hour = get_value_event_field(str_hour);        
        ESP_LOGI(TAG, "hour is %s (%d)",str_hour, hour);          

        if ( (str_minute = strtok(NULL,":")) == NULL) return ESP_ERR_NOT_FOUND;
        minute = get_value_event_field(str_minute);
        ESP_LOGI(TAG, "minute is %s (%d)",str_minute,minute);  

        if ( (str_wday = strtok(NULL,":")) == NULL) return ESP_ERR_NOT_FOUND;
        wday = get_value_event_field(str_wday);        
        ESP_LOGI(TAG, "wday is %s (%d)",str_wday, wday);         
        
        //with wday (weekday) we set dae to wildcard
        day = month = year = -1;

        }
    else {
        //<HH:MM>
        if ( (str_hour = strtok(str_time,":")) == NULL) return ESP_ERR_NOT_FOUND;
        hour = get_value_event_field(str_hour);        
        ESP_LOGI(TAG, "hour is %s (%d)",str_hour, hour);          

        if ( (str_minute = strtok(NULL,":")) == NULL) return ESP_ERR_NOT_FOUND;
        minute = get_value_event_field(str_minute);
        ESP_LOGI(TAG, "minute is %s (%d)",str_minute,minute);  

        //wday (weekday) set to wildcard here
        wday = -1;

        //now interpret date   <DD.MM.YYYY>
        ESP_LOGI(TAG, "date is %s",str_date);  
        if ( (str_day = strtok(str_date,".")) == NULL) return ESP_ERR_NOT_FOUND;
        day = get_value_event_field(str_day);        
        ESP_LOGI(TAG, "day is %s (%d)",str_day, day);  

        if ( (str_month = strtok(NULL,".")) == NULL) return ESP_ERR_NOT_FOUND;
        month = get_value_event_field(str_month);        
        ESP_LOGI(TAG, "month is %s (%d)",str_month, month);  

        if ( (str_year = strtok(NULL,".")) == NULL) return ESP_ERR_NOT_FOUND;
        year = get_value_event_field(str_year);        
        ESP_LOGI(TAG, "year is %s (%d)",str_year, year);  
    }

    //now add event as new entry to the list
    event_entry_t *new_entry;
    
    // search for last entry 
    while( *lst != NULL ) 
    {
        lst = &(*lst)->next;
    }

    new_entry = malloc(sizeof(*new_entry)); // create new entry
    new_entry->text = strdup(str_text);
    new_entry->tm_min = minute;
    new_entry->tm_hour = hour;
    new_entry->tm_mday = day;
    new_entry->tm_wday = wday;
    new_entry->tm_mon = month;
    new_entry->tm_year = year;
    new_entry->event_type = event_type;
    new_entry->next = NULL; // end of list marker

    *lst = new_entry;

    return ESP_OK;
}


//trim config string in case of framing " " (needed for spaces)
//remove also newline and CR and and limit to maxlen chars
void trim_str( char *org, char *trim, int maxlen) {
    int i = 0, j = 0;
    while (org[i]) {
        if ((org[i] != '\"') & (org[i] != '\r') & (org[i] != '\n')){
            trim[j++] = org[i];
        }
        i++;
        if ( i == maxlen-1) break;
    }
    trim[j] = '\0'; // Neuer Null-Terminator
}

//----------------------------------------------------------------------------------------
// set all defaults
//----------------------------------------------------------------------------------------

void InitConfig(void) {

const uint8_t gi[] = { 1, 2, 5, 6, 7, 9, 10, 24, 25, 28, 29, 30, 0 }; //0 indicates end of list
const uint8_t at1[] = { 11, 12, 16, 17, 18, 15, 19, 21, 23, 14, 13, 20, 0}; //0 indicates end of list

strcpy(timezone_str,"CET-1CEST,M3.5.0,M10.5.0/3");

strcpy( weekday[0],"Sunday");
strcpy( weekday[1],"Monday");
strcpy( weekday[2],"Tuesda");
strcpy( weekday[3],"Wednes");
strcpy( weekday[4],"Thursd");
strcpy( weekday[5],"Friday");
strcpy( weekday[6],"Saturd");

ftp_server_enable = 1;
strcpy( ftp_user, "lisy");
strcpy( ftp_pass, "bontango");

//init event list
Event_list = NULL;

int i;

    i = 0;
    do {
        cfg_led_gi[i].num = gi[i]; cfg_led_gi[i].r = cfg_led_gi[i].g = cfg_led_gi[i].b = 166;
        i++;
    } while ( gi[i] != 0);

    i = 0;
    do {
        cfg_led_at1[i].num = at1[i]; cfg_led_at1[i].r = cfg_led_at1[i].g = 0; cfg_led_at1[i].b = 66;
        i++;
    } while ( at1[i] != 0);
    cfg_led_at2[0].num = 0;
    cfg_led_at3[0].num = 0;
    cfg_led_at4[0].num = 0;
    cfg_led_at5[0].num = 0;

}


// read config file and override default values
//
//maximum 122 chars per line, we may need to extend that!
#define LBUF_SZ 122 

//gives back (last) linenumber with Error, 0 otherwise
int ReadConfig(char *fname) {
    FILE *fp;
    uint8_t GI_LED_nu = 0;
    uint8_t AT1_LED_nu = 0;
    uint8_t AT2_LED_nu = 0;
    uint8_t AT3_LED_nu = 0;
    uint8_t AT4_LED_nu = 0;
    uint8_t AT5_LED_nu = 0;

    int num,r,g,b,ret;
    int line = 0;
    int err_line = 0;
    char key[41];
    char val[81];
    char lbuf[LBUF_SZ];

    if ((fp = fopen(fname,"r")) == NULL) {        
        ESP_LOGW(TAG, "open file %s failed, errno = %d",fname,errno);
        return(-1);
    }
    //read all lines
    while (fgets(lbuf,LBUF_SZ,fp) != NULL) {

        //count lines
        line++;

        int k = sscanf(lbuf," %40[^#=] = %80[^#=]",key,val);
        if (k == 2) {
            if (strcmp(key,"FTP_SERVER_ENABLE") == 0) {
                if ( val[0] == 'y') ftp_server_enable = 1;
            }     
            else if (strcmp(key,"FTP_USER") == 0) {
                trim_str( val, ftp_user, 40);
            }     
            else if (strcmp(key,"FTP_PASS") == 0) {
                trim_str( val, ftp_pass, 40);
            }     
            else if (strcmp(key,"TIMEZONE") == 0) {
                trim_str( val, timezone_str, 40);
            }     
            else if (strcmp(key,"DAY_SUN") == 0) {
                trim_str( val, weekday[0], 6);
            }     
            else if (strcmp(key,"DAY_MON") == 0) {
                trim_str( val, weekday[1], 6);
            }     
            else if (strcmp(key,"DAY_TUE") == 0) {
                trim_str( val, weekday[2], 6);
            }     
            else if (strcmp(key,"DAY_WED") == 0) {
                trim_str( val, weekday[3], 6);
            }     
            else if (strcmp(key,"DAY_THU") == 0) {
                trim_str( val, weekday[4], 6);
            }     
            else if (strcmp(key,"DAY_FRI") == 0) {
                trim_str( val, weekday[5], 6);
            }     
            else if (strcmp(key,"DAY_SAT") == 0) {
                trim_str( val, weekday[6], 6);
            }                             
            else if (strcmp(key,"DISP_BRIGHT") == 0) {
                cfg_disp_bright = atoi(val);
            }            
            else if (strcmp(key,"GI_LED") == 0) {
                ret = sscanf(val," %d , %d , %d , %d",&num,&r,&g,&b);
                if (ret == 4) {
                    cfg_led_gi[GI_LED_nu].num = num;
                    cfg_led_gi[GI_LED_nu].r = r;
                    cfg_led_gi[GI_LED_nu].g = g;
                    cfg_led_gi[GI_LED_nu].b = b;
                    GI_LED_nu++;
                    cfg_led_gi[GI_LED_nu].num = 0; //mark end of list
                }   
            }           
            else if (strcmp(key,"AT1_BLINK_RATE") == 0) { cfg_at1_blink_rate = atoi(val); }            
            else if (strcmp(key,"AT2_BLINK_RATE") == 0) { cfg_at2_blink_rate = atoi(val); }            
            else if (strcmp(key,"AT3_BLINK_RATE") == 0) { cfg_at3_blink_rate = atoi(val); }       
            else if (strcmp(key,"AT4_BLINK_RATE") == 0) { cfg_at4_blink_rate = atoi(val); }
            else if (strcmp(key,"AT5_BLINK_RATE") == 0) { cfg_at5_blink_rate = atoi(val); }
            else if (strcmp(key,"AT1_RAND") == 0) { cfg_at1_rand = atoi(val); }            
            else if (strcmp(key,"AT2_RAND") == 0) { cfg_at2_rand = atoi(val); }            
            else if (strcmp(key,"AT3_RAND") == 0) { cfg_at3_rand = atoi(val); }            
            else if (strcmp(key,"AT4_RAND") == 0) { cfg_at4_rand = atoi(val); }            
            else if (strcmp(key,"AT5_RAND") == 0) { cfg_at5_rand = atoi(val); }            
            else if (strcmp(key,"AT1_LED") == 0) {
                ret = sscanf(val," %d , %d , %d , %d",&num,&r,&g,&b);
                if (ret == 4) {
                    cfg_led_at1[AT1_LED_nu].num = num;
                    cfg_led_at1[AT1_LED_nu].r = r;
                    cfg_led_at1[AT1_LED_nu].g = g;
                    cfg_led_at1[AT1_LED_nu].b = b;
                    AT1_LED_nu++;
                    cfg_led_at1[AT1_LED_nu].num = 0; //mark end of list
                }   
                else ESP_LOGW(TAG, "AT1_LED: wrong number of arguments: %d",ret);                       
            }
            else if (strcmp(key,"AT2_LED") == 0) {
                ret = sscanf(val," %d , %d , %d , %d",&num,&r,&g,&b);
                if (ret == 4) {
                    cfg_led_at2[AT2_LED_nu].num = num;
                    cfg_led_at2[AT2_LED_nu].r = r;
                    cfg_led_at2[AT2_LED_nu].g = g;
                    cfg_led_at2[AT2_LED_nu].b = b;
                    AT2_LED_nu++;
                    cfg_led_at2[AT2_LED_nu].num = 0; //mark end of list
                }       
                else ESP_LOGW(TAG, "AT2_LED: wrong number of arguments: %d",ret);                   
            }
            else if (strcmp(key,"AT3_LED") == 0) {
                ret = sscanf(val," %d , %d , %d , %d",&num,&r,&g,&b);
                if (ret == 4) {
                    cfg_led_at3[AT3_LED_nu].num = num;
                    cfg_led_at3[AT3_LED_nu].r = r;
                    cfg_led_at3[AT3_LED_nu].g = g;
                    cfg_led_at3[AT3_LED_nu].b = b;
                    AT3_LED_nu++;
                    cfg_led_at3[AT3_LED_nu].num = 0; //mark end of list
                }
                else ESP_LOGW(TAG, "AT3_LED: wrong number of arguments: %d",ret);          
            }
            else if (strcmp(key,"AT4_LED") == 0) {
                ret = sscanf(val," %d , %d , %d , %d",&num,&r,&g,&b);
                if (ret == 4) {
                    cfg_led_at4[AT4_LED_nu].num = num;
                    cfg_led_at4[AT4_LED_nu].r = r;
                    cfg_led_at4[AT4_LED_nu].g = g;
                    cfg_led_at4[AT4_LED_nu].b = b;
                    AT4_LED_nu++;
                    cfg_led_at4[AT4_LED_nu].num = 0; //mark end of list
                }
                else ESP_LOGW(TAG, "AT4_LED: wrong number of arguments: %d",ret);          
            }
            else if (strcmp(key,"AT5_LED") == 0) {
                ret = sscanf(val," %d , %d , %d , %d",&num,&r,&g,&b);
                if (ret == 4) {
                    cfg_led_at5[AT5_LED_nu].num = num;
                    cfg_led_at5[AT5_LED_nu].r = r;
                    cfg_led_at5[AT5_LED_nu].g = g;
                    cfg_led_at5[AT5_LED_nu].b = b;
                    AT5_LED_nu++;
                    cfg_led_at5[AT5_LED_nu].num = 0; //mark end of list
                }
                else ESP_LOGW(TAG, "AT5_LED: wrong number of arguments: %d",ret);          
            }
            else if (strcmp(key,"TTS_WIT_TOKEN") == 0) { trim_str( val, _witToken, 40); }
            else if (strcmp(key,"TTS_Voice") == 0) { trim_str( val, d_voice, 20); }
            else if (strcmp(key,"TTS_Style") == 0) { trim_str( val, d_style, 20); }
            else if (strcmp(key,"TTS_Speed") == 0) { d_speed = atoi(val); }
            else if (strcmp(key,"TTS_Pitch") == 0) { d_pitch = atoi(val); }
            else if (strcmp(key,"TTS_Gain") == 0) { d_gain = atof(val); d_gain = d_gain / (float)100; }
            else if (strcmp(key,"TTS_SFXChar") == 0) { trim_str( val, d_sfxCharacter, 20); }
            else if (strcmp(key,"TTS_SFXEnv") == 0) { trim_str( val, d_sfxEnvironment, 20); }
            //EVENT_TTS=<MM:HH-DD.MM.YYYY>|<MM:HH:W>,"text ( TTS | MP3 | Batch)""
            else if (strcmp(key,"EVENT_TTS") == 0) {                
                trim_str( val, val, 80);
                ESP_LOGI( TAG, "EVENT_TTS: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_TTS) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT_TTS"); }
            }
            else if (strcmp(key,"EVENT_MP3") == 0) {                
                trim_str( val, val, 80);
                ESP_LOGI( TAG, "EVENT_MP3: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_MP3) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT_MP3"); }
            }
            else if (strcmp(key,"EVENT_BATCH") == 0) {                
                trim_str( val, val, 80);
                ESP_LOGI( TAG, "EVENT_BATCH: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_BATCH) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT_BAT"); }
            }
            else if (strcmp(key,"EVENT_DISPLAYS") == 0) {                
                trim_str( val, val, 80);
                ESP_LOGI( TAG, "EVENT_DISPLAYS: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_DISPLAYS) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT__DISPLAYS"); }
            }
            else if (strcmp(key,"EVENT_GI_LEDS") == 0) {                
                trim_str( val, val, 80);
                ESP_LOGI( TAG, "EVENT_GI_LEDS: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_GI_LEDS) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT_GI_LEDS"); }
            }
            else if (strcmp(key,"EVENT_ATTRACT_LEDS") == 0) {                
                trim_str( val, val, 80);
                ESP_LOGI( TAG, "EVENT_ATTRACT_LEDS: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_ATTRACT_LEDS) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT_ATTRACT_LEDS"); }
            }
            else if (strcmp(key,"EVENT_TIME") == 0) {      
                trim_str( val, val, 80);          
                ESP_LOGI( TAG, "EVENT_TIME: %s",val);
                if ( append_event(val, &Event_list, EVENT_TYPE_TIME) != ESP_OK ) { err_line = line; ESP_LOGW(TAG, "problem with append EVENT_TIME"); }
            }
            else {
                ESP_LOGW(TAG, "unknown key, cfg line: key:%s< val:%s<",key,val);  
                err_line = line;
            }
          //ESP_LOGI(TAG, "cfg line key:%s< val:%s<",key,val);  
        } //if k==2     
    } //while lines available
    fclose(fp);

    return(err_line);
}
