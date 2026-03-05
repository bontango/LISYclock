// audio
// part of LISYclock
//
// play a mp3 file 'audio_play_mp3'
// connect to wit.ai and play speech from text 'audio_play_tts'
//
//use of https://github.com/earlephilhower/BackgroundAudio
//https://github.com/earlephilhower/BackgroundAudio/blob/master/examples/Mixer/Mixer.ino
//https://earlephilhower.github.io/BackgroundAudio/classBackgroundAudioWAVClass.html
//
//https://github.com/jobitjoseph/WitAITTS/tree/main
 //adapted to ESP-IDF and LISYclock by bontango

#include "audio.h"
extern "C"
{
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>     
#include "esp_partition.h"
#include "driver/i2s_std.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "gpiodefs.h"
}

static const char *TAG = "audio";
#define AUDIO_BUFFER 4096    // Size of audio buffer (samples)
ESP32I2SAudio audio(I2S_BLK_PIN, I2S_WS_PIN, I2S_DATA_OUT_PIN);
BackgroundAudioMP3Class<RawDataBuffer<WITAI_BUFFER_SIZE>> mp3(audio);

bool _isStreaming = false;
bool _downloadCompleted = false;

//sync text delivery to speak
SemaphoreHandle_t textReadySemaphore;
SemaphoreHandle_t loopstoppedSemaphore;
char text2speak[81];
bool signal_loop_to_stop = false;

// ============================================================================
// tts tasks
// ============================================================================
//needed because of limited stack in main
void tts_speak_task(void *pvParameters) {
    //if (text.length() > WITAI_MAX_TEXT_LENGTH) {
    while(1) {
        // Wait until data is ready
        if (xSemaphoreTake(textReadySemaphore, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "want to speak: %s",text2speak);
              // Stop any current playback?
            if (_isStreaming) {
              ESP_LOGW(TAG, "detected running stream! Signal loop to stop streaming");
              signal_loop_to_stop = true;
            // Wait until loop stopped streaming
            if (xSemaphoreTake(loopstoppedSemaphore, portMAX_DELAY)) {

              signal_loop_to_stop = false;
            }

            }

            _playWitTTS_ESP32(text2speak);
        }
    }
  }


void tts_loop_task(void *pvParameters) {
    
size_t httpavail;
size_t mp3avail;
size_t toRead;
size_t bytesRead;


while(1) {
  //new TTS request, cancel the running one
  if (signal_loop_to_stop) {
          ESP_LOGI(TAG,"loop task received signal to stop");
          _http.end();
          _isStreaming = false;
          _stream = nullptr;
          xSemaphoreGive(loopstoppedSemaphore);
  }
  // Download Logic
  if (_isStreaming && _stream) {
      // check data capacity of stream and mp3
      if (( httpavail = _stream->available()) > 0) {
      httpavail = std::min((size_t) WITAI_NETWORK_BUFFER, httpavail); // We can only read up to the buffer size
      mp3avail = mp3.availableForWrite();
      toRead = std::min(mp3avail, httpavail); // Only read as much as we can send to MP3
      bytesRead = _stream->read(_networkBuffer, toRead);
      if (bytesRead > 0) {
          int bytesWritten = mp3.write(_networkBuffer, bytesRead);
          if ( bytesWritten < bytesRead)
             ESP_LOGW(TAG,"mp3 missing %d bytes", bytesRead - bytesWritten);
          }
      } else if (!_stream->connected()) {
          ESP_LOGI(TAG,"Download completed");
          _http.end();
          _isStreaming = false;
          _downloadCompleted = true;
          _stream = nullptr;
        }    
  }

  // Playback Logic
  if (mp3.paused()) {
    // Paused/Buffering state
    if (mp3.available() > WITAI_BUFFER_START_LEVEL) {
      ESP_LOGI(TAG, "Buffer ready, starting playback");
      mp3.unpause();
    }
    // Critical fix for short words: If download is done, force play
    else if (_downloadCompleted) {
      ESP_LOGI(TAG, "Short audio/End of stream, force play");
      mp3.unpause();
      _downloadCompleted = false;
    }
  }

    vTaskDelay(1);
    } //endless loop
    
    
}

void audio_play_tts(char* text) {
    ESP_LOGI(TAG, "Semaphore give");
    strncpy(text2speak, text, 80);    
    xSemaphoreGive(textReadySemaphore);

}

void set_tts_defaults(void) {

  strcpy(_voice, d_voice);
  strcpy(_style, d_style);
  _speed = d_speed;
  _pitch = d_pitch;
  strcpy(_sfxCharacter, d_sfxCharacter);
  strcpy(_sfxEnvironment, d_sfxEnvironment);
  _gain = d_gain;
}

