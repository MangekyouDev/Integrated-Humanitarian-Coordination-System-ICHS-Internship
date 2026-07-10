#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board.h"

#define DEBOUNCE_MS 200

static gpio_num_t s_pins[] = { BUTTON_1_PIN, BUTTON_2_PIN };
static int s_last_level[2];
static int64_t s_last_press_time[2];

static void configure_pin(gpio_num_t pin)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}

void buttons_init(void)
{
    for (int i = 0; i < 2; i++) {
        configure_pin(s_pins[i]);
        s_last_level[i] = 1; // idle high (active LOW, pull-up)
        s_last_press_time[i] = 0;
    }
}

bool button_pressed(gpio_num_t pin)
{
    for (int i = 0; i < 2; i++) {
        if (s_pins[i] != pin) continue;

        int level = gpio_get_level(pin);
        bool pressed = false;

        if (level == 0 && s_last_level[i] == 1) {
            int64_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - s_last_press_time[i] > DEBOUNCE_MS) {
                pressed = true;
                s_last_press_time[i] = now;
            }
        }
        s_last_level[i] = level;
        return pressed;
    }
    return false;
}