#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

#include "board.h"
#include "lcd_i2c.h"
#include "buttons.h"
#include "spiffs_log.h"

#define LCD_ADDR 0x27

static int log_count = 0;

void app_main(void)
{
    printf("SYSTEM START\n");
    printf("UART OK\n");

    /* SPIFFS */
    spiffs_log_init();

    /* I2C LCD init */
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = LCD_SDA_PIN,
        .scl_io_num = LCD_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };

    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));

    lcd_i2c_init(bus);
    lcd_init();

    /* Buttons */
    buttons_init();

    /* Boot screen */
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_set_cursor(0, 0);
    lcd_print("Ready");
    lcd_set_cursor(1, 0);
    lcd_print("Logs: 0");

    printf("SYSTEM READY\n");

    char line2[32];

    while (1)
    {
        bool b1 = button_pressed(BUTTON_1_PIN);
        bool b2 = button_pressed(BUTTON_2_PIN);

        if (b1)
        {
            printf("Button 1 pressed\n");
            snprintf(line2, sizeof(line2), "Logs: %d", log_count);

            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Button 1");
            lcd_set_cursor(1, 0);
            lcd_print(line2);
        }

        if (b2)
        {
            log_count++;
            char msg[64];
            snprintf(msg, sizeof(msg), "Log entry %d", log_count);
            spiffs_log_write(msg);
            printf("Log written: %s\n", msg);

            snprintf(line2, sizeof(line2), "Logs: %d", log_count);

            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Logged!");
            lcd_set_cursor(1, 0);
            lcd_print(line2);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}