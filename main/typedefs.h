#ifndef TYPEDEFS_H
#define	TYPEDEFS_H

#include <stdint.h>

#define EVENT_TYPE_TTS 1
#define EVENT_TYPE_MP3 2
#define EVENT_TYPE_BATCH 3
#define EVENT_TYPE_DISPLAYS 4
#define EVENT_TYPE_GI_LEDS 5
#define EVENT_TYPE_ATTRACT_LEDS 6
#define EVENT_TYPE_TIME 7

// GI LED cfg
typedef struct {
    uint8_t num;  // LED number 1 ... x; 0 indicates end of list
    uint8_t r;  // red
    uint8_t g;  // green
    uint8_t b;  // blue
} Led_gi_t;

// attract LED cfg
typedef struct {
    uint8_t num;  // LED number 1 ... x; 0 indicates end of list
    uint8_t r;  // red
    uint8_t g;  // green
    uint8_t b;  // blue
} Led_at_t;

// event routine list
typedef struct event_entry {
    uint8_t num;  // attract file number 1 ... x; 0 indicates end of list    
    int	tm_min;  // time of attract file to start
    int	tm_hour;
    int	tm_mday;
    int	tm_wday;
    int	tm_mon;
    int	tm_year;
    int	event_type;
    char* text;  // TTS | MP3 filename | file with attract mode commands
    struct event_entry *next; /* pointer to next event */
}event_entry_t;

#endif //TYPEDEFS_H