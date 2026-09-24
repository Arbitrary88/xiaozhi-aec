#ifndef _YT2228_H_
#define _YT2228_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <functional>

class Yt2228 {
public:
    using WakeCallback = std::function<void()>;
    using ModeCallback = std::function<void(bool bluetooth_mode)>;

    Yt2228(uart_port_t uart_num, gpio_num_t tx_pin, gpio_num_t rx_pin, int baud_rate = 9600);
    ~Yt2228();

    void OnWake(WakeCallback callback) { on_wake_ = callback; }
    void OnModeChanged(ModeCallback callback) { on_mode_changed_ = callback; }

    bool SetAiMode();
    bool SetBluetoothMode();

private:
    uart_port_t uart_num_;
    gpio_num_t tx_pin_;
    gpio_num_t rx_pin_;
    int baud_rate_;
    TaskHandle_t task_handle_ = nullptr;
    bool running_ = false;

    WakeCallback on_wake_;
    ModeCallback on_mode_changed_;

    void TaskLoop();
};

#endif // _YT2228_H_
