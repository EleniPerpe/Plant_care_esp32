#include "light_sensor.h"

static const char *TAG = "AS7341_SENSOR";

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;

void light_sensor_init()
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AS7341_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    vTaskDelay(pdMS_TO_TICKS(100)); 

    uint8_t cmd_pwr[]    = {0x80, 0x01}; 
    uint8_t cmd_gain[]   = {0xAA, 0x03}; 
    uint8_t cmd_atime[] = {0x81, 0x95};
    uint8_t cmd_astep[] = {0xCA, 0x50, 0x00};
    uint8_t cmd_enable[] = {0x80, 0x03}; // Μόνο Spectral + Power
    uint8_t cmd_led_off[]= {0xBE, 0x00}; // GPIO Control -> OFF

    i2c_master_transmit(dev_handle, cmd_pwr, 2, -1);
    vTaskDelay(pdMS_TO_TICKS(10));
    i2c_master_transmit(dev_handle, cmd_gain, 2, -1);
    i2c_master_transmit(dev_handle, cmd_atime, 2, -1);
    i2c_master_transmit(dev_handle, cmd_astep, 3, -1);
    i2c_master_transmit(dev_handle, cmd_led_off, 2, -1); // Σβήσιμο LED
    i2c_master_transmit(dev_handle, cmd_enable, 2, -1);

    ESP_LOGI(TAG, "Sensor initialized (LED Locked OFF)");
}

void light_sensor_task(void *pvParameters) {
    uint8_t read_reg = 0x94; 
    uint8_t light_data[2];
	int zero_count = 0;

    ESP_LOGI(TAG, "Waiting for I2C driver to initialize...");
    while (dev_handle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

	vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "I2C Device Ready. Starting measurements...");

    while (1) {
        esp_err_t err = i2c_master_transmit_receive(dev_handle, &read_reg, 1, light_data, 2, -1);
        
        if (err == ESP_OK) {
            current_light_value = (light_data[1] << 8) | light_data[0];

			if (current_light_value == 0) {
                zero_count++;
                if (zero_count > 2) {
                    ESP_LOGW(TAG, "Sensor stuck at 0. Re-enabling...");
                    uint8_t cmd_gain[]   = {0xAA, 0x01}; 
					uint8_t cmd_atime[] = {0x81, 0x3B};
					uint8_t cmd_astep[] = {0xCA, 0x50, 0x00}; 
					uint8_t cmd_led_off[]= {0xBE, 0x00}; 
					uint8_t cmd_enable[] = {0x80, 0x03};

					i2c_master_transmit(dev_handle, cmd_gain, 2, -1);
					i2c_master_transmit(dev_handle, cmd_atime, 2, -1);
					i2c_master_transmit(dev_handle, cmd_astep, 3, -1);
					i2c_master_transmit(dev_handle, cmd_led_off, 2, -1);
					i2c_master_transmit(dev_handle, cmd_enable, 2, -1);

                    zero_count = 0;
                }
            } else {
                zero_count = 0;
            }
            
            ESP_LOGI(TAG, "Light Value: %d", current_light_value);
            
            ble_gatts_chr_updated(light_val_handle);
        } else {
            ESP_LOGE(TAG, "I2C Read Failed: %s", esp_err_to_name(err));
			vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}