// ============================================================================
// init audio ( mp3 & tts )
// ============================================================================
esp_err_t audio_init(void) {

    textReadySemaphore = xSemaphoreCreateBinary();
    if (textReadySemaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create semaphore!\n");
        return ESP_FAIL;
    }

    loopstoppedSemaphore = xSemaphoreCreateBinary();
    if (loopstoppedSemaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create semaphore!\n");
        return ESP_FAIL;
    }

    //save the TTS defaults to the 'dynamic' ones
    set_tts_defaults();

    xTaskCreate(&tts_speak_task, "tts_speak_task", 8096, NULL, (tskIDLE_PRIORITY + 3), NULL);  
    xTaskCreatePinnedToCore(&tts_loop_task, "tts_loop_task", 8096, NULL, (tskIDLE_PRIORITY + 3), NULL, 1);  

  // Set CPU to max speed for smooth streaming
  setCpuFrequencyMhz(240);

  mp3.setGain(_gain);
  mp3.begin();

  // Initialize secure client
  _secureClient.setInsecure();

  return ESP_OK;
}

// ============================================================================
// play a mp3 file from SD
// ============================================================================
void audio_play_mp3(const char *filename) {

    ESP_LOGI(TAG , "Task for mp3: %s", filename);

    char path[64];
    snprintf(path, sizeof(path), "/sdcard/%s", filename);

    FILE *fh = fopen(path, "rb");
    if (fh == NULL) {
        ESP_LOGE("Music", "Failed to open file %s", path);
        return;
    }

    // Allocate audio buffer
    uint8_t *buf = (uint8_t*) calloc(AUDIO_BUFFER, sizeof(uint8_t));
    if (!buf) {
        ESP_LOGE(TAG , "Failed to allocate buffer");
        fclose(fh);
        return;
    }

    size_t bytes_read = 0;
    size_t bytes_to_write = 0;
    size_t bytes_availableForWrite = 0;

    bytes_availableForWrite = mp3.availableForWrite();
    if ( bytes_availableForWrite > AUDIO_BUFFER ) {
         bytes_to_write = AUDIO_BUFFER; }
    else {
         bytes_to_write = bytes_availableForWrite; }  

    while ( (bytes_read = fread(buf, sizeof(uint8_t), bytes_to_write, fh)) > 0) {
        // Write buffer to I2S
        mp3.write(buf, bytes_read);
        // Read next chunk if we can
        while ( (bytes_availableForWrite = mp3.availableForWrite()) < 1023 ) { vTaskDelay(1); }
        if ( bytes_availableForWrite > AUDIO_BUFFER ) {
            bytes_to_write = AUDIO_BUFFER; }
        else {
            bytes_to_write = bytes_availableForWrite; }  
    }

    free(buf);
    fclose(fh);
}

// ============================================================================
// ESP32 IMPLEMENTATION (Non-blocking with BackgroundAudio)
// ============================================================================

void _playWitTTS_ESP32(char* text) {

  char url[100];
  char ssml[120];
  char payload[220];
  char auth[50];

  ESP_LOGI(TAG, "Requesting TTS — free heap: %lu, max block: %lu",
           (unsigned long)esp_get_free_heap_size(),
           (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

  _secureClient.stop();
  _secureClient.setInsecure();

  // Build JSON payload
  sprintf(ssml,
          "<speak><sfx character='%s' environment='%s'>%s</sfx></speak>",
          _sfxCharacter,_sfxEnvironment, text);
  sprintf(payload,
          "{\"q\":\"%s\",\"voice\":\"%s\",\"style\":\"%s\",\"speed\":%d,\"pitch\":%d}",
          ssml, _voice, _style, _speed, _pitch);

  // Make HTTP request
  sprintf(url,"https://%s%s",WITAI_HOST,WITAI_PATH);
  if (!_http.begin(_secureClient, url)) {
    ESP_LOGW(TAG, "HTTP connection failed");
    return;
  }
  ESP_LOGI(TAG, "HTTP connection OK");
  sprintf(auth,"Bearer %s",_witToken);
  _http.addHeader("Authorization", auth);
  _http.addHeader("Content-Type", "application/json");
  _http.addHeader("Accept", WITAI_AUDIO_FORMAT);
  ESP_LOGI(TAG, "POST");
  int httpCode = _http.POST(payload);
  //ESP_LOGI(TAG, "payload: %s",payload);
  ESP_LOGI(TAG, "post return %d",httpCode);
  if (httpCode == HTTP_CODE_OK) {
    ESP_LOGI(TAG, "Stream opened");
    _stream = _http.getStreamPtr();
    _isStreaming = true;
    _downloadCompleted = false;
    mp3.pause(); // Start paused for buffering
  } else {
    ESP_LOGW(TAG, "http error %d",httpCode);
    _http.end();
  }
}
