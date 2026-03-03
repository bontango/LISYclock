# LISYclock

ESP32-S3 firmware for a pinball/arcade-themed clock. Displays time, date, day of week, and year across four 6-digit TM1637 LED displays, with a programmable LED strip, MP3 audio playback, Text-to-Speech, and automatic time synchronization via WiFi (NTP) or a DS3231 RTC module.

## Features

- **4× TM1637 6-digit displays** — shows time (HH:MM), date (DD MM), weekday, and year simultaneously
- **LED strip** — up to 31 addressable RGB LEDs with configurable GI (general illumination) and attract mode (5 blink groups)
- **Audio** — MP3 playback from SD card; Text-to-Speech via Wit.ai REST API
- **Time sync** — NTP over WiFi (primary) or DS3231 RTC (fallback / WiFi-less operation)
- **FTP server** — manage SD card files over WiFi (default credentials: `lisy` / `bontango`)
- **OTA firmware update** — place `update.bin` on the SD card; update runs automatically at boot
- **Event system** — schedule MP3, TTS, LED changes, or display changes at specific times/dates via `config.txt`

## Hardware Versions

Two PCB revisions are supported. Select at compile time in [main/gpiodefs.h](main/gpiodefs.h):

| Version | `#define LISYCLOCK2` | Firmware string |
|---------|----------------------|-----------------|
| HW v1.xx | not defined (default) | `v1.36` |
| HW v2.xx | `#define LISYCLOCK2 TRUE` | `v2.36` |

## DIP Switches

Three DIP switches control runtime behaviour:

| DIP | Function |
|-----|----------|
| DIP1 | Disable WiFi (uses RTC for time) |
| DIP2 | Disable attract LED mode |
| DIP3 | Disable sound |

## SD Card Configuration (`config.txt`)

Place `config.txt` in the root of the SD card. Lines use `KEY = value` format. Comments start with `#`.

### General settings

```
TIMEZONE = CET-1CEST,M3.5.0,M10.5.0/3
DISP_BRIGHT = 3                  # 0..7
FTP_SERVER_ENABLE = y
FTP_USER = lisy
FTP_PASS = bontango
DAY_MON = Mon    # 6-char max weekday label for each display
```

### LED configuration

```
GI_LED = <num>,<r>,<g>,<b>          # always-on LED (repeat for each LED)
AT1_LED = <num>,<r>,<g>,<b>         # attract group 1 LED (up to 5 groups: AT1..AT5)
AT1_BLINK_RATE = 500                # blink rate in ms
AT1_RAND = 0                        # 1 = random activation
```

### Text-to-Speech (Wit.ai)

```
TTS_WIT_TOKEN = <your_wit_ai_token>
TTS_Voice = Rebecca
TTS_Style = soft
TTS_Speed = 100
TTS_Pitch = 100
TTS_Gain = 100
TTS_SFXChar = neutral
TTS_SFXEnv = neutral
```

### Event scheduling

Events fire once per minute when the time matches. Supports wildcards (`*`).

```
# By date:    HH:MM-DD.MM.YYYY
EVENT_MP3 = 08:00-*.*.*, "morning.mp3"
EVENT_TTS = 12:00-25.12.*, "Merry Christmas"

# By weekday: HH:MM:W  (W: 0=Sunday .. 6=Saturday)
EVENT_MP3 = 09:00:1, "monday_morning.mp3"

# Other event types:
EVENT_BATCH = ...         # run a batch of events
EVENT_DISPLAYS = ...      # turn displays on/off
EVENT_GI_LEDS = ...       # toggle GI LEDs
EVENT_ATTRACT_LEDS = ...  # toggle attract LEDs
EVENT_TIME = ...          # set time
```

## Building and Flashing

Requires **ESP-IDF 5.5.1** targeting **ESP32-S3**.

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

## Boot sequence

1. Splash screen: `LISY` / `CLOCK` / version / `boot`
2. SD card mount → firmware update check → config load
3. LED strip init + attract mode task
4. DS3231 RTC detection
5. WiFi connect → NTP sync → write time to RTC
6. Play `welcome.mp3` (if present on SD card)
7. Main loop: update displays every second, fire events every minute
