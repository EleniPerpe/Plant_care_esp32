// #include <stdio.h>
// #include <string.h>
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "nimble/nimble_port.h"
// #include "nimble/nimble_port_freertos.h"
// #include "host/ble_hs.h"
// #include "host/ble_gatt.h"
// #include "services/gap/ble_svc_gap.h"
// #include "services/gatt/ble_svc_gatt.h"

// #include "bluetooth/bluetooth.h"
// #include "driver/i2c_master.h"

// // Ορισμοί I2C
// #define I2C_MASTER_SCL_IO           9      
// #define I2C_MASTER_SDA_IO           8      
// #define I2C_MASTER_NUM              0      
// #define AS7341_ADDR                 0x39   

// // Global Handles
// extern uint16_t light_val_handle;
// extern uint16_t current_light_value;
// i2c_master_bus_handle_t bus_handle = NULL; // <--- ΠΡΟΣΘΕΣΕ ΑΥΤΟ
// i2c_master_dev_handle_t dev_handle = NULL;

// int ble_gatt_push_local_val(uint16_t chr_val_handle, struct os_mbuf *om);

// // --- 1. Η ΣΥΝΑΡΤΗΣΗ ΤΟΥ TASK (Αυτή που έλειπε) ---
// void light_sensor_task(void *pvParameters) {
//     uint8_t read_reg = 0x94; 
//     uint8_t light_data[2];

// 	// Περίμενε λίγο μέχρι να βεβαιωθούμε ότι η init_sensors τελείωσε
//     while (dev_handle == NULL) {
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }

//     ESP_LOGI("AS7341", "I2C Device Ready. Starting measurements...");

//     while (1) {
//         // 1. Διάβασμα από I2C
//         esp_err_t err = i2c_master_transmit_receive(dev_handle, &read_reg, 1, light_data, 2, -1);
        
//         if (err == ESP_OK) {
//             current_light_value = (light_data[1] << 8) | light_data[0];
            
//             // 2. Εμφάνιση στο Monitor (ΑΥΤΟ ΕΛΕΙΠΕ)
//             ESP_LOGI("AS7341", "Light Value: %d", current_light_value);
            
//             // 3. Ειδοποίηση στο κινητό (Notify)
//             ble_gatts_chr_updated(light_val_handle);
//         } else {
//             ESP_LOGE("AS7341", "I2C Read Failed: %s", esp_err_to_name(err));
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }
// // void light_sensor_task(void *pvParameters) {
// //     uint8_t read_reg = 0x94; 
// //     uint16_t clear_value;
// //     uint8_t light_data[2];

// //     while (1) {
// //         if (i2c_master_transmit_receive(dev_handle, &read_reg, 1, light_data, 2, -1) == ESP_OK) {
// //             clear_value = (light_data[1] << 8) | light_data[0];
            
// //             // Δημιουργούμε ένα mbuf (memory buffer) που καταλαβαίνει το NimBLE
// //             struct os_mbuf *om = ble_hs_mbuf_from_flat(&clear_value, sizeof(clear_value));
            
// //             if (om) {
// //                 // Αυτή η συνάρτηση είναι η "επίσημη" για να ενημερώνεις τοπικά μια τιμή
// //                 // Αν δεν τη βρίσκει, θα χρησιμοποιήσουμε την από κάτω
// //                 ble_gatts_chr_updated(light_val_handle);
// //             }
            
// //             ESP_LOGI("AS7341", "Light Value: %d", clear_value);
// //         }
// //         vTaskDelay(pdMS_TO_TICKS(2000));
// //     }
// // }

// // --- 2. ΑΡΧΙΚΟΠΟΙΗΣΗ ΑΙΣΘΗΤΗΡΑ ---
// // void init_sensors(void) {
// //     i2c_master_bus_config_t bus_cfg = {
// //         .clk_source = I2C_CLK_SRC_DEFAULT,
// //         .i2c_port = I2C_MASTER_NUM,
// //         .scl_io_num = I2C_MASTER_SCL_IO,
// //         .sda_io_num = I2C_MASTER_SDA_IO,
// //         .glitch_ignore_cnt = 7,
// //         .flags.enable_internal_pullup = true,
// //     };
// //     // Χρησιμοποιούμε την global bus_handle
// //     ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

