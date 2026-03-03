# WitAITTS Quick Start Guide

## 1. Installation (2 minutes)

### Arduino IDE:
```
1. Download WitAITTS.zip
2. Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Install dependencies:

   For ESP32:
   - Library Manager → Search "BackgroundAudio" → Install

   For Pico W:
   - Download ZIP from: https://github.com/pschatzmann/arduino-audio-tools
   - Arduino IDE → Sketch → Include Library → Add .ZIP Library
```

### PlatformIO:

**For ESP32:**
```ini
lib_deps = 
    jobitjoseph/WitAITTS@^1.0.0
    earlephilhower/BackgroundAudio
```

**For Pico W:**
```ini
lib_deps = 
    jobitjoseph/WitAITTS@^1.0.0
    https://github.com/pschatzmann/arduino-audio-tools
```

## 2. Hardware Setup (5 minutes)

### ESP32 + MAX98357A:
```
ESP32 GPIO27 → MAX98357A BCLK
ESP32 GPIO26 → MAX98357A LRC  
ESP32 GPIO25 → MAX98357A DIN
MAX98357A VIN → 5V
MAX98357A GND → GND
Speaker → MAX98357A Speaker Terminals
```

### ESP32-C3 + MAX98357A:
```
ESP32-C3 GPIO7 → MAX98357A BCLK
ESP32-C3 GPIO6 → MAX98357A LRC  
ESP32-C3 GPIO5 → MAX98357A DIN
```

### ESP32-S3 + MAX98357A:
```
ESP32-S3 GPIO16 → MAX98357A BCLK
ESP32-S3 GPIO17 → MAX98357A LRC  
ESP32-S3 GPIO15 → MAX98357A DIN
```

### Pico W + I2S DAC:
```
Pico GP18 → DAC BCLK
Pico GP19 → DAC LRC/LRCLK
Pico GP20 → DAC DIN/DATA
```

## 3. Get Wit.ai Token (2 minutes)
```
1. Visit https://wit.ai
2. Login/Create Account
3. Create New App
4. Settings → Copy Token
```

## 4. First Program (1 minute)

```cpp
#include <WitAITTS.h>

const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* witToken = "YOUR_TOKEN";

WitAITTS tts;

void setup() {
    Serial.begin(115200);
    tts.begin(ssid, password, witToken);
    
    // Configure voice (optional)
    tts.setVoice("wit$Remi");
    tts.setGain(0.5);
    
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

## 5. Upload & Test
```
1. Select Board: Tools → Board → ESP32 Dev Module (or your board)
2. Select Port: Tools → Port → (your port)
3. Upload
4. Open Serial Monitor (115200 baud)
5. Type any text and press Enter
6. Listen! 🔊
```

---

## Available Voices (Quick Reference)

**Popular Voices:**
- `wit$Remi` - Female (default)
- `wit$Cody` - Male
- `wit$Charlie` - Male
- `wit$Railey` - Female

**Fun Voices:**
- `wit$Surfer` - Laid-back male
- `wit$Cartoon Kid` - Playful female
- `wit$Pirate` - Arrr matey!
- `wit$Vampire` - Spooky

## Configuration Options

```cpp
tts.setVoice("wit$Cody");        // Change voice
tts.setStyle("soft");             // Style: default, soft, formal, fast, projected
tts.setSpeed(150);                // Speed: 0-200
tts.setPitch(120);                // Pitch: 0-200
tts.setGain(0.7);                 // Volume: 0.0-1.0
tts.setSFXCharacter("robot");     // Effect: none, chipmunk, robot, monster, alien
tts.setSFXEnvironment("reverb");  // Reverb: none, reverb, room, cathedral
```

## Common Issues

**No Sound?**
- Check connections
- Try: `tts.setGain(1.0);`
- Verify WiFi & token

**Choppy Audio?**
- WiFi sleep is auto-disabled for ESP32
- Move closer to router

---

**Full docs:** See README.md  
**Support:** jobitjoseph@gmail.com  
**Donate:** paypal.me/jobitjoseph

Made by Jobit Joseph & Circuit Digest
