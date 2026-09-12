#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// ES8311 I2S Pins
#define AUDIO_I2S_GPIO_MCLK      GPIO_NUM_40
#define AUDIO_I2S_GPIO_BCLK      GPIO_NUM_38
#define AUDIO_I2S_GPIO_WS        GPIO_NUM_47
#define AUDIO_I2S_GPIO_DOUT      GPIO_NUM_48
#define AUDIO_I2S_GPIO_DIN       GPIO_NUM_5

// ES8311 I2C Pins
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_42
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_41
#define AUDIO_CODEC_ES8311_ADDR  0x30
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_NC

// Auxiliary Power & Peripheral Control Pins
#define PWR_GPIO_46              GPIO_NUM_46
#define PWR_GPIO_39              GPIO_NUM_39
#define PWR_GPIO_9               GPIO_NUM_9

// Buttons
#define BOOT_BUTTON_GPIO         GPIO_NUM_0
#define TOUCH_BUTTON_GPIO        GPIO_NUM_NC
#define VOLUME_UP_BUTTON_GPIO    GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO  GPIO_NUM_NC

// Display ST7789 2.4 inch
#define DISPLAY_EN_PIN           GPIO_NUM_3
#define DISPLAY_MOSI_PIN         GPIO_NUM_6
#define DISPLAY_CLK_PIN          GPIO_NUM_21
#define DISPLAY_DC_PIN           GPIO_NUM_15
#define DISPLAY_RST_PIN          GPIO_NUM_NC
#define DISPLAY_CS_PIN           GPIO_NUM_14
#define DISPLAY_BACKLIGHT_PIN    GPIO_NUM_NC

#define DISPLAY_WIDTH            240
#define DISPLAY_HEIGHT           320
#define DISPLAY_MIRROR_X         false
#define DISPLAY_MIRROR_Y         false
#define DISPLAY_SWAP_XY          false
#define DISPLAY_INVERT_COLOR     true
#define DISPLAY_RGB_ORDER        LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X         0
#define DISPLAY_OFFSET_Y         0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#endif // _BOARD_CONFIG_H_
