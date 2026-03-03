/*
 * WitAITTS Pico W / Pico 2 W Basic Example
 * 
 * Text-to-Speech using Wit.ai API on Raspberry Pi Pico W
 * 
 * Copyright (c) 2025 Jobit Joseph, Circuit Digest
 * 
 * Hardware:
 * - Raspberry Pi Pico W or Pico 2 W
 * - I2S Audio DAC (PCM5102, UDA1334, MAX98357A, etc.)
 * - Speaker (4-8 ohm, 3W recommended)
 * 
 * Default I2S Connections (Pico W):
 * Pico GP18 -> DAC BCLK
 * Pico GP19 -> DAC LRC/LRCLK
 * Pico GP20 -> DAC DIN/DATA
 * DAC VIN   -> 3.3V or 5V (check your DAC)
 * DAC GND   -> GND
 * 
 * Note: This example uses BLOCKING audio playback.
 * The speak() function will not return until audio finishes.
 * 
 * Instructions:
 * 1. Update WiFi credentials below
 * 2. Get Wit.ai token from https://wit.ai
 * 3. Select "Raspberry Pi Pico W" in Arduino IDE
 * 4. Upload sketch
 * 5. Open Serial Monitor (115200 baud)
 * 6. Type any text and press Enter to hear it spoken
 * 
 * Required Libraries:
 * - arduino-audio-tools by Phil Schatzmann
 *   (Install manually from: https://github.com/pschatzmann/arduino-audio-tools)
 */

#include <WitAITTS.h>

// ==================== CONFIGURATION ====================
// WiFi Credentials
const char* WIFI_SSID     = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// Wit.ai Token (get from https://wit.ai -> Your App -> Settings)
const char* WIT_TOKEN = "YOUR_WIT_AI_TOKEN_HERE";

// I2S Pins (default for Pico W: BCLK=18, LRC=19, DIN=20)
// Uncomment to use custom pins:
// #define CUSTOM_BCLK 18
// #define CUSTOM_LRC  19
// #define CUSTOM_DIN  20
// ========================================================

// Create TTS object with default pins
WitAITTS tts;

// Or with custom pins:
// WitAITTS tts(CUSTOM_BCLK, CUSTOM_LRC, CUSTOM_DIN);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);  // Wait for Serial on Pico
    
    Serial.println("\n\n========================================");
    Serial.println("   WitAITTS Pico W Basic Example");
    Serial.println("   Copyright (c) 2025 Jobit Joseph");
    Serial.println("           Circuit Digest");
    Serial.println("========================================\n");
    
    // ==================== DEBUG LEVEL ====================
    // DEBUG_OFF     (0) - No output
    // DEBUG_ERROR   (1) - Errors only
    // DEBUG_INFO    (2) - Info messages (default)
    // DEBUG_VERBOSE (3) - All messages including network data
    tts.setDebugLevel(DEBUG_INFO);
    
    // Initialize TTS
    if (tts.begin(WIFI_SSID, WIFI_PASSWORD, WIT_TOKEN)) {
        Serial.println("✓ TTS Ready!\n");
        
        // ==================== VOICE CONFIGURATION ====================
        // Available Voices:
        // 
        // US English Female:
        //   wit$Remi      - Natural female (default)
        //   wit$Rebecca   - Professional female
        //   wit$Railey    - Friendly female
        //   wit$Rubie     - Warm female
        //   wit$Disaffected - Monotone female
        //   wit$Cartoon Kid - Playful child
        //   wit$Cartoon Villain - Evil female
        //   wit$Kenyan Accent - Kenyan English female
        // 
        // US English Male:
        //   wit$Cody      - Natural male
        //   wit$Charlie   - Friendly male
        //   wit$Cooper    - Professional male
        //   wit$Carl      - Casual male
        //   wit$Surfer    - Laid-back male
        //   wit$Vampire   - Spooky male
        //   wit$Prospector - Old western male
        //   wit$Southern Accent - US Southern male
        // 
        // UK English:
        //   wit$Pirate    - Pirate male (UK)
        //   wit$Wizard    - Fantasy male (UK)
        //   wit$British Butler - Formal male (UK)
        //   wit$Cockney Accent - Cockney nonbinary (UK)
        // 
        // Canadian English:
        //   wit$Rosie     - Canadian female
        //   wit$Colin     - Canadian male
        // 
        // Special:
        //   wit$Cartoon Baby - Baby voice (nonbinary)
        tts.setVoice("wit$Remi");
        
        // ==================== VOICE STYLE ====================
        // Available Styles:
        //   "default"   - Normal speaking voice
        //   "soft"      - Gentle, quiet tone
        //   "formal"    - Professional, clear
        //   "fast"      - Quick speaking pace
        //   "projected" - Loud, clear voice
        // Note: Not all voices support all styles
        tts.setStyle("default");
        
        // ==================== SPEED & PITCH ====================
        // Speed: 0-200 (100 = normal, <100 = slower, >100 = faster)
        // Pitch: 0-200 (100 = normal, <100 = lower, >100 = higher)
        tts.setSpeed(100);
        tts.setPitch(100);
        
        // ==================== VOLUME/GAIN ====================
        // Gain: 0.0 to 1.0 (0.0 = mute, 1.0 = max volume)
        // Note: Gain control may have limited effect on Pico W
        tts.setGain(0.5);
        
        // ==================== SOUND EFFECTS (Optional) ====================
        // SFX Character - Transforms the voice:
        //   "none"     - No effect (default)
        //   "chipmunk" - High-pitched squeaky voice
        //   "monster"  - Deep growly voice
        //   "daemon"   - Dark demonic voice
        //   "robot"    - Mechanical robotic voice
        //   "alien"    - Otherworldly effect
        // tts.setSFXCharacter("none");
        
        // SFX Environment - Adds acoustic effects:
        //   "none"      - No effect (default)
        //   "reverb"    - Standard reverb
        //   "room"      - Small room acoustics
        //   "cathedral" - Large hall echo
        //   "radio"     - Radio/telephone quality
        //   "phone"     - Phone call simulation
        // tts.setSFXEnvironment("none");
        
        // Print current configuration
        tts.printConfig();
        
        Serial.println("Type any text and press Enter to speak:");
        Serial.println("(Note: Audio playback is BLOCKING on Pico W)\n");
        
    } else {
        Serial.println("✗ TTS initialization failed!");
        Serial.println("Check WiFi credentials and Wit.ai token");
    }
}

void loop() {
    // Note: For Pico W, loop() is optional since playback is blocking
    // But we still call it for consistency
    tts.loop();
    
    // Check for serial input
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();
        
        if (text.length() > 0) {
            Serial.println("Speaking: " + text);
            
            // Note: speak() BLOCKS until audio finishes on Pico W
            if (!tts.speak(text)) {
                Serial.println("TTS request failed. Check token/connection.");
            }
        }
    }
}
