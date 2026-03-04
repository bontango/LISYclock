# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LISYclock is an ESP32-S3 firmware for a pinball/arcade-themed clock. It drives 4× TM1637 6-digit LED displays, a programmable LED strip, audio playback (MP3 + TTS via Wit.ai), and syncs time via NTP and a DS3231 RTC. Configuration and audio files are stored on SD card.

## Build System

**Framework:** ESP-IDF 5.5.1 with CMake + Ninja. All builds are done through ESP-IDF toolchain.

```bash
# Configure (first time or after sdkconfig changes)
idf.py set-target esp32s3

# Build
idf.py build

# Flash
idf.py -p <PORT> flash

# Monitor serial output
idf.py -p <PORT> monitor

# Flash + monitor combined
idf.py -p <PORT> flash monitor
```

There are no unit tests in this project.

## Hardware Versions

The hardware version is selected at compile time by commenting/uncommenting `#define LISYCLOCK2` in [main/gpiodefs.h](main/gpiodefs.h):

- **HW v1.xx** (default, `LISYCLOCK2` not defined) — version string `v1.36`
- **HW v2.xx** (`#define LISYCLOCK2 TRUE`) — version string `v2.36`

All GPIO assignments for both versions are in [main/gpiodefs.h](main/gpiodefs.h).

## Architecture

### Main application flow (`main/lisyclock.cpp`)
`app_main()` initializes all subsystems in order: NVS, SD card, config, GPIO, I2C/RTC, displays, LEDs, buttons, WiFi, SNTP, audio, FTP server, then enters the main loop which updates the display and fires scheduled events.

### Key source files

| File | Responsibility |
|------|---------------|
| `main/lisyclock.cpp` | Entry point, display rendering, main loop |
| `main/event.cpp` / `event.h` | Time-based event scheduling (MP3, TTS, LED patterns) |
| `main/audio.cpp` / `audio.h` | MP3 playback from SD + TTS via Wit.ai REST API |
| `main/leds.c` / `leds.h` | RMT-driven LED strip (up to 31 LEDs), GI and attract modes |
| `main/config.c` / `config.h` | Parses `config.txt` from SD card |
| `main/ds3231.c` / `ds3231.h` | I2C driver for DS3231 RTC |
| `main/buttons.c` | ADJUST/SET buttons + DIP switch reading |
| `main/ftp.c` | FTP server for SD card access over WiFi |
| `main/sdcard.c` | SPI SD card mount |
| `main/sntp.c` | NTP time sync |
| `main/fupdate.c` | OTA firmware update from SD card |
| `main/typedefs.h` | Shared structs (LED config, event types) |
| `main/gpiodefs.h` | All GPIO pin assignments (both HW versions) |

### Components and dependencies

- `components/arduino/` — arduino-esp32 framework + added libraries:
  - `libraries/TM1637TinyDisplay/` — display driver
  - `libraries/BackgroundAudio/` — audio mixing
  - `libraries/ESP32-audioI2S/` — I2S audio
  - `libraries/WitAITTS/` — Wit.ai Text-to-Speech
- `managed_components/` — ESP-IDF managed dependencies (wifi-manager, led_strip, button, mp3 decoder, etc.)

### Partition layout

Two OTA partitions (`partitions_two_ota.csv`). OTA firmware updates are loaded from SD card via `fupdate.c`.

### Configuration

Runtime config is read from `config.txt` on the SD card at boot. Timezone and other compile-time settings are in `sdkconfig`.

## Koordination mit dem Partnerprojekt

Der Config Editor befindet sich in `../config_editor/` (relativ zu diesem Verzeichnis).

**API-Vertrag:** [`../API.md`](../API.md) ist die Single Source of Truth für alle HTTP-Endpunkte zwischen Firmware und Editor.

**Regeln:**
- Wenn du einen Endpunkt in `main/httpserver.c` änderst oder hinzufügst, muss `../API.md` aktualisiert werden.
- Bei **Breaking Changes** (geänderte Response-Felder, entfernte Endpunkte): `HTTP_API_VERSION` in `main/httpserver.h` erhöhen **und** `api_version` in `../API.md` erhöhen.
- Nicht-breaking Additions (neue optionale Felder, neue Endpunkte) erfordern keine Versionserhöhung.
- Nach API-Änderungen muss der Config Editor (`../config_editor/index.html`) angepasst werden.
