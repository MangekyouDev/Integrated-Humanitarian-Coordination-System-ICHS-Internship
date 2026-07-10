#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"
#include <stdbool.h>

void buttons_init(void);

/* Returns true once per press (debounced) */
bool button_pressed(gpio_num_t pin);

#endif