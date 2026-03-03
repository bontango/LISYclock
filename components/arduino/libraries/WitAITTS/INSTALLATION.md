# WitAITTS Installation & Testing Guide

## Complete Installation Steps

### For Arduino IDE

#### Step 1: Install WitAITTS Library
1. Search for WitAITTS in library manager and install.

 or
   
1. Download `WitAITTS-v1.0.0.zip`
2. Open Arduino IDE
3. Go to: **Sketch → Include Library → Add .ZIP Library...**
4. Select the downloaded ZIP file
5. Wait for "Library installed" message

#### Step 2: Install Dependencies

**For ESP32 / ESP32-C3 / ESP32-S3:**
1. Go to: **Tools → Manage Libraries...**
2. Search for "**BackgroundAudio**" and **ESP32-audioI2S-master** → Click Install
3. Close Library Manager

**For Pico W / Pico 2 W:**
1. Download ZIP from: https://github.com/pschatzmann/arduino-audio-tools and https://github.com/pschatzmann/arduino-libhelix
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library...**
3. Select the downloaded ZIP file

#### Step 3: Board Support (if not already installed)

**For ESP32:**
1. **File → Preferences**
2. Add to "Additional Board Manager URLs":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Tools → Board → Board Manager**
4. Search "esp32" → Install "esp32 by Espressif"

**For Pico W:**
1. **File → Preferences**
2. Add to "Additional Board Manager URLs":
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. **Tools → Board → Board Manager**
4. Search "pico" → Install "Raspberry Pi Pico/RP2040"

---

### For PlatformIO

#### For ESP32:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    https://github.com/jobitjoseph/WitAITTS.git
    earlephilhower/BackgroundAudio
    schreibfaul1/ESP32-audioI2S-master
```

#### For ESP32-C3:
```ini
[env:esp32c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200

lib_deps = 
    https://github.com/jobitjoseph/WitAITTS.git
    earlephilhower/BackgroundAudio
    schreibfaul1/ESP32-audioI2S-master
```

#### For ESP32-S3:
```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

lib_deps = 
    https://github.com/jobitjoseph/WitAITTS.git
    earlephilhower/BackgroundAudio
    schreibfaul1/ESP32-audioI2S-master
```

#### For Pico W:
```ini
[env:rpipicow]
platform = raspberrypi
board = rpipicow
framework = arduino
monitor_speed = 115200

lib_deps = 
    https://github.com/jobitjoseph/WitAITTS.git
    https://github.com/pschatzmann/arduino-audio-tools
    https://github.com/pschatzmann/arduino-libhelix
```

Build with:
```bash
pio run
```

PlatformIO will automatically download all dependencies!

---

## Hardware Wiring

### ESP32 + MAX98357A I2S Amplifier

```
┌─────────────┐         ┌──────────────┐
│    ESP32    │         │  MAX98357A   │
├─────────────┤         ├──────────────┤
│ GPIO27      ├────────►│ BCLK         │
│ GPIO26      ├────────►│ LRC (WS)     │
│ GPIO25      ├────────►│ DIN (SD)     │
│ 5V          ├────────►│ VIN          │
│ GND         ├────────►│ GND          │
└─────────────┘         └──────┬───────┘
                               │
                               ▼
                        ┌──────────────┐
                        │   Speaker    │
                        │  4-8Ω, 3W    │
                        └──────────────┘
```

### ESP32-C3 + MAX98357A

```
┌─────────────┐         ┌──────────────┐
│  ESP32-C3   │         │  MAX98357A   │
├─────────────┤         ├──────────────┤
│ GPIO7       ├────────►│ BCLK         │
│ GPIO6       ├────────►│ LRC (WS)     │
│ GPIO5       ├────────►│ DIN (SD)     │
│ 5V          ├────────►│ VIN          │
│ GND         ├────────►│ GND          │
└─────────────┘         └──────────────┘
```

### ESP32-S3 + MAX98357A

```
┌─────────────┐         ┌──────────────┐
│  ESP32-S3   │         │  MAX98357A   │
├─────────────┤         ├──────────────┤
│ GPIO16      ├────────►│ BCLK         │
│ GPIO17      ├────────►│ LRC (WS)     │
│ GPIO15      ├────────►│ DIN (SD)     │
│ 5V          ├────────►│ VIN          │
│ GND         ├────────►│ GND          │
└─────────────┘         └──────────────┘
```

### Pico W + I2S DAC

```
┌─────────────┐         ┌──────────────┐
│   Pico W    │         │   I2S DAC    │
├─────────────┤         ├──────────────┤
│ GP18        ├────────►│ BCLK/BCK     │
│ GP19        ├────────►│ LRCLK/WS     │
│ GP20        ├────────►│ DIN/DATA     │
│ 3.3V/5V     ├────────►│ VIN          │
│ GND         ├────────►│ GND          │
└─────────────┘         └──────────────┘
```

**Notes:**
- MAX98357A has built-in DAC and amplifier
- No potentiometer needed (software gain control)
- Connect GAIN pin to VIN for 9dB gain (optional)
- For Pico W, check your DAC's voltage requirements

---

## Get Wit.ai API Token

### Step 1: Create Account
1. Visit [https://wit.ai](https://wit.ai)
2. Click "Get Started" or "Login"
3. Sign in with Facebook/GitHub or create new account

### Step 2: Create App
1. Click "New App"
2. Enter name (e.g., "ESP32_TTS")
3. Select language: **English**
4. Click "Create"

### Step 3: Get Token
1. Click "Settings" (gear icon)
2. Find "Server Access Token"
3. Click "Show" and copy the token
4. Keep this token secret!

Example token format: `F4OCFXFEFU24XDXMY3HD2KOY5J2FZPTV`

---

## Testing Your Setup

### Test: Basic Speech

```cpp
#include <WitAITTS.h>

const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* witToken = "YOUR_TOKEN_HERE";

WitAITTS tts;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Starting WitAITTS Test...");
    
    if (tts.begin(ssid, password, witToken)) {
        Serial.println("TTS Ready!");
        
        // Configure voice
        tts.setVoice("wit$Remi");
        tts.setGain(0.5);
        
        Serial.println("Type any text to speak:");
    } else {
        Serial.println("TTS Failed!");
    }
}

