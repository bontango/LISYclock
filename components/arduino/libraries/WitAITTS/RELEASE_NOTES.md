# WitAITTS Library v1.0.0 - Release Notes

## Package Contents

```
WitAITTS/
├── src/
│   ├── WitAITTS.h              # Main library header
│   └── WitAITTS.cpp            # Implementation
├── examples/
│   ├── ESP32_Basic/            # ESP32 example
│   ├── ESP32_C3_Basic/         # ESP32-C3 example
│   ├── ESP32_S3_Basic/         # ESP32-S3 example
│   └── PicoW_Basic/            # Pico W / Pico 2 W example
├── README.md                   # Full documentation
├── QUICKSTART.md               # 10-minute setup guide
├── INSTALLATION.md             # Detailed installation
├── CHEATSHEET.md               # Quick reference
├── PIN_CONFIGURATION_GUIDE.md  # Pin wiring guide
├── LICENSE                     # MIT License
├── keywords.txt                # Arduino IDE syntax
├── library.properties          # Arduino metadata
├── library.json                # PlatformIO metadata
└── platformio.ini.example      # PlatformIO config
```

---

## What is WitAITTS?

A professional Arduino library for high-quality text-to-speech using Wit.ai API. 
Features audio streaming, 23+ voices, and support for ESP32 and Raspberry Pi Pico W platforms.

**Perfect for:**
- Voice assistants
- IoT notification systems
- Accessibility devices
- Educational projects
- Smart home automation
- Robotics applications

---

## Key Features

✅ **High-Quality TTS** - Natural-sounding voices from Wit.ai  
✅ **Background Playback** - Non-blocking audio streaming (ESP32)  
✅ **23+ Voices** - Multiple genders, accents, and characters  
✅ **Voice Styles** - default, soft, formal, fast, projected  
✅ **Sound Effects** - Character (robot, alien) & environment (reverb, cathedral)  
✅ **Multi-Platform** - ESP32, ESP32-C3, ESP32-S3, Pico W, Pico 2 W  
✅ **Configurable Debug** - 4 levels (OFF, ERROR, INFO, VERBOSE)  
✅ **Low Memory** - Optimized for embedded systems  
✅ **Open Source** - MIT License  

---

## Platform Support

### ESP32 Family
- ESP32 (original) - Default pins: 27, 26, 25
- ESP32-C3 - Default pins: 7, 6, 5
- ESP32-S3 - Default pins: 16, 17, 15

### Raspberry Pi Pico
- Pico W (RP2040) - Default pins: GP18, GP19, GP20
- Pico 2 W (RP2350) - Default pins: GP18, GP19, GP20

---

## Platform Differences

| Feature | ESP32 | Pico W |
|---------|-------|--------|
| Audio Library | BackgroundAudio | AudioTools |
| Playback Mode | Non-blocking | Blocking |
| `loop()` required | Yes | Optional |
| `speak()` returns | Immediately | After audio |

---

## Technical Specifications

### Audio Support
- **Formats:** MP3
- **Output:** I2S Digital Audio
- **DACs:** MAX98357A, PCM5102, UDA1334, etc.
- **Streaming:** Background playback (ESP32)
- **Buffer:** 32KB default (configurable)

### Memory Requirements
- **Flash:** ~150KB (ESP32), ~140KB (Pico W)
- **RAM:** ~40KB (ESP32), ~20KB (Pico W)

### Network
- **Protocol:** HTTPS (TLS)
- **API:** Wit.ai v20240304
- **WiFi:** 2.4GHz

---

## Dependencies

### For ESP32:
- **BackgroundAudio** by earlephilhower
  - Install via Arduino Library Manager

### For Pico W:
- **arduino-audio-tools** by Phil Schatzmann
  - Manual install from: https://github.com/pschatzmann/arduino-audio-tools

---

## Installation

### Arduino IDE

1. Download `WitAITTS-v1.0.0.zip`
2. Sketch → Include Library → Add .ZIP Library
3. Install platform-specific dependency (see above)

### PlatformIO

**ESP32:**
```ini
lib_deps = 
    jobitjoseph/WitAITTS@^1.0.0
    earlephilhower/BackgroundAudio
```

**Pico W:**
```ini
lib_deps = 
    jobitjoseph/WitAITTS@^1.0.0
    https://github.com/pschatzmann/arduino-audio-tools.git
```

---

## Quick Example

```cpp
#include <WitAITTS.h>

WitAITTS tts;

void setup() {
    Serial.begin(115200);
    tts.begin("WiFi", "password", "WIT_TOKEN");
    tts.setVoice("wit$Remi");
    Serial.println("Type text to speak:");
}

void loop() {
    tts.loop();  // Required for ESP32
    
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();
        if (text.length() > 0) {
            tts.speak(text);
        }
    }
}
```

---

## API Overview

### Initialization
```cpp
WitAITTS tts;                    // Default pins
WitAITTS tts(bclk, lrc, din);    // Custom pins
bool begin(ssid, password, token);
```

### Core Methods
```cpp
bool speak(String text);
void loop();
void stop();
bool isPlaying();
bool isBusy();
```

### Configuration
```cpp
void setVoice(String voice);
void setStyle(String style);
void setSpeed(int speed);        // 0-200
void setPitch(int pitch);        // 0-200
void setGain(float gain);        // 0.0-1.0
void setSFXCharacter(String fx);
void setSFXEnvironment(String fx);
void setDebugLevel(uint8_t lvl); // 0-3
void setPins(bclk, lrc, din);
```

### Status
```cpp
void printConfig();
String getConfig();
void setErrorCallback(callback);
```

---

## Available Voices

### Popular Voices
- `wit$Remi` - Female (default)
- `wit$Cody` - Male
- `wit$Charlie` - Male
- `wit$Railey` - Female

### Character Voices
- `wit$Surfer` - Laid-back male
- `wit$Cartoon Kid` - Playful
- `wit$Pirate` - Arrr!
- `wit$Vampire` - Spooky

See README.md for full list of 23+ voices.

---

## Sound Effects

### Characters
- `none`, `chipmunk`, `monster`, `daemon`, `robot`, `alien`

### Environments
- `none`, `reverb`, `room`, `cathedral`, `radio`, `phone`

---

## Credits

**Author:** Jobit Joseph  
**Organization:** Circuit Digest  
**Website:** circuitdigest.com  

**Dependencies:**
- BackgroundAudio by earlephilhower (ESP32)
- arduino-audio-tools by Phil Schatzmann (Pico W)
- Wit.ai API by Meta

---

## License

MIT License - Copyright © 2025 Jobit Joseph, Circuit Digest

---

**Made with ❤️ for the maker community**
