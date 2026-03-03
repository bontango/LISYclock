/*
 *
 * from
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

 //adapted to ESP-IDF and LISYclock by bontango
 
#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BackgroundAudio.h>
#include <ESP32I2SAudio.h>
#include <HTTPClient.h>

// ============================================================================
// functions
// ============================================================================
void _playWitTTS_ESP32(char* text);

// ============================================================================
// USER CONFIGURABLE PARAMETERS
// ============================================================================
  // Wit.ai Token (get from https://wit.ai -> Your App -> Settings)
  char _witToken[41] = "KIY6326Q5XTBHAV6E6HSNLSG3653Q2XY";

  // Default settings
  char d_voice[21] = "wit$Cooper";
  char d_style[21] = "default";
  int d_speed = 80;
  int d_pitch = 80;
  char d_sfxCharacter[21] = "robot";
  char d_sfxEnvironment[21] = "none";
  float d_gain = 0.5;
  // dynamic settings
  char _voice[21],_style[21],_sfxCharacter[21],_sfxEnvironment[21];
  int _speed, _pitch;
  float _gain;;
  
// Buffer Configuration 
//#define WITAI_BUFFER_SIZE (32 * 1024) // Ring buffer size (32KB default)
#define WITAI_BUFFER_SIZE (16 * 1024) // Ring buffer size (32KB default)
#define WITAI_NETWORK_BUFFER 512 //2048     // Network read buffer
#define WITAI_BUFFER_START_LEVEL                                               \
  (1 * 1024) // Wait for this much data before playing
#define WITAI_BUFFER_LOW_LEVEL (1 * 1024) // Pause if buffer drops below this

// Text Configuration
#define WITAI_MAX_TEXT_LENGTH 280 // Maximum text length (Wit.ai limit)

// Wit.ai API
#define WITAI_HOST "api.wit.ai"
#define WITAI_PORT 443
#define WITAI_PATH "/synthesize?v=20240304"
#define WITAI_AUDIO_FORMAT "audio/mpeg"

  HTTPClient _http;
  uint8_t _networkBuffer[WITAI_NETWORK_BUFFER];
  WiFiClient *_stream;
  
  // Network
  WiFiClientSecure _secureClient;

#endif // AUDIO_H
