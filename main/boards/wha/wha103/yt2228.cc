#include "yt2228.h"
#include <esp_log.h>
#include <cstring>

#define TAG "YT2228"

enum class FrameState {
    kWaitHeader,
    kCmd1,
    kCmd2,
    kWaitTail
};

Yt2228::Yt2228(uart_port_t uart_num, gpio_num_t tx_pin, gpio_num_t rx_pin, int baud_rate)
    : uart_num_(uart_num), tx_pin_(tx_pin), rx_pin_(rx_pin), baud_rate_(baud_rate) {
    
    uart_config_t uart_config = {
        .baud_rate = baud_rate_,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(uart_num_, 1024, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num_, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num_, tx_pin_, rx_pin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "YT2228串口初始化完成 TX:%d RX:%d", (int)tx_pin_, (int)rx_pin_);

    // 默认发送关闭蓝牙/进入AI模式指令
    SetAiMode();

    running_ = true;
    BaseType_t ret = xTaskCreate(
        [](void* arg) {
            static_cast<Yt2228*>(arg)->TaskLoop();
            vTaskDelete(nullptr);
        },
        "yt2228",
        4096,
        this,
        8,
        &task_handle_
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create YT2228 task");
    }
}

Yt2228::~Yt2228() {
    running_ = false;
    if (task_handle_ != nullptr) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    uart_driver_delete(uart_num_);
}

bool Yt2228::SetAiMode() {
    // 0x99, 0x23, 0x24, 0x66: 关闭蓝牙指令 / 切换AI模式
    const uint8_t disable_bt_cmd[] = {0x99, 0x23, 0x24, 0x66};
    int ret = uart_write_bytes(uart_num_, disable_bt_cmd, sizeof(disable_bt_cmd));
    if (ret <= 0) {
        ESP_LOGE(TAG, "关闭蓝牙指令发送失败");
        return false;
    }
    return true;
}

bool Yt2228::SetBluetoothMode() {
    const uint8_t enable_bt_cmd[] = {0x99, 0x22, 0x23, 0x66};
    int ret = uart_write_bytes(uart_num_, enable_bt_cmd, sizeof(enable_bt_cmd));
    return ret > 0;
}

void Yt2228::TaskLoop() {
    ESP_LOGI(TAG, "开始监听YT2228数据...");
    FrameState state = FrameState::kWaitHeader;
    uint8_t cmd1 = 0, cmd2 = 0;
    uint8_t buf[32];

    while (running_) {
        int bytes_read = uart_read_bytes(uart_num_, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (bytes_read <= 0) {
            continue;
        }

        for (int i = 0; i < bytes_read; i++) {
            uint8_t b = buf[i];
            switch (state) {
                case FrameState::kWaitHeader:
                    if (b == 0x99) {
                        state = FrameState::kCmd1;
                    }
                    break;
                case FrameState::kCmd1:
                    cmd1 = b;
                    state = FrameState::kCmd2;
                    break;
                case FrameState::kCmd2:
                    cmd2 = b;
                    state = FrameState::kWaitTail;
                    break;
                case FrameState::kWaitTail:
                    if (b == 0x66) {
                        if (cmd1 == 0x00 && cmd2 == 0x01) {
                            ESP_LOGI(TAG, "唤醒词触发: 99 00 01 66");
                            if (on_wake_) {
                                on_wake_();
                            }
                        } else if (cmd1 == 0x22 && cmd2 == 0x23) {
                            ESP_LOGI(TAG, "收到蓝牙模式切换帧: 99 22 23 66");
                            if (on_mode_changed_) {
                                on_mode_changed_(true);
                            }
                        } else if (cmd1 == 0x23 && cmd2 == 0x24) {
                            ESP_LOGI(TAG, "收到AI模式切换帧: 99 23 24 66");
                            if (on_mode_changed_) {
                                on_mode_changed_(false);
                            }
                        } else {
                            ESP_LOGW(TAG, "未知YT2228数据帧: 99 %02x %02x 66", cmd1, cmd2);
                        }
                        state = FrameState::kWaitHeader;
                    } else if (b == 0x99) {
                        // 遇到新的起始字节
                        state = FrameState::kCmd1;
                    } else {
                        state = FrameState::kWaitHeader;
                    }
                    break;
            }
        }
    }
}
