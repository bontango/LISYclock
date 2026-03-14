idf_component_register(
    SRCS "httpserver.c" "event.cpp" "audio.cpp" "set_time.c" "i2cdev.c" "ds3231.c" "ftp.c" "buttons.c" "config.c" "sntp.c" "fupdate.c" "leds.c" "sdcard.c" "usb_com.c" "lisyclock.cpp"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS
        "../components/arduino/cores/esp32"
        "../components/arduino/variants/esp32s3"
        "../components/arduino/libraries/WiFi/src"
        "../components/arduino/libraries/NetworkClientSecure/src"
        "../components/arduino/libraries/HTTPClient/src"
        "../components/arduino/libraries/BackgroundAudio/src"
        "../components/arduino/libraries/ESP32-audioI2S/src"
        "../components/arduino/libraries/TM1637TinyDisplay/src"
        "../components/arduino/libraries/WitAITTS/src"
)
