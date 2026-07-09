#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"

void buttons_init(void);
int button_pressed(gpio_num_t pin);

#endif