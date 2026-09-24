#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/display.h"
#include "led/single_led.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "yt2228.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>

#define TAG "WHA103XiaoZhuiBoard"

class WHA103XiaoZhuiBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_ = nullptr;
    Button boot_button_;
    Yt2228* yt2228_ = nullptr;
    esp_timer_handle_t manual_listening_timer_ = nullptr;

    void InitializeGpio() {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << SHUTDOWN_GPIO) | (1ULL << AUDIO_CODEC_PA_PIN),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(SHUTDOWN_GPIO, 0);
        gpio_set_level(AUDIO_CODEC_PA_PIN, 0);
    }

    void InitializeCodecI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));

        if (i2c_master_probe(codec_i2c_bus_, 0x18, 1000) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to probe ES8311 I2C device at 0x18");
        } else {
            ESP_LOGI(TAG, "ES8311 I2C device detected at 0x18");
        }
    }

    void StartManualListeningGuardTimer() {
        if (manual_listening_timer_ != nullptr) {
            esp_timer_stop(manual_listening_timer_);
            // 60秒防呆保护定时器：防止用户首次按键后忘记再次按下
            esp_timer_start_once(manual_listening_timer_, 60 * 1000000ULL);
        }
    }

    void StopManualListeningGuardTimer() {
        if (manual_listening_timer_ != nullptr) {
            esp_timer_stop(manual_listening_timer_);
        }
    }

    void InitializeGuardTimer() {
        esp_timer_create_args_t timer_args = {
            .callback = [](void* arg) {
                auto& app = Application::GetInstance();
                if (app.GetDeviceState() == kDeviceStateListening) {
                    ESP_LOGW(TAG, "手动聆听超时（60秒），自动结束并进入回复...");
                    app.StopListening();
                }
            },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "manual_listen_guard"
        };
        esp_timer_create(&timer_args, &manual_listening_timer_);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            auto state = app.GetDeviceState();
            ESP_LOGI(TAG, "按键触发, 当前状态: %d", (int)state);

            // 开机未联网状态下，按键进入配网模式
            if (state == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }

            if (state == kDeviceStateIdle) {
                // 第1次按下：开启手动聆听（云端VAD静音不自动打断）
                ESP_LOGI(TAG, "按键操作: 开启手动聆听 (manual 模式)");
                app.StartListening();
                StartManualListeningGuardTimer();
            } else if (state == kDeviceStateListening) {
                // 第2次按下：用户说完话，手动结束聆听并立即触发AI回复
                ESP_LOGI(TAG, "按键操作: 结束手动聆听 (立即交由大模型回答)");
                StopManualListeningGuardTimer();
                app.StopListening();
            } else if (state == kDeviceStateSpeaking) {
                // AI播报中按下：立即打断说话
                ESP_LOGI(TAG, "按键操作: 打断AI播报");
                StopManualListeningGuardTimer();
                app.ToggleChatState();
            } else if (state == kDeviceStateConnecting || state == kDeviceStateNotifying) {
                // 连接或通知状态下按下：重置或打断
                StopManualListeningGuardTimer();
                app.ToggleChatState();
            }
        });
    }

    void InitializeYt2228() {
        yt2228_ = new Yt2228(YT2228_UART_NUM, YT2228_TX_PIN, YT2228_RX_PIN, YT2228_BAUD_RATE);
        yt2228_->OnWake([this]() {
            ESP_LOGI(TAG, "YT2228 离线唤醒词触发");
            Application::GetInstance().WakeWordInvoke("xiaozhi");
        });
        yt2228_->OnModeChanged([this](bool bluetooth_mode) {
            ESP_LOGI(TAG, "YT2228 模式变化: %s", bluetooth_mode ? "蓝牙模式" : "AI模式");
        });
    }

public:
    WHA103XiaoZhuiBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeGpio();
        InitializeCodecI2c();
        InitializeGuardTimer();
        InitializeButtons();
        InitializeYt2228();
    }

    virtual ~WHA103XiaoZhuiBoard() {
        if (manual_listening_timer_ != nullptr) {
            esp_timer_stop(manual_listening_timer_);
            esp_timer_delete(manual_listening_timer_);
        }
        delete yt2228_;
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual Display* GetDisplay() override {
        static NoDisplay no_display;
        return &no_display;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(
            codec_i2c_bus_,
            I2C_NUM_0,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR
        );
        return &audio_codec;
    }
};

DECLARE_BOARD(WHA103XiaoZhuiBoard);
