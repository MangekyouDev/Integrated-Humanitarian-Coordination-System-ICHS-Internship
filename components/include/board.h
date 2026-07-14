#ifndef BOARD_H
#define BOARD_H

#include "driver/gpio.h"

/* LCD I2C pins */
#define LCD_SDA_PIN GPIO_NUM_21
#define LCD_SCL_PIN GPIO_NUM_22

/* Buttons — named by role, used throughout the report terminal state machine */
#define BUTTON_SELECT_PIN GPIO_NUM_32
#define BUTTON_UP_PIN      GPIO_NUM_33
#define BUTTON_DOWN_PIN    GPIO_NUM_25
#define BUTTON_BACK_PIN    GPIO_NUM_26

#endif