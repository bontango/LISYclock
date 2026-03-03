# WitAITTS Cheat Sheet

Quick reference for WitAITTS library by Jobit Joseph & Circuit Digest

---

## Basic Template

```cpp
#include <WitAITTS.h>

const char* ssid = "WiFi_SSID";
const char* password = "WiFi_Pass";
const char* witToken = "WIT_TOKEN";

WitAITTS tts;  // Default pins

void setup() {
    Serial.begin(115200);
    tts.begin(ssid, password, witToken);
}

void loop() {
    tts.loop();  // REQUIRED for ESP32!
}
```

---

## Core Methods

| Method | Usage | Example |
|--------|-------|---------|
| `begin()` | Initialize | `tts.begin(ssid, pass, token)` |
| `speak()` | Say text | `tts.speak("Hello")` |
| `loop()` | Process audio | `tts.loop()` |
| `stop()` | Stop speaking | `tts.stop()` |
| `isPlaying()` | Check if playing | `if(tts.isPlaying())` |
| `isBusy()` | Check if busy | `if(tts.isBusy())` |

---

## Configuration Methods

```cpp
tts.setVoice("wit$Remi");        // Change voice
tts.setStyle("soft");             // Change style
tts.setSpeed(150);                // Speed: 0-200
tts.setPitch(120);                // Pitch: 0-200
tts.setGain(0.7);                 // Volume: 0.0-1.0
tts.setSFXCharacter("robot");     // Add effect
tts.setSFXEnvironment("reverb");  // Add reverb
tts.setDebugLevel(DEBUG_INFO);    // Debug: 0-3
tts.setPins(27, 26, 25);          // Custom pins (call before begin)
```

---

## Popular Voices

**Female:**
- `wit$Remi` - Natural (default)
- `wit$Rebecca` - Professional
- `wit$Railey` - Friendly
- `wit$Rubie` - Warm

**Male:**
- `wit$Cody` - Natural
- `wit$Charlie` - Friendly
- `wit$Cooper` - Professional
- `wit$Carl` - Casual

**Character:**
- `wit$Surfer` - Laid-back
- `wit$Cartoon Kid` - Playful
- `wit$Pirate` - Arrr!
- `wit$Vampire` - Spooky

---

## Voice Styles

```cpp
"default"    // Normal speech
"soft"       // Gentle tone
"formal"     // Professional
"fast"       // Quick speech
"projected"  // Loud & clear
```

---

## Sound Effects

**Characters:**
```cpp
"none"       // No effect
"chipmunk"   // High pitch
"robot"      // Mechanical
"monster"    // Deep growl
"daemon"     // Dark voice
"alien"      // Otherworldly
```

**Environments:**
```cpp
"none"       // No effect
"reverb"     // Room reverb
"room"       // Small room
"cathedral"  // Large hall
"radio"      // Radio quality
"phone"      // Telephone
```

---

## Debug Levels

```cpp
DEBUG_OFF      // 0 - Silent
DEBUG_ERROR    // 1 - Errors only
DEBUG_INFO     // 2 - Info (default)
DEBUG_VERBOSE  // 3 - Everything
```

---

## Pin Configurations

### Platform Defaults

| Platform | BCLK | LRC | DIN |
|----------|------|-----|-----|
| ESP32 | 27 | 26 | 25 |
| ESP32-C3 | 7 | 6 | 5 |
| ESP32-S3 | 16 | 17 | 15 |
| Pico W | 18 | 19 | 20 |

### Custom Pins
```cpp
// Via constructor
WitAITTS tts(27, 26, 25);  // BCLK, LRC, DIN

// Or via method (call before begin)
WitAITTS tts;
tts.setPins(14, 15, 32);
tts.begin(ssid, password, token);
```

---

## Common Code Snippets

### Serial Input Processing
```cpp
void loop() {
    tts.loop();
    
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();
        if (text.length() > 0) {
            tts.speak(text);
        }
    }
}
```

### Button Triggered Speech
```cpp
#define BUTTON_PIN 0

void loop() {
    tts.loop();
    
    if (digitalRead(BUTTON_PIN) == LOW) {
        tts.speak("Button pressed!");
        delay(500);
    }
}
```

### Multiple Messages (ESP32)
```cpp
void playMessages() {
    tts.speak("First message");
    while(tts.isBusy()) { tts.loop(); }
    
    tts.speak("Second message");
    while(tts.isBusy()) { tts.loop(); }
}
```

### Error Handling
```cpp
void errorHandler(String error) {
    Serial.println("Error: " + error);
}

void setup() {
    tts.setErrorCallback(errorHandler);
    tts.begin(ssid, password, witToken);
}
```

---

## Platform Differences

| Feature | ESP32 | Pico W |
|---------|-------|--------|
| Audio Library | BackgroundAudio | AudioTools |
| Playback | Non-blocking | Blocking |
| `loop()` required | Yes | Optional |
| `speak()` returns | Immediately | After audio |

---

## Troubleshooting Quick Fixes

| Problem | Quick Fix |
|---------|-----------|
| No sound | `tts.setGain(1.0);` |
| Choppy audio | Auto-fixed (WiFi sleep disabled) |
| HTTP 401 | Check Wit.ai token |
| Short words cut | Already fixed in library |
| Connection fails | Check 2.4GHz WiFi |
| Compilation error | Install correct audio library |

---

## Configuration Constants

Edit in `WitAITTS.h`:

```cpp
#define WITAI_BUFFER_SIZE (32 * 1024)
#define WITAI_MAX_TEXT_LENGTH 280
#define WITAI_NETWORK_BUFFER 2048
```

---

## Memory Usage

**ESP32:**
- Flash: ~150KB
- RAM: ~40KB

**Pico W:**
- Flash: ~140KB
- RAM: ~20KB

**Optimized:**
```cpp
#define WITAI_BUFFER_SIZE (16 * 1024)  // 16KB
```

---

## Full Example

```cpp
#include <WitAITTS.h>

const char* ssid = "MyWiFi";
const char* password = "password";
const char* witToken = "YOUR_TOKEN";

WitAITTS tts;

void errorHandler(String error) {
    Serial.println("Error: " + error);
}

void setup() {
    Serial.begin(115200);
    
    // Set error callback
    tts.setErrorCallback(errorHandler);
    tts.setDebugLevel(DEBUG_INFO);
    
    // Initialize
    if (!tts.begin(ssid, password, witToken)) {
        Serial.println("Failed!");
        while(1);
    }
    
    // Configure voice
    tts.setVoice("wit$Cody");
    tts.setSpeed(120);
    tts.setGain(0.7);
    
    // Print settings
    tts.printConfig();
    
    Serial.println("Type text to speak:");
}

void loop() {
    tts.loop();
    
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

---

## Resources

- 📖 README.md - Full documentation
- 🚀 QUICKSTART.md - 10-minute guide
- 🔧 INSTALLATION.md - Setup help
- 💻 Examples folder - 4 platform examples

## Links

- 🌐 Wit.ai: https://wit.ai
- 📧 Email: jobitjoseph@gmail.com
- 💰 Donate: paypal.me/jobitjoseph
- 🐙 GitHub: github.com/jobitjoseph

---

**Made with ❤️ by Jobit Joseph & Circuit Digest**

*Version 1.0.0 - 2025*
