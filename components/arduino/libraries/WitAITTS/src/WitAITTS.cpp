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

#include "WitAITTS.h"

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

#ifdef ARDUINO_ARCH_ESP32
WitAITTS::WitAITTS(uint8_t bclkPin, uint8_t lrcPin, uint8_t dinPin,
                   uint32_t bufferSize)
    : _bclkPin(bclkPin), _lrcPin(lrcPin), _dinPin(dinPin) {
  _audio = nullptr;
  _mp3 = nullptr;
  _stream = nullptr;
  _isStreaming = false;
  _downloadCompleted = false;
  _initDefaults();
}
#endif

#ifdef ARDUINO_ARCH_RP2040
WitAITTS::WitAITTS(uint8_t bclkPin, uint8_t lrcPin, uint8_t dinPin)
    : _bclkPin(bclkPin), _lrcPin(lrcPin), _dinPin(dinPin) {
  _i2s = nullptr;
  _decoder = nullptr;
  _mp3Decoder = nullptr;
  _isPlaying = false;
  _initDefaults();
}
#endif

WitAITTS::~WitAITTS() {
#ifdef ARDUINO_ARCH_ESP32
  if (_mp3)
    delete _mp3;
  if (_audio)
    delete _audio;
#elif defined(ARDUINO_ARCH_RP2040)
  if (_decoder)
    delete _decoder;
  if (_mp3Decoder)
    delete _mp3Decoder;
  if (_i2s)
    delete _i2s;
#endif
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void WitAITTS::_initDefaults() {
  _initialized = false;
  _errorCallback = nullptr;

  // Default settings
  _voice = "wit$Remi";
  _style = "default";
  _speed = 100;
  _pitch = 100;
  _sfxCharacter = "none";
  _sfxEnvironment = "none";
  _gain = 0.5;
  _audioFormat = "audio/mpeg";
  _debugLevel = DEBUG_INFO;
}

bool WitAITTS::begin(const char *ssid, const char *password,
                     const char *witToken) {
  _debugPrint(DEBUG_INFO, "WitAITTS Initializing...");

  // Store credentials
  _ssid = String(ssid);
  _password = String(password);
  _witToken = String(witToken);

#ifdef ARDUINO_ARCH_ESP32
  // Set CPU to max speed for smooth streaming
  setCpuFrequencyMhz(240);

  // Initialize audio objects
  _audio = new ESP32I2SAudio(_bclkPin, _lrcPin, _dinPin);
  _mp3 = new BackgroundAudioMP3Class<RawDataBuffer<WITAI_BUFFER_SIZE>>(*_audio);
  _mp3->setGain(_gain);
  _mp3->begin();
#endif

#ifdef ARDUINO_ARCH_RP2040
  // Initialize I2S audio
  _i2s = new I2SStream();
  auto cfg = _i2s->defaultConfig(TX_MODE);
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.pin_bck = _bclkPin;
  cfg.pin_ws = _lrcPin;
  cfg.pin_data = _dinPin;
  _i2s->begin(cfg);

  // Initialize decoder
  _mp3Decoder = new MP3DecoderHelix();
  _decoder = new EncodedAudioStream(_i2s, _mp3Decoder);
  _decoder->begin();
#endif

  // Initialize secure client
  _secureClient.setInsecure();

  // Connect WiFi
  if (!_connectWiFi()) {
    _reportError("WiFi connection failed");
    return false;
  }

  _initialized = true;
  _debugPrint(DEBUG_INFO, "WitAITTS Ready");
  return true;
}

bool WitAITTS::_connectWiFi() {
  WiFi.mode(WIFI_STA);

#ifdef ARDUINO_ARCH_ESP32
  WiFi.setSleep(false); // Critical for streaming
#endif

  WiFi.begin(_ssid.c_str(), _password.c_str());

  _debugPrint(DEBUG_INFO, "Connecting to WiFi: " + _ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
    if (_debugLevel >= DEBUG_VERBOSE)
      Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  if (_debugLevel >= DEBUG_VERBOSE)
    Serial.println();
  _debugPrint(DEBUG_INFO, "WiFi Connected: " + WiFi.localIP().toString());
  return true;
}

// ============================================================================
// CORE FUNCTIONS
// ============================================================================

bool WitAITTS::speak(String text) {
  if (!_initialized) {
    _reportError("Not initialized");
    return false;
  }

  if (text.length() == 0) {
    _reportError("Empty text");
    return false;
  }

  if (text.length() > WITAI_MAX_TEXT_LENGTH) {
    _reportError("Text too long (max " + String(WITAI_MAX_TEXT_LENGTH) +
                 " chars)");
    return false;
  }

#ifdef ARDUINO_ARCH_ESP32
  _playWitTTS_ESP32(text);
  return true;
#elif defined(ARDUINO_ARCH_RP2040)
  return _playWitTTS_Pico(text);
#endif
}

// ============================================================================
// ESP32 IMPLEMENTATION (Non-blocking with BackgroundAudio)
// ============================================================================

#ifdef ARDUINO_ARCH_ESP32
void WitAITTS::_playWitTTS_ESP32(String text) {
  // Stop any current playback
  if (_isStreaming) {
    _http.end();
    _isStreaming = false;
    _stream = nullptr;
  }

  _secureClient.stop();
  _secureClient.setInsecure();

  ESP_LOGI("WitAITTS", "Requesting TTS: %s",text);
  _debugPrint(DEBUG_INFO, "Requesting TTS: " + text.substring(0, 30) + "...");

  // Build JSON payload
  String payload = _buildPayload(text);

  // Make HTTP request
  String url = "https://" + String(WITAI_HOST) + String(WITAI_PATH);
  if (!_http.begin(_secureClient, url)) {
    _reportError("HTTP connection failed");
    return;
  }
ESP_LOGI("WitAITTS", "HTTP con OK");
  _http.addHeader("Authorization", "Bearer " + _witToken);
  _http.addHeader("Content-Type", "application/json");
  _http.addHeader("Accept", _audioFormat);
ESP_LOGI("WitAITTS", "POST");
  int httpCode = _http.POST(payload);
ESP_LOGI("WitAITTS", "post return %d",httpCode);
  if (httpCode == HTTP_CODE_OK) {
    ESP_LOGI("WitAITTS", "Stream opened");
    _debugPrint(DEBUG_INFO, "Stream opened");
    _stream = _http.getStreamPtr();
    _isStreaming = true;
    _downloadCompleted = false;
    _mp3->pause(); // Start paused for buffering
  } else {
    ESP_LOGI("WitAITTS", "http error %d",httpCode);
    _reportError("HTTP Error: " + String(httpCode));
    _http.end();
  }
}

void WitAITTS::loop() {
  // Download Logic
  if (_isStreaming && _stream) {
    // Read data actively (multiple reads per loop for smooth streaming)
    for (int i = 0; i < 4; i++) {
      if (_stream->available()) {
        int bytesRead = _stream->read(_networkBuffer, WITAI_NETWORK_BUFFER);
        if (bytesRead > 0) {
          _mp3->write(_networkBuffer, bytesRead);
          _debugPrint(DEBUG_VERBOSE, "Read: " + String(bytesRead) + " bytes");
        }
      } else if (!_stream->connected()) {
        _debugPrint(DEBUG_INFO, "Download completed");
        _http.end();
        _isStreaming = false;
        _downloadCompleted = true;
        _stream = nullptr;
        break;
      }
    }
  }

  // Playback Logic
  if (_mp3->paused()) {
    // Paused/Buffering state
    if (_mp3->available() > WITAI_BUFFER_START_LEVEL) {
      _debugPrint(DEBUG_INFO, "Buffer ready, starting playback");
      _mp3->unpause();
    }
    // Critical fix for short words: If download is done, force play
    else if (_downloadCompleted) {
      _debugPrint(DEBUG_INFO, "Short audio/End of stream, force play");
      _mp3->unpause();
      _downloadCompleted = false;
    }
  }

  yield();
}

void WitAITTS::stop() {
  if (_isStreaming) {
    _http.end();
    _isStreaming = false;
    _stream = nullptr;
  }
  if (_mp3) {
    _mp3->pause();
  }
  _debugPrint(DEBUG_INFO, "Stopped");
}

bool WitAITTS::isPlaying() { return (_mp3 && !_mp3->paused()); }

bool WitAITTS::isBusy() { return _isStreaming || isPlaying(); }
#endif

// ============================================================================
// RP2040/PICO IMPLEMENTATION (Blocking with AudioTools)
// ============================================================================

#ifdef ARDUINO_ARCH_RP2040
bool WitAITTS::_playWitTTS_Pico(String text) {
  _isPlaying = true;

  _debugPrint(DEBUG_INFO, "Connecting to " + String(WITAI_HOST));

  if (!_secureClient.connect(WITAI_HOST, WITAI_PORT)) {
    _reportError("TLS connect failed");
    _isPlaying = false;
    return false;
  }

  // Build payload
  String payload = _buildPayload(text);

  // Send HTTP request manually
  _secureClient.printf("POST %s HTTP/1.1\r\n", WITAI_PATH);
  _secureClient.printf("Host: %s\r\n", WITAI_HOST);
  _secureClient.printf("Authorization: Bearer %s\r\n", _witToken.c_str());
  _secureClient.println("Content-Type: application/json");
  _secureClient.println("Accept: audio/mpeg");
  _secureClient.println("Connection: close");
  _secureClient.print("Content-Length: ");
  _secureClient.println(payload.length());
  _secureClient.println();
  _secureClient.print(payload);

  // Read status line
  String statusLine = _secureClient.readStringUntil('\n');
  _debugPrint(DEBUG_VERBOSE, "Status: " + statusLine);

  if (!statusLine.startsWith("HTTP/1.1 200")) {
    _reportError("HTTP Error: " + statusLine);
    _skipHeaders();
    _secureClient.stop();
    _isPlaying = false;
    return false;
  }

  // Skip headers
  _skipHeaders();
  _debugPrint(DEBUG_INFO, "Streaming audio...");

  // Stream audio using StreamCopy
  StreamCopy copier(*_decoder, _secureClient);
  unsigned long lastData = millis();

  while (true) {
    size_t n = copier.copy();

    if (n > 0) {
      lastData = millis();
    } else {
      delay(1);
    }

    // Timeout if no data for 500ms
    if (millis() - lastData > 500) {
      break;
    }
  }

  _secureClient.stop();
  _isPlaying = false;
  _debugPrint(DEBUG_INFO, "Playback finished");
  return true;
}

void WitAITTS::_skipHeaders() {
  while (_secureClient.connected()) {
    String line = _secureClient.readStringUntil('\n');
    if (line.length() == 0 || line == "\r")
      break;
    _debugPrint(DEBUG_VERBOSE, "[HDR] " + line);
  }
}

void WitAITTS::loop() {
  // Pico uses blocking playback, loop() does nothing
  yield();
}

void WitAITTS::stop() {
  if (_secureClient.connected()) {
    _secureClient.stop();
  }
  _isPlaying = false;
  _debugPrint(DEBUG_INFO, "Stopped");
}

bool WitAITTS::isPlaying() { return _isPlaying; }

bool WitAITTS::isBusy() { return _isPlaying; }
#endif

// ============================================================================
// COMMON HELPER FUNCTIONS
// ============================================================================

String WitAITTS::_buildSSML(String text) {
  String ssml = "<speak><sfx character='" + _sfxCharacter + "' environment='" +
                _sfxEnvironment + "'>" + text + "</sfx></speak>";
  return ssml;
}

String WitAITTS::_buildPayload(String text) {
  String ssml = _buildSSML(text);

  String payload = "{";
  payload += "\"q\":\"" + ssml + "\",";
  payload += "\"voice\":\"" + _voice + "\",";
  payload += "\"style\":\"" + _style + "\",";
  payload += "\"speed\":" + String(_speed) + ",";
  payload += "\"pitch\":" + String(_pitch);
  payload += "}";

  return payload;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void WitAITTS::setVoice(String voice) {
  _voice = voice;
  _debugPrint(DEBUG_INFO, "Voice: " + voice);
}

void WitAITTS::setStyle(String style) {
  _style = style;
  _debugPrint(DEBUG_INFO, "Style: " + style);
}

void WitAITTS::setSpeed(int speed) {
  _speed = constrain(speed, 0, 200);
  _debugPrint(DEBUG_INFO, "Speed: " + String(_speed));
}

void WitAITTS::setPitch(int pitch) {
  _pitch = constrain(pitch, 0, 200);
  _debugPrint(DEBUG_INFO, "Pitch: " + String(_pitch));
}

void WitAITTS::setSFXCharacter(String character) {
  _sfxCharacter = character;
  _debugPrint(DEBUG_INFO, "SFX Character: " + character);
}

void WitAITTS::setSFXEnvironment(String environment) {
  _sfxEnvironment = environment;
  _debugPrint(DEBUG_INFO, "SFX Environment: " + environment);
}

void WitAITTS::setGain(float gain) {
  _gain = constrain(gain, 0.0f, 1.0f);
#ifdef ARDUINO_ARCH_ESP32
  if (_mp3)
    _mp3->setGain(_gain);
#endif
  _debugPrint(DEBUG_INFO, "Gain: " + String(_gain));
}

void WitAITTS::setAudioFormat(String format) {
  if (format == "audio/mpeg" || format == "audio/pcm16") {
    _audioFormat = format;
    _debugPrint(DEBUG_INFO, "Format: " + format);
  } else {
    _reportError("Invalid audio format");
  }
}

void WitAITTS::setDebugLevel(uint8_t level) {
  _debugLevel = constrain(level, 0, 3);
  _debugPrint(DEBUG_INFO, "Debug Level: " + String(_debugLevel));
}

void WitAITTS::setPins(uint8_t bclk, uint8_t lrc, uint8_t din) {
  _bclkPin = bclk;
  _lrcPin = lrc;
  _dinPin = din;
  _debugPrint(DEBUG_INFO, "Pins set: BCLK=" + String(bclk) +
                              " LRC=" + String(lrc) + " DIN=" + String(din));
}

void WitAITTS::setErrorCallback(void (*callback)(String)) {
  _errorCallback = callback;
}

// ============================================================================
// STATUS & CONFIG
// ============================================================================

void WitAITTS::printConfig() {
  Serial.println("\n===== WitAITTS Configuration =====");
  Serial.println("Voice: " + _voice);
  Serial.println("Style: " + _style);
  Serial.println("Speed: " + String(_speed));
  Serial.println("Pitch: " + String(_pitch));
  Serial.println("SFX Character: " + _sfxCharacter);
  Serial.println("SFX Environment: " + _sfxEnvironment);
  Serial.println("Gain: " + String(_gain));
  Serial.println("Format: " + _audioFormat);
  Serial.println("Debug: " + String(_debugLevel));
  Serial.println("Pins: BCLK=" + String(_bclkPin) + " LRC=" + String(_lrcPin) +
                 " DIN=" + String(_dinPin));
  Serial.println("Status: " +
                 String(_initialized ? "Ready" : "Not initialized"));
  Serial.println("WiFi: " + String(WiFi.status() == WL_CONNECTED
                                       ? "Connected"
                                       : "Disconnected"));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("IP: " + WiFi.localIP().toString());
  }
  Serial.println("==================================\n");
}

String WitAITTS::getConfig() {
  String config = "Voice:" + _voice + ",Style:" + _style +
                  ",Speed:" + String(_speed) + ",Pitch:" + String(_pitch) +
                  ",Gain:" + String(_gain) + ",Debug:" + String(_debugLevel);
  return config;
}

// ============================================================================
// DEBUG & ERROR HANDLING
// ============================================================================

void WitAITTS::_debugPrint(uint8_t level, String message) {
  if (_debugLevel >= level) {
    String prefix = "";
    switch (level) {
    case DEBUG_ERROR:
      prefix = "[ERROR] ";
      break;
    case DEBUG_INFO:
      prefix = "[INFO] ";
      break;
    case DEBUG_VERBOSE:
      prefix = "[DEBUG] ";
      break;
    }
    Serial.println(prefix + message);
  }
}

void WitAITTS::_reportError(String error) {
  _debugPrint(DEBUG_ERROR, error);
  if (_errorCallback) {
    _errorCallback(error);
  }
}
