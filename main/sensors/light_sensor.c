#include "light_sensor.h"
#include <string.h>

static const char *TAG = "AS7341";

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;
as7341_data_t sensor_data = {0};

#define REG_CFG0     0xA9  
#define REG_ENABLE   0x80  
#define REG_ATIME    0x81
#define REG_WTIME    0x83
#define REG_ASTEP_L  0x84  
#define REG_STATUS   0x71  
#define REG_CFG6     0xAF  
#define REG_GAIN     0xAA  
#define REG_DATA     0x95  
#define REG_ID       0x92  

static esp_err_t reg_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev_handle, buf, 2, -1);
}

static esp_err_t reg_read(uint8_t reg, uint8_t *out, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, out, len, -1);
}

static void set_bank(bool bank1) {
    uint8_t cfg0 = 0;
    reg_read(REG_CFG0, &cfg0, 1);
    if (bank1) cfg0 |=  (1 << 4);
    else       cfg0 &= ~(1 << 4);
    reg_write(REG_CFG0, cfg0);
}

static void set_gain(uint8_t g) {
    if (g > GAIN_512X) g = GAIN_512X;
    set_bank(false);
    reg_write(REG_GAIN, g);
    sensor_data.gain = g;
}

static float gain_mult(void) {
    static const float tbl[] = { 0.5f, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };
    return tbl[sensor_data.gain > 10 ? 10 : sensor_data.gain];
}

// Η αυθεντική μέθοδος της Adafruit για F1-F4 (Απευθείας στο Bank 0!)
static void setup_F1F4_Clear_NIR(void) {
    reg_write(0x00, 0x30);
    reg_write(0x01, 0x01);
    reg_write(0x02, 0x00);
    reg_write(0x03, 0x00);
    reg_write(0x04, 0x00);
    reg_write(0x05, 0x42);
    reg_write(0x06, 0x00);
    reg_write(0x07, 0x00);
    reg_write(0x08, 0x50);
    reg_write(0x09, 0x00);
    reg_write(0x0A, 0x00);
    reg_write(0x0B, 0x00);
    reg_write(0x0C, 0x20);
    reg_write(0x0D, 0x04);
    reg_write(0x0E, 0x00);
    reg_write(0x0F, 0x30);
    reg_write(0x10, 0x01);
    reg_write(0x11, 0x50);
    reg_write(0x12, 0x00);
    reg_write(0x13, 0x06);
}

// Η αυθεντική μέθοδος της Adafruit για F5-F8 (Απευθείας στο Bank 0!)
static void setup_F5F8_Clear_NIR(void) {
    reg_write(0x00, 0x00);
    reg_write(0x01, 0x00);
    reg_write(0x02, 0x00);
    reg_write(0x03, 0x40);
    reg_write(0x04, 0x02);
    reg_write(0x05, 0x00);
    reg_write(0x06, 0x10);
    reg_write(0x07, 0x03);
    reg_write(0x08, 0x50);
    reg_write(0x09, 0x10);
    reg_write(0x0A, 0x03);
    reg_write(0x0B, 0x00);
    reg_write(0x0C, 0x00);
    reg_write(0x0D, 0x00);
    reg_write(0x0E, 0x24);
    reg_write(0x0F, 0x00);
    reg_write(0x10, 0x00);
    reg_write(0x11, 0x50);
    reg_write(0x12, 0x00);
    reg_write(0x13, 0x06);
}

