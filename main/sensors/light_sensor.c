#include "light_sensor.h"

static const char *TAG = "AS7341_SENSOR";

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;

void light_sensor_init()
{
    // Κρατάμε ΜΟΝΟ το setup του Bus
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, // Οι εξωτερικές 4.7k που βάλαμε είναι καλύτερες
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C bus");
        return;
    }

    // ΔΕΝ προσθέτουμε συσκευή εδώ με i2c_master_bus_add_device.
    // Θα το κάνει η as7341_init μέσα στο Task χρησιμοποιώντας το bus_handle.

    ESP_LOGI(TAG, "I2C Bus Initialized. Waiting for Task to take over.");
}

void light_sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting AS7341 Task...");

    as7341_config_t dev_cfg = I2C_AS7341_CONFIG_DEFAULT;
    as7341_handle_t dev_hdl = NULL;

    vTaskDelay(pdMS_TO_TICKS(1000));

    as7341_init(bus_handle, &dev_cfg, &dev_hdl);

    as7341_disable_led(dev_hdl);

    // Σωστό όνομα συνάρτησης και enum για τη βιβλιοθήκη k0i05
    as7341_set_spectral_gain(dev_hdl, AS7341_SPECTRAL_GAIN_16X);

    // Το ATIME παραμένει ως έχει
    as7341_set_atime(dev_hdl, 100);

    if (dev_hdl == NULL)
    {
        ESP_LOGE(TAG, "AS7341 handle init failed!");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "AS7341 Initialized Successfully!");

    while (1)
    {
        as7341_channels_spectral_data_t adc_data;
        // 1. Παίρνουμε τα raw δεδομένα
        esp_err_t result = as7341_get_spectral_measurements(dev_hdl, &adc_data);

        if (result == ESP_OK)
        {
            // 2. Μετατροπή σε Basic Counts για μέγιστη ακρίβεια (float τιμές)
            as7341_channels_basic_counts_data_t basic_data;
            as7341_get_basic_counts(dev_hdl, adc_data, &basic_data);

            // 3. Εκτύπωση όλων των χρωμάτων
            // F1: Violet, F2: Indigo, F3: Blue, F4: Cyan, F5: Green, F6: Yellow, F7: Orange, F8: Red
            ESP_LOGI(TAG, "--- Spectral Analysis ---");
            ESP_LOGW(TAG, "VIOLET (F1): %.2f | BLUE (F3): %.2f", basic_data.f1, basic_data.f3);
            ESP_LOGW(TAG, "GREEN  (F5): %.2f | YELLOW (F6): %.2f", basic_data.f5, basic_data.f6);
            ESP_LOGW(TAG, "ORANGE (F7): %.2f | RED    (F8): %.2f", basic_data.f7, basic_data.f8);
            ESP_LOGI(TAG, "CLEAR: %.2f | NIR: %.2f", basic_data.clear, basic_data.nir);

            // Ενημέρωση BLE με την τιμή CLEAR (ως ακέραιο για το BLE)
            current_light_value = (uint32_t)adc_data.clear;
            ble_gatts_chr_updated(light_val_handle);
        }
        else
        {
            ESP_LOGE(TAG, "Measurement failed");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// void light_sensor_task(void *pvParameters)
// {
//     ESP_LOGI(TAG, "Starting AS7341 Task with official component...");

//     // 1. Αρχικοποίηση Configuration
//     as7341_config_t dev_cfg = I2C_AS7341_CONFIG_DEFAULT;
//     as7341_handle_t dev_hdl = NULL;

//     // Περιμένουμε λίγο να σταθεροποιηθεί το σύστημα
//     vTaskDelay(pdMS_TO_TICKS(1000));

//     // 2. Init Device
//     // Προσοχή: Η βιβλιοθήκη θέλει το bus_handle που φτιάξαμε στην αρχή
//     as7341_init(bus_handle, &dev_cfg, &dev_hdl);
//     // Αυξάνουμε το Gain στο 16x ή 64x (αντί για 1x)
//     as7341_set_gain(dev_hdl, AS7341_GAIN_16X);

//     // Αυξάνουμε το χρόνο μέτρησης (ATIME).
//     // Το 100 δίνει περίπου 280ms ανά μέτρηση (πιο ακριβές)
//     as7341_set_atime(dev_hdl, 100);

//     if (dev_hdl == NULL)
//     {
//         ESP_LOGE(TAG, "AS7341 handle init failed! Check connections.");
//         vTaskDelete(NULL);
//         return;
//     }

//     ESP_LOGI(TAG, "AS7341 Initialized Successfully!");

//     while (1)
//     {
//         // 3. Λήψη μετρήσεων
//         as7341_channels_spectral_data_t adc_data;
//         esp_err_t result = as7341_get_spectral_measurements(dev_hdl, &adc_data);

//         if (result == ESP_OK)
//         {
//             // ch0 στη βιβλιοθήκη αντιστοιχεί στο CLEAR
//             ESP_LOGI(TAG, "CLEAR: %d | F1(Violet): %d | NIR: %d",
//                      adc_data.clear, adc_data.f1, adc_data.nir);

//             // Ενημέρωση του BLE
//             current_light_value = adc_data.clear;
//             ble_gatts_chr_updated(light_val_handle);
//         }
//         else
//         {
//             ESP_LOGE(TAG, "Spectral measurement failed (%s)", esp_err_to_name(result));
//         }

//         // Delay 1 δευτερόλεπτο
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }

//     // Αν ποτέ βγει από το loop, καθαρίζουμε
//     as7341_delete(dev_hdl);
//     vTaskDelete(NULL);
// }