void loop() {
    tts.loop();  // Required for ESP32
    
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();
        if (text.length() > 0) {
            Serial.println("Speaking: " + text);
            tts.speak(text);
        }
    }
}
```

**Expected Output:**
```
Starting WitAITTS Test...
[INFO] WitAITTS Initializing...
[INFO] Connecting to WiFi...
[INFO] WiFi Connected: 192.168.1.100
[INFO] WitAITTS Ready
TTS Ready!
Type any text to speak:
```

---

## Troubleshooting

### Problem: Compilation Error - "WitAITTS.h not found"
**Solution:** 
- Restart Arduino IDE
- Verify library installed: Sketch → Include Library → (WitAITTS should be listed)
- Check library path: Documents/Arduino/libraries/WitAITTS

### Problem: "BackgroundAudio.h not found" (ESP32)
**Solution:**
- Install BackgroundAudio: Tools → Manage Libraries
- Search "BackgroundAudio" → Install

### Problem: "AudioTools.h not found" (Pico W)
**Solution:**
- Download from: https://github.com/pschatzmann/arduino-audio-tools
- Install via: Sketch → Include Library → Add .ZIP Library

### Problem: "WiFi Connected" but no sound
**Solution:**
1. Check hardware connections
2. Try: `tts.setGain(1.0);`
3. Test speaker with multimeter
4. Verify MAX98357A power LED is on

### Problem: "HTTP Error: 401"
**Solution:**
- Invalid Wit.ai token
- Get new token from wit.ai Settings
- Make sure no extra spaces in token

### Problem: "HTTP Error: -1" or "Connection failed"
**Solution:**
- WiFi issue
- Check SSID/password
- Try 2.4GHz network (not 5GHz)
- Move closer to router

### Problem: Choppy/stuttering audio (ESP32)
**Solution:**
1. WiFi sleep is auto-disabled
2. Increase buffer: Edit WITAI_BUFFER_SIZE in WitAITTS.h
3. Reduce WiFi interference
4. CPU is auto-set to 240MHz

### Problem: Short words don't play
**Solution:**
- Library already includes fix
- Make sure `tts.loop()` is called continuously (ESP32)
- Try: `tts.setDebugLevel(DEBUG_VERBOSE);` to see what's happening

### Problem: Serial Monitor shows garbage
**Solution:**
- Set baud rate to 115200
- Check: Tools → Port (correct port selected)
- Reset board after opening Serial Monitor

---

## Memory Optimization

### For ESP32 with Limited RAM:

Edit `WitAITTS.h`:
```cpp
#define WITAI_BUFFER_SIZE (16 * 1024)      // Reduce from 32KB
#define WITAI_BUFFER_START_LEVEL (8 * 1024)  // Reduce proportionally
```

### For Large Projects:

Disable features in platformio.ini:
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -DBOARD_HAS_PSRAM
```

---

## Default Pin Reference

| Platform | BCLK | LRC | DIN |
|----------|------|-----|-----|
| ESP32 | 27 | 26 | 25 |
| ESP32-C3 | 7 | 6 | 5 |
| ESP32-S3 | 16 | 17 | 15 |
| Pico W | 18 | 19 | 20 |

All pins are configurable via:
```cpp
WitAITTS tts(bclk, lrc, din);
// or
tts.setPins(bclk, lrc, din);  // Before begin()
```

---

## Next Steps

1. ✅ Test basic speech
2. ✅ Try different voices
3. ✅ Experiment with sound effects
4. ✅ Integrate into your project

## Support

- 📖 **Full Docs:** README.md
- 🐛 **Issues:** GitHub Issues
- 📧 **Email:** jobitjoseph@gmail.com
- 💬 **Community:** Circuit Digest Forum

---

**Happy Making! 🎉**

*Jobit Joseph & Circuit Digest*
