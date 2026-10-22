#include "wha103_red_led.h"

#include "application.h"

#include <esp_log.h>

#define TAG "Wha103RedLed"

// LEDC 资源：低速模式 TIMER_1 / CHANNEL_0（wha103 无背光屏，TIMER_0 未被占用）
#define LEDC_SPEED_MODE_USED LEDC_LOW_SPEED_MODE
#define LEDC_TIMER_USED      LEDC_TIMER_1
#define LEDC_CHANNEL_USED    LEDC_CHANNEL_0
#define LEDC_DUTY_MAX        8191  // 13bit

// 单色红灯亮度（百分比）
#define BRIGHT_STARTUP        60
#define BRIGHT_WIFI_CONFIG    60
#define BRIGHT_CONNECTING     40
#define BRIGHT_LISTEN_VOICE   80
#define BRIGHT_LISTEN_SILENT  25
#define BRIGHT_SPEAKING       60
#define BRIGHT_UPGRADING      60
#define BRIGHT_ACTIVATING     60

#define BLINK_INFINITE -1

Wha103RedLed::Wha103RedLed(gpio_num_t gpio, bool output_invert) {
    if (gpio == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "GPIO_NUM_NC, 红色 LED 不工作");
        return;
    }

    ledc_timer_config_t timer_config = {};
    timer_config.duty_resolution = LEDC_TIMER_13_BIT;
    timer_config.freq_hz = 4000;
    timer_config.speed_mode = LEDC_SPEED_MODE_USED;
    timer_config.timer_num = LEDC_TIMER_USED;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    channel_.channel = LEDC_CHANNEL_USED;
    channel_.duty = 0;
    channel_.gpio_num = gpio;
    channel_.speed_mode = LEDC_SPEED_MODE_USED;
    channel_.hpoint = 0;
    channel_.timer_sel = LEDC_TIMER_USED;
    channel_.flags.output_invert = output_invert ? 1 : 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_));

    esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) {
            static_cast<Wha103RedLed*>(arg)->OnBlinkTimer();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wha103_red_led",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &blink_timer_));

    initialized_ = true;
}

Wha103RedLed::~Wha103RedLed() {
    if (blink_timer_ != nullptr) {
        esp_timer_stop(blink_timer_);
        esp_timer_delete(blink_timer_);
    }
    if (initialized_) {
        ledc_stop(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED, 0);
    }
}

void Wha103RedLed::SetBrightness(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    duty_ = (uint32_t)percent * LEDC_DUTY_MAX / 100;
}

void Wha103RedLed::TurnOn() {
    if (!initialized_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);
    ledc_set_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED, duty_);
    ledc_update_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED);
}

void Wha103RedLed::TurnOff() {
    if (!initialized_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);
    ledc_set_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED, 0);
    ledc_update_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED);
}

void Wha103RedLed::StartContinuousBlink(int interval_ms) {
    if (!initialized_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);
    blink_counter_ = BLINK_INFINITE * 2;
    esp_timer_start_periodic(blink_timer_, (uint64_t)interval_ms * 1000);
}

void Wha103RedLed::OnBlinkTimer() {
    if (!initialized_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    blink_counter_--;
    if (blink_counter_ & 1) {
        ledc_set_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED, duty_);
    } else {
        ledc_set_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED, 0);
    }
    ledc_update_duty(LEDC_SPEED_MODE_USED, LEDC_CHANNEL_USED);

    if (blink_counter_ == 0) {
        esp_timer_stop(blink_timer_);
    }
}

void Wha103RedLed::OnStateChanged() {
    auto& app = Application::GetInstance();
    switch (app.GetDeviceState()) {
        case kDeviceStateStarting:
            SetBrightness(BRIGHT_STARTUP);
            StartContinuousBlink(100);
            break;
        case kDeviceStateWifiConfiguring:
            SetBrightness(BRIGHT_WIFI_CONFIG);
            StartContinuousBlink(500);
            break;
        case kDeviceStateIdle:
            TurnOff();
            break;
        case kDeviceStateConnecting:
            SetBrightness(BRIGHT_CONNECTING);
            TurnOn();
            break;
        case kDeviceStateListening:
        case kDeviceStateAudioTesting:
            // 单色灯无法变色，用亮度区分“有无检测到语音”
            SetBrightness(app.IsVoiceDetected() ? BRIGHT_LISTEN_VOICE : BRIGHT_LISTEN_SILENT);
            TurnOn();
            break;
        case kDeviceStateSpeaking:
        case kDeviceStateNotifying:
            SetBrightness(BRIGHT_SPEAKING);
            TurnOn();
            break;
        case kDeviceStateUpgrading:
            SetBrightness(BRIGHT_UPGRADING);
            StartContinuousBlink(100);
            break;
        case kDeviceStateActivating:
            SetBrightness(BRIGHT_ACTIVATING);
            StartContinuousBlink(500);
            break;
        default:
            ESP_LOGW(TAG, "未知设备状态: %d", (int)app.GetDeviceState());
            break;
    }
}
