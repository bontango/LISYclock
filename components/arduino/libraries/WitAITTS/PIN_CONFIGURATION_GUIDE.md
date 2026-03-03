# WitAITTS Pin Configuration Guide

## Default Pins by Platform

| Platform | BCLK | LRC | DIN | Notes |
|----------|------|-----|-----|-------|
| **ESP32** | GPIO 27 | GPIO 26 | GPIO 25 | Original ESP32 |
| **ESP32-C3** | GPIO 7 | GPIO 6 | GPIO 5 | Limited GPIOs (0-21) |
| **ESP32-S3** | GPIO 16 | GPIO 17 | GPIO 15 | Many GPIOs available |
| **Pico W** | GP18 | GP19 | GP20 | Pico W / Pico 2 W |

---

## ESP32 Pin Configuration

### Method 1: Constructor (Recommended)

**Default pins:**
```cpp
#include <WitAITTS.h>

// Uses platform-specific default pins automatically
WitAITTS tts;

void setup() {
    tts.begin("WiFi", "password", "token");
}
```

**Custom pins:**
```cpp
#include <WitAITTS.h>

// Custom pins: BCLK=14, LRC=15, DIN=32
WitAITTS tts(14, 15, 32);

void setup() {
    tts.begin("WiFi", "password", "token");
}
```

### Method 2: setPins() Method

Call `setPins()` before `begin()`:

```cpp
#include <WitAITTS.h>

WitAITTS tts;

void setup() {
    // Set custom pins before begin()
    tts.setPins(14, 15, 32);
    
    tts.begin("WiFi", "password", "token");
    tts.speak("Using custom pins");
}

void loop() {
    tts.loop();
}
```

---

## Pico W Pin Configuration

### Method 1: Constructor (Recommended)

**Default pins (GP18, GP19, GP20):**
```cpp
#include <WitAITTS.h>

WitAITTS tts;  // Uses GP18, GP19, GP20

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    tts.begin("WiFi", "password", "token");
}

void loop() {
    tts.loop();
}
```

**Custom pins:**
```cpp
#include <WitAITTS.h>

// Custom pins: BCLK=0, LRC=1, DIN=2
WitAITTS tts(0, 1, 2);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    tts.begin("WiFi", "password", "token");
}

void loop() {
    tts.loop();
}
```

### Method 2: setPins() Method

```cpp
#include <WitAITTS.h>

WitAITTS tts;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    // Set custom pins before begin()
    tts.setPins(0, 1, 2);
    
    tts.begin("WiFi", "password", "token");
}

void loop() {
    tts.loop();
}
```

---

## Wiring Examples

### ESP32 + MAX98357A

```
ESP32 GPIO27 → MAX98357A BCLK
ESP32 GPIO26 → MAX98357A LRC (or WS)
ESP32 GPIO25 → MAX98357A DIN (or SD)
ESP32 5V     → MAX98357A VIN
ESP32 GND    → MAX98357A GND
MAX98357A    → Speaker (4-8Ω)
```

### ESP32-C3 + MAX98357A

```
ESP32-C3 GPIO7 → MAX98357A BCLK
ESP32-C3 GPIO6 → MAX98357A LRC
ESP32-C3 GPIO5 → MAX98357A DIN
ESP32-C3 5V    → MAX98357A VIN
ESP32-C3 GND   → MAX98357A GND
```

### ESP32-S3 + MAX98357A

```
ESP32-S3 GPIO16 → MAX98357A BCLK
ESP32-S3 GPIO17 → MAX98357A LRC
ESP32-S3 GPIO15 → MAX98357A DIN
ESP32-S3 5V     → MAX98357A VIN
ESP32-S3 GND    → MAX98357A GND
```

### Pico W + PCM5102 DAC

```
Pico GP18  → PCM5102 BCK (Bit Clock)
Pico GP19  → PCM5102 LRCK (LR Clock)
Pico GP20  → PCM5102 DIN (Data)
Pico 3.3V  → PCM5102 VIN
Pico GND   → PCM5102 GND & SCK (tie SCK to GND)
PCM5102    → Speaker/Amplifier
```

---

## Pin Recommendations

### ESP32 - Good Pin Choices

