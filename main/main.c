#include "nvs_flash.h"
#include "led/led.h"
#include "sensors/light_sensor.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    light_sensor_init();

    led_init();

    bluetooth_init();

    xTaskCreate(light_sensor_task, "light_task", 8192, NULL, 10, NULL);

    ESP_LOGI(TAG, "System Initialized Successfully");
}