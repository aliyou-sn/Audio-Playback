#include <freertos/FreeRTOS.h>
#include <driver/i2s.h>

#define USE_I2S

// speaker settings - if using I2S
#define I2S_SPEAKER_SERIAL_CLOCK GPIO_NUM_27
#define I2S_SPEAKER_LEFT_RIGHT_CLOCK GPIO_NUM_14
#define I2S_SPEAKER_SERIAL_DATA GPIO_NUM_26

#define GPIO_BUTTON GPIO_NUM_2

// sdcard
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_CLK GPIO_NUM_18
#define PIN_NUM_MOSI GPIO_NUM_23
#define PIN_NUM_CS GPIO_NUM_5

// i2s speaker pins definition
extern i2s_pin_config_t i2s_speaker_pins;

