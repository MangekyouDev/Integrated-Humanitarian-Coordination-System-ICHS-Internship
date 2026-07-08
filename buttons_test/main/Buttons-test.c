#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BTN1 GPIO_NUM_25
#define BTN2 GPIO_NUM_26
#define BTN3 GPIO_NUM_27

#define DEBOUNCE_MS 200

void init_button(gpio_num_t pin) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}

void app_main(void)
{
    printf("BUTTON SYSTEM READY\n");

    init_button(BTN1);
    init_button(BTN2);
    init_button(BTN3);

    int last1 = 1, last2 = 1, last3 = 1;

    while (1) {

        int b1 = gpio_get_level(BTN1);
        int b2 = gpio_get_level(BTN2);
        int b3 = gpio_get_level(BTN3);

        // BUTTON 1
        if (b1 == 0 && last1 == 1) {
            printf("Button 1 pressed\n");
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }
        last1 = b1;

        // BUTTON 2
        if (b2 == 0 && last2 == 1) {
            printf("Button 2 pressed\n");
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }
        last2 = b2;

        // BUTTON 3
        if (b3 == 0 && last3 == 1) {
            printf("Button 3 pressed\n");
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }
        last3 = b3;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}