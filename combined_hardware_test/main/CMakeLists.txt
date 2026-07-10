idf_component_register(
    SRCS "Combined_Hardware_Test.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_i2c buttons spiffs_log include
)