idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd esp_lcd_ili9341 lvgl)

target_add_binary_data(${COMPONENT_LIB} "images/bg.bin" BINARY)