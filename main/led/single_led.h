#ifndef _SINGLE_LED_H_
#define _SINGLE_LED_H_

#include "led.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <led_strip.h>
#include <esp_timer.h>
#include <atomic>
#include <mutex>

class SingleLed : public Led {
public:
    SingleLed(gpio_num_t gpio, uint8_t default_brightness = 180, uint8_t high_brightness = 200, uint8_t low_brightness = 150);
    virtual ~SingleLed();

    void SetBrightness(uint8_t default_brightness, uint8_t high_brightness, uint8_t low_brightness);
    void OnStateChanged() override;

private:
    std::mutex mutex_;
    TaskHandle_t blink_task_ = nullptr;
    led_strip_handle_t led_strip_ = nullptr;
    uint8_t r_ = 0, g_ = 0, b_ = 0;
    uint8_t default_brightness_ = 180;
    uint8_t high_brightness_ = 200;
    uint8_t low_brightness_ = 150;
    int blink_counter_ = 0;
    int blink_interval_ms_ = 0;
    esp_timer_handle_t blink_timer_ = nullptr;

    void StartBlinkTask(int times, int interval_ms);
    void OnBlinkTimer();

    void BlinkOnce();
    void Blink(int times, int interval_ms);
    void StartContinuousBlink(int interval_ms);
    void TurnOn();
    void TurnOff();
    void SetColor(uint8_t r, uint8_t g, uint8_t b);
};

#endif // _SINGLE_LED_H_
