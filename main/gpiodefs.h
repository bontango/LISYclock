#ifndef GPIODEFS_H
#define	GPIODEFS_H

#define LISYCLOCK2 TRUE



//all 22 GPIO assigments for LISYclock HW 2.xx
#ifdef LISYCLOCK2
#define LISYCLOCK_VERSION "v2.37 " 
// 8 Digital Pins for Displays (lisyclock.cpp)
#define CLK1 15
#define DIO1 16
#define CLK2 13
#define DIO2 14
#define CLK3 6
#define DIO3 7
#define CLK4 21
#define DIO4 47
// 2 Digital Pins for I2C (lisyclock.cpp)
#define I2C_SDA 17
#define I2C_SCL 18
#define RTC_INT 8
// 3 Digital Pins for I2S (audio.c)
#define I2S_BLK_PIN 9       // Bit Clock (BCLK)
#define I2S_WS_PIN 3        // Word Select (LRCLK)
#define I2S_DATA_OUT_PIN 10  // Serial Data Out (DIN to MAX98357A)
// 3 Digital Pins for buttons/dips (buttons.c)
#define ADJUST_GPIO 4
#define SET_GPIO 5
#define READ_DIP_GPIO 12
#define DIP1_GPIO I2S_WS_PIN
#define DIP2_GPIO I2S_BLK_PIN
#define DIP3_GPIO I2S_DATA_OUT_PIN
// 1 Digital Pin for LEDs (leds.c)
#define LED_STRIP_GPIO_PIN  11
// 4 Digital Pins for SD card (sdcard.c)
#define PIN_NUM_MISO  48
#define PIN_NUM_MOSI  2
#define PIN_NUM_CLK   38
#define PIN_NUM_CS    1

#else
#define LISYCLOCK_VERSION "v1.37 " 
//all 22 GPIO assigments for LISYclock HW 1.xx
// 8 Digital Pins for Displays (lisyclock.cpp)
#define CLK1 10
#define DIO1 11
#define CLK2 3
#define DIO2 8
#define CLK3 15
#define DIO3 16
#define CLK4 18
#define DIO4 17
// 2 Digital Pins for I2C (lisyclock.cpp)
#define I2C_SDA 12
#define I2C_SCL 13
#define RTC_INT 8
// 3 Digital Pins for I2S (audio.c)
#define I2S_BLK_PIN 47       // Bit Clock (BCLK)
#define I2S_WS_PIN 48        // Word Select (LRCLK)
#define I2S_DATA_OUT_PIN 21  // Serial Data Out (DIN to MAX98357A)
// 3 Digital Pins for buttons/dips (buttons.c)
#define ADJUST_GPIO 6
#define SET_GPIO 7
#define READ_DIP_GPIO 9
#define DIP1_GPIO I2S_DATA_OUT_PIN
#define DIP2_GPIO I2S_BLK_PIN
#define DIP3_GPIO I2S_WS_PIN
// 1 Digital Pin for LEDs (leds.c)
#define LED_STRIP_GPIO_PIN  38
// 4 Digital Pins for SD card (sdcard.c)
#define PIN_NUM_MISO  2
#define PIN_NUM_MOSI  5
#define PIN_NUM_CLK   1
#define PIN_NUM_CS    4

#endif

#endif //GPIODEFS_H