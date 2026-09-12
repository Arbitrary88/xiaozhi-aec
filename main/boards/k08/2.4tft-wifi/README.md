# K08 2.4-inch TFT LCD ESP32-S3 Wi-Fi Board (with AEC)

## 硬件特性
- 主控: ESP32-S3 (8MB Octal PSRAM, 16MB Flash)
- 屏幕: 2.4寸 ST7789 TFT LCD (240x320)
  - 屏幕供电控制: GPIO 3 (HIGH)
  - MOSI: GPIO 6, SCLK: GPIO 21, CS: GPIO 14, DC: GPIO 15
- 音频 Codec: ES8311
  - I2C: SDA GPIO 42, SCL GPIO 41
  - I2S: BCLK GPIO 40, WS GPIO 38, DOUT GPIO 47, DIN GPIO 48
  - 外设电源控制: GPIO 46 (1), GPIO 39 (1), GPIO 9 (0)
- 按键: Boot 按键 GPIO 0
- AEC 功能: 支持说话时通过唤醒词打断 (Wake Word Barge-in)