// //     i2c_device_config_t dev_cfg = {
// //         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
// //         .device_address = AS7341_ADDR,
// //         .scl_speed_hz = 100000,
// //     };
// //     // Χρησιμοποιούμε την global dev_handle
// //     ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

// //     // Power ON & Enable
// //     uint8_t pwr_cmd[2] = {0x80, 0x03}; 
// //     i2c_master_transmit(dev_handle, pwr_cmd, 2, -1);
// // }

// // // --- 3. APP MAIN ---
// // void app_main(void) {
// //     // 1. NVS Init
// //     esp_err_t ret = nvs_flash_init();
// //     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
// //         ESP_ERROR_CHECK(nvs_flash_erase());
// //         ret = nvs_flash_init();
// //     }
// //     ESP_ERROR_CHECK(ret);

// //     // 2. Hardware Init (I2C & LED)
// //     init_sensors();
// //     led_init();

// //     // 3. NimBLE Stack Init
// //     nimble_port_init();
    
// //     // 4. GATT & GAP Config
// //     ble_svc_gap_init();
// //     ble_svc_gatt_init(); // Μην το ξεχάσεις αυτό!
    
// //     bluetooth_init(); // Εδώ πρέπει να καλείται η ble_gatts_count_cfg() και ble_gatts_add_svcs()

// //     ble_svc_gap_device_name_set("Solar_Panel_Hub");

// //     // 5. Ορισμός του Sync Callback
// //     ble_hs_cfg.sync_cb = on_sync;

// //     // 6. Ξεκινάμε το Host Task
// //     nimble_port_freertos_init(nimble_host_task);

// //     // 7. Το δικό σου task για τον αισθητήρα
// //     xTaskCreate(light_sensor_task, "light_task", 4096, NULL, 5, NULL);
// // }

// void app_main(void) {
//     // 1. NVS Init
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     // 2. ΑΜΕΣΗ ΑΡΧΙΚΟΠΟΙΗΣΗ I2C ΕΔΩ
//     i2c_master_bus_config_t bus_cfg = {
//         .clk_source = I2C_CLK_SRC_DEFAULT,
//         .i2c_port = I2C_MASTER_NUM,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .glitch_ignore_cnt = 7,
//         .flags.enable_internal_pullup = true,
//     };
//     ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

//     i2c_device_config_t dev_cfg = {
//         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//         .device_address = AS7341_ADDR,
//         .scl_speed_hz = 100000,
//     };
//     ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

//     // Power ON & Enable αισθητήρα
//     uint8_t pwr_cmd[2] = {0x80, 0x03}; 
//     i2c_master_transmit(dev_handle, pwr_cmd, 2, -1);

//     // 3. LED & Bluetooth (τα υπόλοιπα όπως ήταν)
//     led_init();
//     nimble_port_init();
//     ble_svc_gap_init();
//     ble_svc_gatt_init();
//     bluetooth_init();
//     ble_svc_gap_device_name_set("Solar_Panel_Hub");
//     ble_hs_cfg.sync_cb = on_sync;
    
//     // 4. Ξεκινάμε τα Tasks
//     nimble_port_freertos_init(nimble_host_task);
//     xTaskCreate(light_sensor_task, "light_task", 4096, NULL, 5, NULL);
// }

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

// NimBLE Includes
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Project Includes
#include "bluetooth/bluetooth.h"
#include "driver/i2c_master.h"
#include "led/led.h"

// Ορισμοί I2C
#define I2C_MASTER_SCL_IO           2      
#define I2C_MASTER_SDA_IO           1      
#define I2C_MASTER_NUM              0      
#define AS7341_ADDR                 0x39   

// Global Handles
extern uint16_t light_val_handle;
uint16_t current_light_value = 0; // Αρχικοποίηση

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;

