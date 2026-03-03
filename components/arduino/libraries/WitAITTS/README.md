# 🎤 WitAITTS - Wit.ai Text-to-Speech Library

### High-quality text-to-speech streaming for ESP32, ESP32-S3, ESP32-C3 and Raspberry Pi Pico W

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-blue.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.0-green.svg)](https://github.com/jobitjoseph/WitAITTS/releases)

**Created by [Jobit Joseph](https://github.com/jobitjoseph) & [Circuit Digest](https://circuitdigest.com)**

---

## ✨ Features

- 🎤 **High-Quality TTS** - Stream natural-sounding speech from Wit.ai
- 🔊 **Background Playback** - Non-blocking audio streaming (ESP32)
- 🎭 **23+ Voices** - Multiple genders, accents, and character voices
- 🎚️ **Full Control** - Speed, pitch, gain, and effects
- 🎵 **Sound Effects** - Character effects (robot, alien) and environments (reverb, cathedral)
- 🐛 **Debug Levels** - 4 configurable levels (OFF to VERBOSE)
- 🔌 **Multi-Platform** - ESP32, ESP32-C3, ESP32-S3, Pico W, Pico 2 W
- 📦 **Easy Integration** - Simple API, comprehensive docs

---

## 🛠️ Supported Platforms

| Platform | Audio Library | Playback Mode | Default Pins (BCLK/LRC/DIN) |
|----------|---------------|---------------|------------------------------|
| ESP32    | BackgroundAudio | Non-blocking | 27 / 26 / 25 |
| ESP32-C3 | BackgroundAudio | Non-blocking | 7 / 6 / 5 |
| ESP32-S3 | BackgroundAudio | Non-blocking | 16 / 17 / 15 |
| Pico W   | AudioTools      | Blocking     | 18 / 19 / 20 |
| Pico 2 W | AudioTools      | Blocking     | 18 / 19 / 20 |

---

## 🔧 Hardware Requirements

### ESP32 Family
- ESP32 / ESP32-C3 / ESP32-S3 Dev Board
- MAX98357A I2S Amplifier or compatible DAC
- Speaker (4-8Ω, 3W recommended)
- USB cable for programming

### Raspberry Pi Pico W / Pico 2 W
- Raspberry Pi Pico W or Pico 2 W
- I2S Audio DAC (PCM5102, UDA1334, MAX98357A, etc.)
- Speaker
- USB cable

### Wiring

**ESP32:**
```
ESP32 GPIO27 → MAX98357A BCLK
ESP32 GPIO26 → MAX98357A LRC
ESP32 GPIO25 → MAX98357A DIN
MAX98357A VIN → 5V
MAX98357A GND → GND
MAX98357A    → Speaker (4-8Ω)
```

**Pico W:**
```
Pico GP18 → DAC BCLK
Pico GP19 → DAC LRC
Pico GP20 → DAC DIN
DAC VIN   → 3.3V or 5V
DAC GND   → GND
DAC       → Speaker (4-8Ω)
```

---

## 📦 Installation

### Arduino IDE
#### Library Manager Installation
- Search for WitAITTS in the Arduino IDE library manager and install.
#### Manual Installation
1. Download this repository as ZIP
2. In Arduino IDE: `Sketch` → `Include Library` → `Add .ZIP Library`
3. Install dependencies based on your platform:

**For ESP32:**
- Open Library Manager: `Tools` → `Manage Libraries`
- Search "**BackgroundAudio**" and **ESP32-audioI2S-master** → Install

**For Pico W:**
- Download ZIP from: https://github.com/pschatzmann/arduino-audio-tools and https://github.com/pschatzmann/arduino-libhelix
- Install via: `Sketch` → `Include Library` → `Add .ZIP Library`


### PlatformIO

Add to `platformio.ini`:

**For ESP32:**
```ini
lib_deps =
    jobitjoseph/WitAITTS@^1.0.0
    earlephilhower/BackgroundAudio
    schreibfaul1/ESP32-audioI2S-master
```

**For Pico W:**
```ini
lib_deps =
    jobitjoseph/WitAITTS@^1.0.0
    https://github.com/pschatzmann/arduino-audio-tools
    https://github.com/pschatzmann/arduino-libhelix
```

---

## 🚀 Quick Start

### 1. Get Wit.ai Token

1. Visit [wit.ai](https://wit.ai)
2. Create account / Login
3. Create new app
4. Go to Settings → Copy your **Server Access Token**

### 2. Basic Usage

```cpp
#include <WitAITTS.h>

const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* witToken = "YOUR_TOKEN_HERE";

WitAITTS tts;  // Uses default pins for your platform

void setup() {
    Serial.begin(115200);
    
    // Initialize
    tts.begin(ssid, password, witToken);
    
    // Optional configuration
    tts.setVoice("wit$Remi");
    tts.setGain(0.5);
    
    Serial.println("Type text to speak:");
}

void loop() {
    tts.loop();  // Required for ESP32 streaming
    
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();
        if (text.length() > 0) {
            tts.speak(text);
        }
    }
}
```

**That's it! 🎉**

### 3. Custom Pins

```cpp
// Via constructor
WitAITTS tts(27, 26, 25);  // BCLK, LRC, DIN

// Or via method (call before begin)
WitAITTS tts;
tts.setPins(27, 26, 25);
tts.begin(ssid, password, token);
```

---

## 📖 API Reference

### Initialization
```cpp
// Constructor with custom pins
WitAITTS tts(bclkPin, lrcPin, dinPin);

// Or use defaults
WitAITTS tts;

// Initialize
bool begin(ssid, password, witToken);
```

### Core Methods
```cpp
bool speak(String text);          // Speak text (max 280 chars)
void stop();                       // Stop current playback
void loop();                       // Must call in loop() for ESP32
bool isPlaying();                  // Check if playing
bool isBusy();                     // Check if busy (streaming/playing)
```

### Configuration
```cpp
void setVoice(String voice);       // wit$Remi, wit$Cody, etc.
void setStyle(String style);       // default, soft, formal, fast, projected
void setSpeed(int speed);          // 0-200, default 100
void setPitch(int pitch);          // 0-200, default 100
void setSFXCharacter(String fx);   // none, chipmunk, monster, robot, alien, daemon
void setSFXEnvironment(String fx); // none, reverb, room, cathedral, radio, phone
void setGain(float gain);          // 0.0-1.0, default 0.5
void setAudioFormat(String fmt);   // "audio/mpeg" or "audio/pcm16"
void setDebugLevel(uint8_t lvl);   // 0=OFF, 1=ERROR, 2=INFO, 3=VERBOSE
void setPins(bclk, lrc, din);      // Set I2S pins (call before begin)
```

### Status & Debug
```cpp
void printConfig();                // Print current settings
String getConfig();                // Get settings as string
void setErrorCallback(callback);   // Set error handler
```

---

## 🎭 Available Voices

### US English Female
| Voice | Description | Styles |
|-------|-------------|--------|
| `wit$Remi` | Natural female (default) | default, soft, formal, fast, projected |
| `wit$Rebecca` | Professional female | default, soft, formal, fast, projected |
| `wit$Railey` | Friendly female | default, soft, formal, fast, projected |
| `wit$Rubie` | Warm female | default, soft, formal, fast, projected |
| `wit$Disaffected` | Monotone female | default, soft, formal, fast, projected |
| `wit$Cartoon Kid` | Playful child | default, soft, formal, fast, projected |
| `wit$Cartoon Villain` | Evil female | default, soft, formal, fast, projected |
| `wit$Kenyan Accent` | Kenyan English | default, soft, formal, fast, projected |

### US English Male
| Voice | Description | Styles |
|-------|-------------|--------|
| `wit$Cody` | Natural male | default, soft, formal, fast, projected |
| `wit$Charlie` | Friendly male | default, soft, fast, projected |
| `wit$Cooper` | Professional male | default, soft, formal, fast, projected |
| `wit$Carl` | Casual male | default, soft, formal, fast, projected |
| `wit$Surfer` | Laid-back male | default, soft, formal, fast, projected |
| `wit$Vampire` | Spooky male | default, soft, formal, fast, projected |
| `wit$Prospector` | Old western male | default, soft, formal, fast, projected |
| `wit$Southern Accent` | US Southern male | default, soft, formal, fast, projected |

### UK English
| Voice | Description |
|-------|-------------|
| `wit$Pirate` | Pirate male |
| `wit$Wizard` | Fantasy male |
| `wit$British Butler` | Formal butler |
| `wit$Cockney Accent` | Cockney nonbinary |

### Canadian English
| Voice | Description |
|-------|-------------|
| `wit$Rosie` | Canadian female |
| `wit$Colin` | Canadian male |

### Special
| Voice | Description |
|-------|-------------|
| `wit$Cartoon Baby` | Baby voice (nonbinary) |

---

## 🎵 Sound Effects

### SFX Characters
Transform the voice with character effects:

| Effect | Description |
|--------|-------------|
| `none` | No effect (default) |
| `chipmunk` | High-pitched squeaky voice |
| `monster` | Deep, growly voice |
| `daemon` | Dark, demonic voice |
| `robot` | Mechanical robotic voice |
| `alien` | Otherworldly effect |

### SFX Environments
Add acoustic environment effects:

| Effect | Description |
|--------|-------------|
| `none` | No effect (default) |
| `reverb` | Standard reverb |
| `room` | Small room acoustics |
| `cathedral` | Large hall echo |
| `radio` | Radio/telephone quality |
| `phone` | Phone call simulation |

### Example
```cpp
tts.setSFXCharacter("robot");
tts.setSFXEnvironment("cathedral");
tts.speak("I am a robot in a cathedral!");
```

---

## 🐛 Debug Levels

```cpp
DEBUG_OFF      // 0 - No output
DEBUG_ERROR    // 1 - Errors only
DEBUG_INFO     // 2 - Info + errors (default)
DEBUG_VERBOSE  // 3 - All messages
```

Set via code:
```cpp
tts.setDebugLevel(DEBUG_VERBOSE);
```

---

## 📂 Examples

Four examples included for different platforms:

| Example | Platform | Default Pins |
|---------|----------|--------------|
| `ESP32_Basic` | ESP32 | 27, 26, 25 |
| `ESP32_C3_Basic` | ESP32-C3 | 7, 6, 5 |
| `ESP32_S3_Basic` | ESP32-S3 | 16, 17, 15 |
| `PicoW_Basic` | Pico W / Pico 2 W | 18, 19, 20 |

---

## 📊 Platform Differences

| Feature | ESP32 | Pico W |
|---------|-------|--------|
| Audio Library | BackgroundAudio | AudioTools |
| Playback | Non-blocking | Blocking |
| `loop()` required | Yes | Optional |
| `speak()` returns | Immediately | After playback |

---

## 🔍 Troubleshooting

### No Sound
1. Check I2S wiring connections
2. Verify speaker is connected
3. Increase gain: `tts.setGain(1.0)`
4. Check WiFi connection
5. Verify Wit.ai token is valid

### Choppy Audio (ESP32)
1. Reduce WiFi interference
2. WiFi sleep is auto-disabled
3. CPU is auto-set to 240MHz
4. Increase buffer size in WitAITTS.h

### WiFi Connection Failed
1. Verify SSID/password
2. Use 2.4GHz network (5GHz not supported)
3. Move closer to router
4. Check serial monitor for errors

### HTTP 401 Error
- Invalid Wit.ai token
- Get new token from wit.ai Settings

---

## 💾 Memory Usage

| Platform | Flash | RAM |
|----------|-------|-----|
| ESP32 | ~150KB | ~40KB (with 32KB buffer) |
| Pico W | ~140KB | ~20KB |

---

## 📄 License

MIT License - See [LICENSE](LICENSE) file

Copyright © 2025 Jobit Joseph, Circuit Digest

---

## 👨‍💻 Author

**Jobit Joseph**
- GitHub: [@jobitjoseph](https://github.com/jobitjoseph)
- Website: [Circuit Digest](https://circuitdigest.com)

---

## 🙏 Credits

**Dependencies:**
- [BackgroundAudio](https://github.com/earlephilhower/BackgroundAudio) by earlephilhower (ESP32)
- [ESP32-audioI2S-master](https://github.com/schreibfaul1/ESP32-audioI2S) by schreibfaul1 (ESP32)
- [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools) by Phil Schatzmann (Pico W)
- [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) by Phil Schatzmann (Pico W)

**API:**
- [Wit.ai](https://wit.ai) by Meta

---

## 💖 Support

If this library helped your project:
- ⭐ Star this repository
- 📢 Share with others
- ☕ [Buy me a coffee](https://paypal.me/jobitjoseph)
- 🐛 [Report issues](https://github.com/jobitjoseph/WitAITTS/issues)
- 💡 Suggest features

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [QUICKSTART.md](QUICKSTART.md) | 10-minute setup guide |
| [INSTALLATION.md](INSTALLATION.md) | Detailed installation instructions |
| [CHEATSHEET.md](CHEATSHEET.md) | Quick reference card |
| [PIN_CONFIGURATION_GUIDE.md](PIN_CONFIGURATION_GUIDE.md) | Pin wiring guide |

---

<div align="center">

**Made with ❤️ for the maker community**

[⬆ Back to top](#-witaitts---witai-text-to-speech-library)

</div>
