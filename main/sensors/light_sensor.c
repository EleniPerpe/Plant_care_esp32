#include "light_sensor.h"

static const char *TAG = "AS7341_SENSOR";

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;

void light_sensor_init() {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK) return;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AS7341_ADDR,
        .scl_speed_hz = 10000, // Χαμηλή ταχύτητα για σιγουριά
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    vTaskDelay(pdMS_TO_TICKS(500)); 

    // 1. Πλήρες Reset (Power OFF)
    uint8_t cmd_stop[] = {0x80, 0x00};
    i2c_master_transmit(dev_handle, cmd_stop, 2, -1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. Ενεργοποίηση PON (Power On)
    uint8_t cmd_pwr[] = {0x80, 0x01};
    i2c_master_transmit(dev_handle, cmd_pwr, 2, -1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 3. Manual SMUX Configuration (Το "κλειδί")
    // Πρέπει να γράψουμε 20 bytes για να ορίσουμε το mapping
    uint8_t smux_cfg[21];
    smux_cfg[0] = 0x00; // Ξεκινάμε από τον εσωτερικό SMUX register 0
    for(int i=1; i<21; i++) smux_cfg[i] = 0x00; // Καθαρίζουμε τα πάντα
    
    // Ορίζουμε το mapping (Αυτές οι τιμές λένε στον αισθητήρα να συνδέσει CH0 και CH1)
    smux_cfg[7] = 0x85; 
    smux_cfg[8] = 0x21;

    // Πρόσβαση στο SMUX Bank
    uint8_t cmd_smux_access[] = {0xAF, 0x10}; 
    i2c_master_transmit(dev_handle, cmd_smux_access, 2, -1);
    
    // Γράφουμε το config
    i2c_master_transmit(dev_handle, smux_cfg, 21, -1);
    
    // Εκτέλεση SMUX Command
    uint8_t cmd_smux_exe[] = {0x80, 0x10};
    i2c_master_transmit(dev_handle, cmd_smux_exe, 2, -1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // 4. Επιστροφή στο Bank 0 και ρυθμίσεις
    uint8_t cmd_bank0[] = {0xAF, 0x00};
    i2c_master_transmit(dev_handle, cmd_bank0, 2, -1);

    uint8_t configs[][2] = {
        {0x81, 0x1D}, // ATIME 29
        {0xAA, 0x01}, // GAIN 1x
        {0x80, 0x03}  // ENABLE (PON + SPEN)
    };
    for(int i=0; i<3; i++) i2c_master_transmit(dev_handle, configs[i], 2, -1);

    ESP_LOGI(TAG, "Manual SMUX Mapping complete.");
}

void light_sensor_task(void *pvParameters) {
    uint8_t start_reg = 0x94; // Ξεκινάμε το διάβασμα από το CH0 (Clear)
    uint8_t raw_data[12];     // Θα διαβάσουμε 6 συνεχόμενα κανάλια (12 bytes)
    int zero_count = 0;
    float filtered_val = 0;   // Για το φίλτρο εξομάλυνσης

    while (dev_handle == NULL) vTaskDelay(pdMS_TO_TICKS(100));

	uint8_t force_start[] = {0x80, 0x03};
    i2c_master_transmit(dev_handle, force_start, 2, -1);


	uint8_t wake_cmd[] = {0x80, 0x03}; // PON + SP_EN
    uint8_t status_reg = 0x71;
    uint8_t status = 0;

	ESP_LOGI(TAG, "TASK STARTED - If you see this, we are alive!");
    
    while (1) {
		uint8_t status = 0;
		uint8_t status_reg = 0x71; 
		i2c_master_transmit_receive(dev_handle, &status_reg, 1, &status, 1, -1);

		// Αν το status είναι 0x00, ο αισθητήρας έχασε τις ρυθμίσεις του!
		if (status == 0x00) {
			ESP_LOGW(TAG, "Sensor Reset Detected! Re-applying SMUX...");
			
			// 1. Ξανακάνουμε το PON
			uint8_t cmd_pwr[] = {0x80, 0x01};
			i2c_master_transmit(dev_handle, cmd_pwr, 2, -1);

			// 2. SMUX Configuration (Το snippet που δούλεψε)
			uint8_t smux_cfg[21] = {0}; 
			smux_cfg[7] = 0x85; 
			smux_cfg[8] = 0x21;
			
			uint8_t cmd_smux_access[] = {0xAF, 0x10}; 
			i2c_master_transmit(dev_handle, cmd_smux_access, 2, -1);
			i2c_master_transmit(dev_handle, smux_cfg, 21, -1);
			
			uint8_t cmd_smux_exe[] = {0x80, 0x10};
			i2c_master_transmit(dev_handle, cmd_smux_exe, 2, -1);
			vTaskDelay(pdMS_TO_TICKS(50));

			// 3. Επιστροφή στο Bank 0 και Enable
			uint8_t cmd_bank0[] = {0xAF, 0x00};
			i2c_master_transmit(dev_handle, cmd_bank0, 2, -1);
			uint8_t enable[] = {0x80, 0x03}; 
			i2c_master_transmit(dev_handle, enable, 2, -1);
			
			vTaskDelay(pdMS_TO_TICKS(100)); // Δώσε χρόνο για την πρώτη μέτρηση
			continue; // Πήγαινε στην αρχή του loop να ξαναδεις το status
		}

		// Αν το status ΔΕΝ είναι 0, διάβασε κανονικά
		uint8_t data[4];
		uint8_t start_reg = 0x94; 
		esp_err_t err = i2c_master_transmit_receive(dev_handle, &start_reg, 1, data, 4, -1);
		
		if (err == ESP_OK) {
			uint16_t clear = (data[1] << 8) | data[0];
			uint16_t violet = (data[3] << 8) | data[2];

			ESP_LOGI(TAG, "STATUS: 0x%02X | CLEAR: %d | VIOLET: %d", status, clear, violet);
			
			current_light_value = clear;
			ble_gatts_chr_updated(light_val_handle);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
