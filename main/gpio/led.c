#include "led.h"

#include "driver/gpio.h"


void LedInit(void)
{
    gpio_config_t config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << GPIO_LED_PIN)
    };

    // gpio_reset_pin(GPIO_LED_PIN);
    gpio_config(&config);
}


void LedCtrl(uint32_t level) 
{
    gpio_set_level(GPIO_LED_PIN, level);
}