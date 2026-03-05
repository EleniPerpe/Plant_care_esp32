#include "nvs_flash.h"
#include "led/led.h"
#include "sensors/light_sensor.h"

// Ορισμοί I2C
#define I2C_MASTER_SCL_IO           2      
#define I2C_MASTER_SDA_IO           1      
#define I2C_MASTER_NUM              0      
#define AS7341_ADDR                 0x39   

static const char *TAG = "MAIN";


// --- 2. ΑΡΧΙΚΟΠΟΙΗΣΗ ΣΥΣΤΗΜΑΤΟΣ ---
void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	light_sensor_init();

    led_init();
    bluetooth_init(); 

    xTaskCreate(light_sensor_task, "light_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System Initialized Successfully");
}