static const char *L_TAG = "AS7341_MAIN";

// --- 1. Η ΣΥΝΑΡΤΗΣΗ ΤΟΥ TASK ---
void light_sensor_task(void *pvParameters) {
    uint8_t read_reg = 0x94; // Clear Channel Data Register
    uint8_t light_data[2];
	int zero_count = 0;

    // Δικλείδα ασφαλείας: Περίμενε μέχρι η app_main να δημιουργήσει το dev_handle
    ESP_LOGI(L_TAG, "Waiting for I2C driver to initialize...");
    while (dev_handle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

	vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(L_TAG, "I2C Device Ready. Starting measurements...");

    while (1) {
        // Διάβασμα από τον αισθητήρα
        esp_err_t err = i2c_master_transmit_receive(dev_handle, &read_reg, 1, light_data, 2, -1);
        
        if (err == ESP_OK) {
            current_light_value = (light_data[1] << 8) | light_data[0];

			if (current_light_value == 0) {
                zero_count++;
                if (zero_count > 5) {
                    ESP_LOGW(TAG, "Sensor stuck at 0. Re-enabling...");
                    uint8_t enable_cmd[] = {0x80, 0x07}; 
                    i2c_master_transmit(dev_handle, enable_cmd, 2, -1);
                    zero_count = 0;
                }
            } else {
                zero_count = 0;
            }
            
            ESP_LOGI(L_TAG, "Light Value: %d", current_light_value);
            
            // Ενημέρωση της τιμής στο Bluetooth Stack
            // Χρησιμοποιούμε το handle που έχουμε από το bluetooth.c
            ble_gatts_chr_updated(light_val_handle);
        } else {
            ESP_LOGE(L_TAG, "I2C Read Failed: %s", esp_err_to_name(err));
			vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Ανάγνωση ανά 2 δευτερόλεπτα
    }
}

// --- 2. ΑΡΧΙΚΟΠΟΙΗΣΗ ΣΥΣΤΗΜΑΤΟΣ ---
void app_main(void) {
    // 1. NVS Flash Init (Απαραίτητο για το Bluetooth)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Αρχικοποίηση I2C Bus
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    // 3. Προσθήκη του AS7341 στο Bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AS7341_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

	// 4. Power ON & Configuration του AS7341
    uint8_t cmd_pwr[]   = {0x80, 0x01}; // Power ON
    uint8_t cmd_atime[] = {0x81, 0x64}; // Set ATIME (100)
    uint8_t cmd_astep[] = {0xCA, 0x03, 0xE7}; // Set ASTEP (999) - Register 0xCA και 0xCB
    uint8_t cmd_enable[]= {0x80, 0x03}; // Power ON + Spectral Enable (SMEN)

    i2c_master_transmit(dev_handle, cmd_pwr, 2, -1);
    vTaskDelay(pdMS_TO_TICKS(10));
    i2c_master_transmit(dev_handle, cmd_atime, 2, -1);
    i2c_master_transmit(dev_handle, cmd_astep, 3, -1); // Στέλνουμε 3 bytes για τα low/high registers
    i2c_master_transmit(dev_handle, cmd_enable, 2, -1);

    vTaskDelay(pdMS_TO_TICKS(100));

	vTaskDelay(pdMS_TO_TICKS(100)); // Δώσε του χρόνο να ξεκινήσει

    // 5. LED & Bluetooth Setup
    led_init();
    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    bluetooth_init(); // Πρέπει να καλεί τις ble_gatts_add_svcs κλπ.

    ble_svc_gap_device_name_set("Solar_Panel_Hub");
    ble_hs_cfg.sync_cb = on_sync;

    // 6. Εκκίνηση Tasks
    // Ξεκινάμε το Bluetooth Host
    nimble_port_freertos_init(nimble_host_task);
    
    // Ξεκινάμε το Task του αισθητήρα
    xTaskCreate(light_sensor_task, "light_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(L_TAG, "System Initialized Successfully");
}