#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include "esp_log.h"

/* ================= LCD DRIVER (YOUR CODE) ================= */

#include "driver/i2c_master.h"

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22
#define LCD_ADDR 0x27

i2c_master_bus_handle_t bus;
i2c_master_dev_handle_t lcd;

void lcd_write(uint8_t data)
{
    i2c_master_transmit(lcd, &data, 1, 1000 / portTICK_PERIOD_MS);
}

void lcd_pulse(uint8_t data)
{
    lcd_write(data | 0x04);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write(data & ~0x04);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void lcd_send4(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | (rs ? 0x01 : 0x00) | 0x08;
    lcd_pulse(data);
}

void lcd_cmd(uint8_t cmd)
{
    lcd_send4(cmd & 0xF0, 0);
    lcd_send4((cmd << 4) & 0xF0, 0);
}

void lcd_data(uint8_t data)
{
    lcd_send4(data & 0xF0, 1);
    lcd_send4((data << 4) & 0xF0, 1);
}

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

void lcd_set_cursor(int row, int col)
{
    int addr = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_cmd(addr);
}

void lcd_print(const char *str)
{
    while (*str)
        lcd_data(*str++);
}

/* ================= PINS ================= */

#define BTN1 GPIO_NUM_32
#define BTN2 GPIO_NUM_33

/* ================= GLOBAL STATE ================= */

static int log_count = 0;

/* ================= SPIFFS ================= */

void spiffs_init()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
    printf("SPIFFS mounted\n");
}

void write_log(const char *msg)
{
    FILE *f = fopen("/spiffs/log.txt", "a");
    if (f == NULL) {
        printf("Failed to open log file\n");
        return;
    }

    fprintf(f, "%s\n", msg);
    fclose(f);
}

/* ================= BUTTONS ================= */

void buttons_init()
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BTN1) | (1ULL << BTN2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io);
}

int btn1_pressed()
{
    return gpio_get_level(BTN1) == 0;
}

int btn2_pressed()
{
    return gpio_get_level(BTN2) == 0;
}

/* ================= LCD HELPERS ================= */

void lcd_show(const char *line1, const char *line2)
{
    char buf[32];

    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_set_cursor(0, 0);
    lcd_print(line1);

    lcd_set_cursor(1, 0);

    snprintf(buf, sizeof(buf), "%s", line2);
    lcd_print(buf);
}

/* ================= MAIN ================= */

void app_main(void)
{
    printf("SYSTEM START\n");

    /* UART ready */
    printf("UART OK\n");

    /* SPIFFS */
    spiffs_init();

    /* I2C LCD init */
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

    lcd_init();

    /* Buttons */
    buttons_init();

    /* Boot screen */
    lcd_show("Ready", "Logs: 0");

    printf("SYSTEM READY\n");

    int last1 = 1;
    int last2 = 1;

    char line2[32];

    while (1)
    {
        int b1 = btn1_pressed();
        int b2 = btn2_pressed();

        /* BTN1 = display */
        if (b1 && !last1)
        {
            printf("Button 1 pressed\n");

            snprintf(line2, sizeof(line2), "Logs: %d", log_count);
            lcd_show("Button 1", line2);
        }

        /* BTN2 = log */
        if (b2 && !last2)
        {
            log_count++;

            char msg[64];
            snprintf(msg, sizeof(msg), "Log entry %d", log_count);

            write_log(msg);

            printf("Log written: %s\n", msg);

            snprintf(line2, sizeof(line2), "Logs: %d", log_count);
            lcd_show("Logged!", line2);
        }

        last1 = b1;
        last2 = b2;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}