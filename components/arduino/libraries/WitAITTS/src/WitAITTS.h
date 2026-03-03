/*
 * WitAITTS - Wit.ai Text-to-Speech Library for ESP32 and RP2040
 *
 * Copyright (c) 2025 Jobit Joseph, Circuit Digest
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WITAITTS_H
#define WITAITTS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ============================================================================
// PLATFORM-SPECIFIC INCLUDES AND DEFAULTS
// ===========================================================================

#ifdef ARDUINO_ARCH_ESP32
#include <BackgroundAudio.h>
#include <ESP32I2SAudio.h>
#include <HTTPClient.h>

// Default pins for different ESP32 variants
#if CONFIG_IDF_TARGET_ESP32C3
#define WITAI_DEFAULT_BCLK 7
#define WITAI_DEFAULT_LRC 6
#define WITAI_DEFAULT_DIN 5
#elif CONFIG_IDF_TARGET_ESP32S3
#define WITAI_DEFAULT_BCLK 16
#define WITAI_DEFAULT_LRC 17
#define WITAI_DEFAULT_DIN 15
#else
// Original ESP32
#define WITAI_DEFAULT_BCLK 27
#define WITAI_DEFAULT_LRC 26
#define WITAI_DEFAULT_DIN 25
#endif

#elif defined(ARDUINO_ARCH_RP2040)
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

// Default pins for Pico W / Pico 2 W
#define WITAI_DEFAULT_BCLK 18
#define WITAI_DEFAULT_LRC 19
#define WITAI_DEFAULT_DIN 20
#endif

// ============================================================================
// USER CONFIGURABLE PARAMETERS
// ============================================================================

// Buffer Configuration (ESP32 only - Pico uses streaming)
#define WITAI_BUFFER_SIZE (32 * 1024) // Ring buffer size (32KB default)
#define WITAI_NETWORK_BUFFER 2048     // Network read buffer
#define WITAI_BUFFER_START_LEVEL                                               \
  (1 * 1024) // Wait for this much data before playing
#define WITAI_BUFFER_LOW_LEVEL (1 * 1024) // Pause if buffer drops below this

// Text Configuration
#define WITAI_MAX_TEXT_LENGTH 280 // Maximum text length (Wit.ai limit)

// Debug Levels
#define DEBUG_OFF 0
#define DEBUG_ERROR 1
#define DEBUG_INFO 2
#define DEBUG_VERBOSE 3

// Wit.ai API
#define WITAI_HOST "api.wit.ai"
#define WITAI_PORT 443
#define WITAI_PATH "/synthesize?v=20240304"

// ============================================================================
// WITAITTS CLASS
// ============================================================================

class WitAITTS {
public:
// Constructor - uses default pins for platform
#ifdef ARDUINO_ARCH_ESP32
  WitAITTS(uint8_t bclkPin = WITAI_DEFAULT_BCLK,
           uint8_t lrcPin = WITAI_DEFAULT_LRC,
           uint8_t dinPin = WITAI_DEFAULT_DIN,
           uint32_t bufferSize = WITAI_BUFFER_SIZE);
#elif defined(ARDUINO_ARCH_RP2040)
  WitAITTS(uint8_t bclkPin = WITAI_DEFAULT_BCLK,
           uint8_t lrcPin = WITAI_DEFAULT_LRC,
           uint8_t dinPin = WITAI_DEFAULT_DIN);
#endif

  ~WitAITTS();

  // Initialization
  bool begin(const char *ssid, const char *password, const char *witToken);

  // Core Functions
  bool speak(String text);
  void stop();
  void loop(); // Required for ESP32, optional for Pico (blocking)
  bool isPlaying();
  bool isBusy();

  // Configuration - all settings configurable via code
  void setVoice(String voice);
  void setStyle(String style);
  void setSpeed(int speed); // 0-200, default 100
  void setPitch(int pitch); // 0-200, default 100
  void setSFXCharacter(String character);
  void setSFXEnvironment(String environment);
  void setGain(float gain);           // 0.0-1.0, default 0.5
  void setAudioFormat(String format); // "audio/mpeg" or "audio/pcm16"
  void setDebugLevel(uint8_t level);  // 0-3

  // Pin reconfiguration (call before begin())
  void setPins(uint8_t bclk, uint8_t lrc, uint8_t din);

  // Status
  void printConfig();
  String getConfig();

  // Error Callback (optional)
  void setErrorCallback(void (*callback)(String error));

private:
// Platform-specific audio objects
#ifdef ARDUINO_ARCH_ESP32
  ESP32I2SAudio *_audio;
  BackgroundAudioMP3Class<RawDataBuffer<WITAI_BUFFER_SIZE>> *_mp3;
  HTTPClient _http;
  uint8_t _networkBuffer[WITAI_NETWORK_BUFFER];
  WiFiClient *_stream;
  bool _isStreaming;
  bool _downloadCompleted;
#elif defined(ARDUINO_ARCH_RP2040)
  I2SStream *_i2s;
  EncodedAudioStream *_decoder;
  MP3DecoderHelix *_mp3Decoder;
  bool _isPlaying;
#endif

  // Network
  WiFiClientSecure _secureClient;

  // Pins
  uint8_t _bclkPin, _lrcPin, _dinPin;

  // Settings
  String _ssid;
  String _password;
  String _witToken;
  String _voice;
  String _style;
  int _speed;
  int _pitch;
  String _sfxCharacter;
  String _sfxEnvironment;
  float _gain;
  String _audioFormat;
  uint8_t _debugLevel;

  // State
  bool _initialized;

  // Error callback
  void (*_errorCallback)(String);

  // Internal Methods
  void _initDefaults();
  String _buildSSML(String text);
  String _buildPayload(String text);
  void _debugPrint(uint8_t level, String message);
  void _reportError(String error);
  bool _connectWiFi();

#ifdef ARDUINO_ARCH_ESP32
  void _playWitTTS_ESP32(String text);
#elif defined(ARDUINO_ARCH_RP2040)
  bool _playWitTTS_Pico(String text);
  void _skipHeaders();
#endif
};

#endif // WITAITTS_H
