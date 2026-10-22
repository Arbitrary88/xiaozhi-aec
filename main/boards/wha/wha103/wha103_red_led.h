#ifndef _WHA103_RED_LED_H_
#define _WHA103_RED_LED_H_

#include "led/led.h"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_timer.h>

#include <mutex>

// WHA103 板级单色红色 LED 驱动（GPIO + LEDC PWM 调亮度）
// 独立于公共的 led/single_led.cc（WS2812）与 led/gpio_led.cc，互不影响。
// 单色灯没有颜色通道，状态仅靠亮度与闪烁节奏表达。
class Wha103RedLed : public Led {
public:
    // output_invert: false = GPIO 高电平点亮（LED 接 GPIO -> 限流电阻 -> GND）
    //                true  = GPIO 低电平点亮（3.3V -> LED -> 限流电阻 -> GPIO）
    explicit Wha103RedLed(gpio_num_t gpio, bool output_invert = false);
    ~Wha103RedLed() override;

    void OnStateChanged() override;

private:
    void SetBrightness(uint8_t percent);
    void TurnOn();
    void TurnOff();
    void StartContinuousBlink(int interval_ms);
    void OnBlinkTimer();

    std::mutex mutex_;
    ledc_channel_config_t channel_ = {};
    esp_timer_handle_t blink_timer_ = nullptr;
    uint32_t duty_ = 0;
    int blink_counter_ = 0;
    bool initialized_ = false;
};

#endif  // _WHA103_RED_LED_H_