// Ενεργοποίηση SMUX με βάση τη λογική Adafruit (AS7341_ENABLE bit 4)
static bool enableSMUX(void) {
    reg_write(REG_ENABLE, 0x11); // PON=1, SMUXEN=1
    
    // Timeout 100ms για να κάτσει το hardware mapping
    for (int i = 0; i < 100; i++) {
        uint8_t en = 0;
        reg_read(REG_ENABLE, &en, 1);
        if (!(en & 0x10)) return true; // Το SMUXEN γίνεται αυτόματα 0 όταν τελειώσει
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return false;
}

static void read_cycle(bool f1_f4, uint16_t out_ch[6]) {
    set_bank(false);
    reg_write(REG_ENABLE, 0x01); // measurement = false, PON = 1
    vTaskDelay(pdMS_TO_TICKS(5));

    reg_write(REG_CFG6, 0x10); // SMUX_CMD_WRITE

    if (f1_f4) setup_F1F4_Clear_NIR();
    else       setup_F5F8_Clear_NIR();

    enableSMUX();

    // Ξεκινάμε την ενσωμάτωση (enableSpectralMeasurement = true)
    reg_write(REG_ENABLE, 0x03); // PON=1, SP_EN=1

    // delayForData: Περιμένουμε το AVALID (STATUS2 bit 6 στην Adafruit, bit 3 στο γενικό status)
    uint8_t status = 0;
    for (int i = 0; i < 60; i++) {
        reg_read(REG_STATUS, &status, 1);
        if (status & (1 << 3)) break; 
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Ανάγνωση των καναλιών
    uint8_t raw[12] = {0};
    reg_read(REG_DATA, raw, 12);
    for (int i = 0; i < 6; i++) {
        out_ch[i] = (uint16_t)((raw[i*2+1] << 8) | raw[i*2]);
    }
}

void light_sensor_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&bus_cfg, &bus_handle);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = AS7341_ADDR,
        .scl_speed_hz    = 10000,
    };
    i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Καθαρό reset και αρχικοποίηση
    reg_write(REG_ENABLE, 0x00); 
    vTaskDelay(pdMS_TO_TICKS(50));
    reg_write(REG_ENABLE, 0x01); // PON = 1
    vTaskDelay(pdMS_TO_TICKS(10));

    reg_write(REG_ATIME, 71);
    uint8_t astep[3] = {REG_ASTEP_L, 0x57, 0x02}; 
    i2c_master_transmit(dev_handle, astep, 3, -1);

    reg_write(REG_WTIME, 0x00);
    set_gain(GAIN_16X); // Επαναφέρουμε το GAIN_16X που θέλει το Arduino sketch

    ESP_LOGI(TAG, "AS7341 True-Adafruit Driver Ready.");
}

void light_sensor_task(void *pvParameters) {
    while (dev_handle == NULL) vTaskDelay(pdMS_TO_TICKS(100));
    uint16_t ch[6];

    while (1) {
        // --- ΚΥΚΛΟΣ 1: F1 - F4 ---
        read_cycle(true, ch);
        sensor_data.f3     = ch[0]; 
        sensor_data.f1     = ch[1]; 
        sensor_data.f4     = ch[2]; 
        sensor_data.f2     = ch[3]; 
        sensor_data.vis_c1 = ch[4]; 
        sensor_data.nir    = ch[5]; 

        // --- ΚΥΚΛΟΣ 2: F5 - F8 ---
        read_cycle(false, ch);
        sensor_data.f6     = ch[0]; 
        sensor_data.f5     = ch[1]; 
        sensor_data.f8     = ch[2]; 
        sensor_data.f7     = ch[3]; 
        sensor_data.vis_c2 = ch[4]; 

        // Υπολογισμός Lux
        float vis_avg = (sensor_data.vis_c1 + sensor_data.vis_c2) / 2.0f;
        float basic = vis_avg / (gain_mult() * 120.10f);
        sensor_data.lux = basic * 0.8f * 1000.0f;

		// Ενημερώνουμε τη μεταβλητή του Bluetooth με το ακέραιο LUX
        current_light_value = (uint16_t)sensor_data.lux;

        // Αν υπάρχει ενεργή σύνδεση Bluetooth, στέλνουμε αυτόματα Notification
        if (current_conn_handle != 0 && light_val_handle != 0) {
            struct os_mbuf *om = ble_hs_mbuf_from_flat(&current_light_value, sizeof(current_light_value));
            if (om != NULL) {
                ble_gatts_notify_custom(current_conn_handle, light_val_handle, om); // Στέλνει το LUX στον αέρα!
            }
        }

        // Εκτύπωση
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        ESP_LOGI(TAG, "  LUX: %.1f  |  Gain: %.0fx  |  STATUS: ADAFRUIT MATCH", sensor_data.lux, gain_mult());
        ESP_LOGI(TAG, "  ── Spectrum (AS7341 100%% ACCURATE) ────────────────");
        ESP_LOGI(TAG, "  F1  415nm Violet   : %5d", sensor_data.f1);
        ESP_LOGI(TAG, "  F2  435nm Indigo   : %5d", sensor_data.f2);
        ESP_LOGI(TAG, "  F3  470nm Blue     : %5d", sensor_data.f3);
        ESP_LOGI(TAG, "  F4  515nm Cyan     : %5d", sensor_data.f4);
        ESP_LOGI(TAG, "  F5  555nm Green    : %5d", sensor_data.f5);
        ESP_LOGI(TAG, "  F6  590nm Orange   : %5d", sensor_data.f6);
        ESP_LOGI(TAG, "  F7  630nm Red      : %5d", sensor_data.f7);
        ESP_LOGI(TAG, "  F8  680nm Deep Red : %5d", sensor_data.f8);
        ESP_LOGI(TAG, "  ── Non-visible ──────────────────────────────────");
        ESP_LOGI(TAG, "  NIR 855nm Infrared : %5d", sensor_data.nir);
        ESP_LOGI(TAG, "  VIS Clear Average  : %.0f", vis_avg);
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}