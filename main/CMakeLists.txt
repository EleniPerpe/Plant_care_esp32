idf_component_register(SRCS "main.c" "bluetooth/bluetooth.c" "led/led.c" "sensors/light_sensor.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash bt esp_driver_gpio driver bt)