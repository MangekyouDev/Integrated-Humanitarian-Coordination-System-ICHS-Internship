idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_i2c buttons spiffs_log include
)