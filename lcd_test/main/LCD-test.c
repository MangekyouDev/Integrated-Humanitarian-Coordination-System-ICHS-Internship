#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22
#define LCD_ADDR 0x27

i2c_master_bus_handle_t bus;
i2c_master_dev_handle_t lcd;

/* low-level write */
void lcd_write(uint8_t data)
{
    i2c_master_transmit(lcd, &data, 1, 1000 / portTICK_PERIOD_MS);
}

/* pulse enable */
void lcd_pulse(uint8_t data)
{
    lcd_write(data | 0x04);   // EN = 1
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write(data & ~0x04);  // EN = 0
    vTaskDelay(pdMS_TO_TICKS(1));
}

/* send 4-bit */
void lcd_send4(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | (rs ? 0x01 : 0x00) | 0x08;
    lcd_pulse(data);
}

/* command */
void lcd_cmd(uint8_t cmd)
{
    lcd_send4(cmd & 0xF0, 0);
    lcd_send4((cmd << 4) & 0xF0, 0);
}

/* data */
void lcd_data(uint8_t data)
{
    lcd_send4(data & 0xF0, 1);
    lcd_send4((data << 4) & 0xF0, 1);
}

/* init */
void lcd_init()
{
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_cmd(0x33);
    lcd_cmd(0x32);
    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);

    vTaskDelay(pdMS_TO_TICKS(5));
}

/* set cursor */
void lcd_set_cursor(int row, int col)
{
    int addr = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_cmd(addr);
}

/* print string */
void lcd_print(const char *str)
{
    while (*str)
        lcd_data(*str++);
}

void app_main(void)
{
    printf("LCD TEST START\n");

    /* I2C init */
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_config, &lcd));

    /* LCD init */
    lcd_init();

    int counter = 0;
    char buf[32];   // bigger + safe

    while (1) {

        lcd_cmd(0x01); // clear
        vTaskDelay(pdMS_TO_TICKS(5));

        lcd_set_cursor(0, 0);
        lcd_print("ICHS OK");

        lcd_set_cursor(1, 0);

        /* FIX: safe formatting */
        snprintf(buf, sizeof(buf), "Counter: %d", counter++);
        lcd_print(buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}