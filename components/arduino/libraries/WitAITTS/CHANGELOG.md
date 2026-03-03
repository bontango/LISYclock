# Changelog

All notable changes to WitAITTS library will be documented in this file.

## [1.0.0] - 2025-12-20

### Initial Release

- 🎉 First public release of WitAITTS library
- ✅ ESP32 support with BackgroundAudio (non-blocking playback)
- ✅ ESP32-C3 support with default pins (7, 6, 5)
- ✅ ESP32-S3 support with default pins (16, 17, 15)
- ✅ Raspberry Pi Pico W support with AudioTools (blocking playback)
- ✅ Raspberry Pi Pico 2 W support
- ✅ 23+ voices with multiple styles
- ✅ Sound effects (character and environment)
- ✅ Configurable I2S pins via constructor or setPins()
- ✅ Debug levels (OFF, ERROR, INFO, VERBOSE)
- ✅ Error callback support
- ✅ Comprehensive documentation

### Supported Platforms

| Platform | Default Pins (BCLK/LRC/DIN) | Audio Library |
|----------|------------------------------|---------------|
| ESP32 | 27 / 26 / 25 | BackgroundAudio |
| ESP32-C3 | 7 / 6 / 5 | BackgroundAudio |
| ESP32-S3 | 16 / 17 / 15 | BackgroundAudio |
| Pico W | 18 / 19 / 20 | AudioTools |
| Pico 2 W | 18 / 19 / 20 | AudioTools |

---

**Made with ❤️ by Jobit Joseph & Circuit Digest**