**Recommended:**
- GPIO 25, 26, 27 (default - well tested)
- GPIO 12, 13, 14, 15
- GPIO 16, 17, 18, 19
- GPIO 21, 22, 23
- GPIO 32, 33

**Avoid:**
- GPIO 0, 2, 5, 12, 15 (strapping pins)
- GPIO 6-11 (flash - will crash!)
- GPIO 34-39 (input only)

### ESP32-C3 - Good Pin Choices

**Recommended:**
- GPIO 5, 6, 7 (default)
- GPIO 0, 1, 2, 3, 4
- GPIO 8, 9, 10

**Avoid:**
- GPIO 11-17 (some reserved)
- GPIO 18-21 (USB/JTAG)

### ESP32-S3 - Good Pin Choices

**Recommended:**
- GPIO 15, 16, 17 (default)
- GPIO 1-14
- GPIO 35-48

**Avoid:**
- GPIO 0 (strapping pin)
- GPIO 19-20 (USB)
- GPIO 26-32 (SPI flash)

### Pico W - Good Pin Choices

**Recommended:**
- GP18, GP19, GP20 (default)
- GP0, GP1, GP2
- GP10, GP11, GP12
- Any GP0-22

**Avoid:**
- GP23-25 (WiFi chip control)
- GP29 (ADC reference)

---

## Troubleshooting

### No Audio Output

1. **Check wiring** - Verify all connections
2. **Increase gain:** `tts.setGain(1.0);`
3. **Check pins:** `tts.printConfig();`
4. **Verbose debug:** `tts.setDebugLevel(DEBUG_VERBOSE);`

### Garbled Audio (ESP32)

**Possible causes:**
- Wrong pin order (BCLK/LRC/DIN swapped)
- Using input-only pins (34-39)
- Using flash pins (6-11)

**Solution:**
```cpp
// Try swapping pins
tts.setPins(26, 27, 25);  // Different order
```

### Pico W - No Sound

1. Verify I2S DAC connections
2. Check DAC power (3.3V and GND)
3. Verify correct pins in constructor
4. Check AudioTools library installed

---

## Complete Examples

### ESP32 - Custom Pins

```cpp
#include <WitAITTS.h>

// Custom pins for ESP32
WitAITTS tts(14, 15, 32);  // BCLK=14, LRC=15, DIN=32

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    tts.setDebugLevel(DEBUG_INFO);
    
    if (tts.begin("YourSSID", "YourPassword", "YourToken")) {
        tts.printConfig();  // Shows current pin configuration
        tts.speak("Custom pins working");
    }
}

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

### Pico W - Custom Pins

```cpp
#include <WitAITTS.h>

// Custom pins for Pico W
WitAITTS tts(0, 1, 2);  // BCLK=GP0, LRC=GP1, DIN=GP2

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    tts.setDebugLevel(DEBUG_INFO);
    
    if (tts.begin("YourSSID", "YourPassword", "YourToken")) {
        tts.printConfig();
        Serial.println("Type text to speak:");
    }
}

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

---

## Hardware Compatibility

### Tested I2S Audio Devices

**ESP32 Compatible:**
- MAX98357A (recommended - built-in DAC+Amp)
- PCM5102 (high quality DAC)
- UDA1334 (good quality)

**Pico W Compatible:**
- PCM5102 (recommended)
- UDA1334
- MAX98357A

---

## Quick Reference

### ESP32 Family Defaults

```cpp
// ESP32 (original)
WitAITTS tts;  // BCLK=27, LRC=26, DIN=25

// ESP32-C3
WitAITTS tts;  // BCLK=7, LRC=6, DIN=5

// ESP32-S3
WitAITTS tts;  // BCLK=16, LRC=17, DIN=15
```

### Pico W Default

```cpp
WitAITTS tts;  // BCLK=GP18, LRC=GP19, DIN=GP20
```

### Custom Pins (All Platforms)

```cpp
// Via constructor
WitAITTS tts(bclk, lrc, din);

// Via method (before begin)
WitAITTS tts;
tts.setPins(bclk, lrc, din);
tts.begin(...);
```

---

**Made with ❤️ by Jobit Joseph & Circuit Digest**
