// #include "light_sensor.h"

// static const char *TAG = "AS7343_SENSOR";

// i2c_master_bus_handle_t bus_handle = NULL;
// i2c_master_dev_handle_t dev_handle = NULL;

// void light_sensor_init() {
//     i2c_master_bus_config_t bus_cfg = {
//         .clk_source = I2C_CLK_SRC_DEFAULT,
//         .i2c_port = I2C_MASTER_NUM,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .glitch_ignore_cnt = 7,
//         .flags.enable_internal_pullup = true,
//     };

//     esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Bus init failed: %s", esp_err_to_name(err));
//         return;
//     }

//     i2c_device_config_t dev_cfg = {
//         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//         .device_address = AS7343_ADDR,
//         .scl_speed_hz = 10000,
//     };

//     err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Device add failed: %s", esp_err_to_name(err));
//         return;
//     }

//     vTaskDelay(pdMS_TO_TICKS(500));

//     // 1. Reset
//     uint8_t cmd_stop[] = {0x80, 0x00};
//     i2c_master_transmit(dev_handle, cmd_stop, 2, -1);
//     vTaskDelay(pdMS_TO_TICKS(200));

//     // 2. Power ON
//     uint8_t cmd_pwr[] = {0x80, 0x01};
//     i2c_master_transmit(dev_handle, cmd_pwr, 2, -1);
//     vTaskDelay(pdMS_TO_TICKS(10));

//     // 3. ATIME + GAIN
//     uint8_t cmd_atime[] = {0x81, 0x63}; // ATIME = 99
//     uint8_t cmd_gain[]  = {0xAA, 0x08}; // GAIN = 256x
//     i2c_master_transmit(dev_handle, cmd_atime, 2, -1);
//     i2c_master_transmit(dev_handle, cmd_gain,  2, -1);

//     // 4. Set CFG6: SMUX write mode
//     uint8_t cmd_cfg6[] = {0xAF, 0x10};
//     i2c_master_transmit(dev_handle, cmd_cfg6, 2, -1);

//     // 5. Write SMUX RAM (F1-F4 + Clear + NIR mapping)
//     uint8_t smux[21] = {
//         0x00,
//         0x30, 0x01,
//         0x00, 0x00,
//         0x42, 0x02,
//         0x00, 0x50,
//         0x00, 0x00,
//         0x20, 0x51,
//         0x00, 0x00,
//         0x00, 0x00,
//         0x00, 0x50,
//         0x06, 0x00,
//     };
//     i2c_master_transmit(dev_handle, smux, 21, -1);
//     vTaskDelay(pdMS_TO_TICKS(10));

//     // 6. Execute SMUX transfer (PON | SMUXEN)
//     uint8_t cmd_smuxen[] = {0x80, 0x11};
//     i2c_master_transmit(dev_handle, cmd_smuxen, 2, -1);
//     vTaskDelay(pdMS_TO_TICKS(100));

//     // 7. Enable measurements (PON | SP_EN)
//     uint8_t cmd_enable[] = {0x80, 0x03};
//     i2c_master_transmit(dev_handle, cmd_enable, 2, -1);

//     // 8. Disable wait timer + return to Bank 0
//     uint8_t cmd_wtime[] = {0x83, 0x00};
//     i2c_master_transmit(dev_handle, cmd_wtime, 2, -1);
//     uint8_t cmd_bank0[] = {0xAF, 0x00};
//     i2c_master_transmit(dev_handle, cmd_bank0, 2, -1);

//     vTaskDelay(pdMS_TO_TICKS(200));
//     ESP_LOGI(TAG, "AS7341 initialized.");
// }

// void light_sensor_task(void *pvParameters) {
//     while (dev_handle == NULL) vTaskDelay(pdMS_TO_TICKS(100));

//     ESP_LOGI(TAG, "light_sensor_task started.");

//     while (1) {
//         // Read all 6 channels starting at 0x94 (2 bytes each)
//         uint8_t start_reg = 0x94;
//         uint8_t data[12];
//         esp_err_t err = i2c_master_transmit_receive(dev_handle, &start_reg, 1, data, 12, -1);

//         if (err == ESP_OK) {
//             uint16_t ch0 = (data[1]  << 8) | data[0];
//             uint16_t ch1 = (data[3]  << 8) | data[2];
//             uint16_t ch2 = (data[5]  << 8) | data[4];
//             uint16_t ch3 = (data[7]  << 8) | data[6];
//             uint16_t ch4 = (data[9]  << 8) | data[8];
//             uint16_t ch5 = (data[11] << 8) | data[10];

//             ESP_LOGI(TAG, "CH0(violet)=%d  CH1(blue)=%d  CH2(cyan)=%d  CH3(green)=%d  CH4(clear)=%d  CH5(NIR)=%d",
//          	ch0, ch1, ch2, ch3, ch4, ch5);

//             current_light_value = ch0;
//             ble_gatts_chr_updated(light_val_handle);
//         } else {
//             ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
//         }

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

#include "light_sensor.h"
#include <string.h>

static const char *TAG = "AS7343";

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;
as7343_data_t sensor_data = {0};

// ---------------------------------------------------------------------------
// Register helpers
// ---------------------------------------------------------------------------

static esp_err_t reg_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev_handle, buf, 2, -1);
}

static esp_err_t reg_read(uint8_t reg, uint8_t *out, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, out, len, -1);
}

// ---------------------------------------------------------------------------
// Gain helpers
// ---------------------------------------------------------------------------

static void set_gain(uint8_t g) {
    if (g > GAIN_2048X) g = GAIN_2048X;
    reg_write(0xAA, g);
    sensor_data.gain = g;
}

static float gain_multiplier(uint8_t g) {
    static const float tbl[] = {
        0.5f, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048
    };
    if (g > 12) g = 12;
    return tbl[g];
}

static float gain_mult(void) { return gain_multiplier(sensor_data.gain); }

// ---------------------------------------------------------------------------
// SMUX configs — verified from AN001033 (ams-OSRAM official application note)
//
// Pixel layout (Figure 3 of AN001033):
//   ID: 0      1     2    3    4    5    6    7    8      9
//       VIS_LB XL    F2   F7   FZ   F1   F4   F5   VIS_LT VIS_RT
//   ID: 10     11    12   13   14     15   16    17   18
//       F6     F8    FY   F3   VIS_RB FD   NIR   Ext  Dark
//
// byte[n] = (nibble[2n+1] << 4) | nibble[2n]
// ADC values: 0=disabled, 1=ADC0, 2=ADC1, 3=ADC2, 4=ADC3, 5=ADC4, 6=ADC5(FD)
//
// Cycle 1: F1→ADC0, F2→ADC1, F3→ADC2, F4→ADC3, VIS→ADC4, NIR→ADC5
//          read order: ADC0=F1, ADC1=F2, ADC2=F3, ADC3=F4, ADC4=VIS, ADC5=NIR
// static const uint8_t SMUX_C1[10] = {
//     0x05, 0x02, 0x10, 0x04, 0x55, 0x00, 0x30, 0x05, 0x06, 0x00
// };

// // Cycle 2: F6→ADC0, F7→ADC1, F8→ADC2, F5→ADC3, VIS→ADC4, NIR→ADC5
// //          read order: ADC0=F6, ADC1=F7, ADC2=F8, ADC3=F5, ADC4=VIS, ADC5=NIR
// static const uint8_t SMUX_C2[10] = {
//     0x05, 0x20, 0x00, 0x40, 0x55, 0x31, 0x00, 0x05, 0x06, 0x00
// };

// // Cycle 3: FZ→ADC0, FY→ADC1, FXL→ADC2, NIR→ADC3, VIS→ADC4, FD→ADC5
// //          read order: ADC0=FZ, ADC1=FY, ADC2=FXL, ADC3=NIR, ADC4=VIS, ADC5=FD
// static const uint8_t SMUX_C3[10] = {
//     0x30, 0x00, 0x01, 0x00, 0x05, 0x00, 0x02, 0x65, 0x04, 0x00
// };

// ---------------------------------------------------------------------------
// Load SMUX config and trigger measurement
// ---------------------------------------------------------------------------

static void smux_run(const uint8_t cfg[10]) {
    reg_write(0x80, 0x01); // PON only — stop spectral measurement
    vTaskDelay(pdMS_TO_TICKS(5));

    reg_write(0xF5, 0x10); // CFG6: SMUX write mode

    // Write 10 bytes to internal SMUX RAM starting at address 0x00
    // The start address is sent as the first data byte (not an I2C subaddress)
    // uint8_t payload[11];
    // payload[0] = 0x00;
    // memcpy(&payload[1], cfg, 10);
    // i2c_master_transmit(dev_handle, payload, 11, -1);
	for (int i = 0; i < 10; i++) {
		reg_write(i, cfg[i]);
	}
    vTaskDelay(pdMS_TO_TICKS(5));

    // PON | SMUXEN — copy RAM to SMUX chain
    reg_write(0x80, 0x11);

    // Wait for SMUXEN to clear (transfer complete)
    for (int i = 0; i < 20; i++) {
        vTaskDelay(pdMS_TO_TICKS(5));
        uint8_t en = 0;
        reg_read(0x80, &en, 1);
        if (!(en & 0x10)) break;
    }

    // reg_write(0xAF, 0x00); // back to Bank 0
    reg_write(0x80, 0x03); // PON | SP_EN — start measurement

    // Wait for integration: ATIME=29 → (29+1)*2.78ms = 83.4ms + margin
    vTaskDelay(pdMS_TO_TICKS(120)); // 500
}

static void read_6ch(uint16_t out[6]) {
    uint8_t raw[12] = {0};
    reg_read(0x94, raw, 12);
    for (int i = 0; i < 6; i++)
        out[i] = (uint16_t)((raw[i * 2 + 1] << 8) | raw[i * 2]);
}

// ---------------------------------------------------------------------------
// Auto-gain: run one SMUX cycle, adjust gain if saturated or too dark
// ---------------------------------------------------------------------------

static void measure_cycle(const uint8_t smux[10], uint16_t out[6]) {
    for (int attempt = 0; attempt < 8; attempt++) {
        smux_run(smux);
		uint8_t en = 0;
		reg_read(0x80, &en, 1);
		ESP_LOGI(TAG, "ENABLE after smux_run = 0x%02X", en);
        read_6ch(out);

        // ch[4] is always VIS/Clear in our configs
        bool saturated = false;
        for (int i = 0; i < 4; i++) { // check spectral channels only
            if (out[i] >= SATURATION_THRESHOLD) { saturated = true; break; }
        }

        if (saturated) {
            if (sensor_data.gain == GAIN_0_5X) {
                sensor_data.saturated = true;
                return;
            }
            set_gain(sensor_data.gain - 1);
            ESP_LOGW(TAG, "Saturated → gain down to %.0fx", gain_mult());
        } else if (out[4] < LOW_LIGHT_THRESHOLD) {
            if (sensor_data.gain == GAIN_2048X) return;
            set_gain(sensor_data.gain + 1);
            ESP_LOGW(TAG, "Dark → gain up to %.0fx", gain_mult());
        } else {
            sensor_data.saturated = false;
            return; // exposure OK
        }
    }
}

// ---------------------------------------------------------------------------
// Lux — calibrate LUX_COEFF against a reference meter
// ATIME=29 → integration = (29+1) * 2.78ms = 83.4ms
// ---------------------------------------------------------------------------

#define LUX_COEFF 0.55f

static void compute_lux(void) {
    float vis = (sensor_data.vis_c1 + sensor_data.vis_c2 + sensor_data.vis_c3) / 3.0f;
    float basic = vis / (gain_mult() * 83.4f);
    sensor_data.lux = basic * LUX_COEFF * 1000.0f;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void light_sensor_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Bus init failed: %s", esp_err_to_name(err));
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = AS7343_ADDR,
        .scl_speed_hz    = 10000,
    };

    err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Device add failed: %s", esp_err_to_name(err));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

	reg_write(0x80, 0x00); // reset
    vTaskDelay(pdMS_TO_TICKS(200));
    reg_write(0x80, 0x01); // PON
    vTaskDelay(pdMS_TO_TICKS(10));

    reg_write(0x81, 0x1D); // ATIME = 29
    reg_write(0x83, 0x00); // WTIME = 0

    // CFG20 (0xD6): auto_smux = 3 → 18 channels, 3 cycles αυτόματα
    reg_write(0xD6, 0x60);

    set_gain(GAIN_64X);
    reg_write(0x80, 0x03); // PON | SP_EN

    // reg_write(0x80, 0x00); // reset
    // vTaskDelay(pdMS_TO_TICKS(200));
    // reg_write(0x80, 0x01); // PON
    // vTaskDelay(pdMS_TO_TICKS(10));

    // reg_write(0x81, 0x1D); // ATIME = 29 → 83.4ms
    // reg_write(0x83, 0x00); // WTIME = 0

    // set_gain(GAIN_64X);

    ESP_LOGI(TAG, "AS7343 init OK");
}

// ---------------------------------------------------------------------------
// Task
// ---------------------------------------------------------------------------

void light_sensor_task(void *pvParameters) {
    while (dev_handle == NULL) vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "light_sensor_task started.");

    uint16_t ch[6];

	while (1) {
		// Περίμενε 3 integration cycles (3 x 83ms + margin)
		vTaskDelay(pdMS_TO_TICKS(600));
		
		uint8_t raw[42] = {0};
		reg_read(0x94, raw, 42);
		
		// Cycle 1 (registers 0x94-0xA0): FZ, FY, FXL, NIR, VIS_TL, VIS_BR, FD
		uint16_t fz  = (raw[1]  << 8) | raw[0];
		uint16_t fy  = (raw[3]  << 8) | raw[2];
		uint16_t fxl = (raw[5]  << 8) | raw[4];
		uint16_t nir = (raw[7]  << 8) | raw[6];
		uint16_t vis1= (raw[9]  << 8) | raw[8];
		
		// Cycle 2 (registers 0xA2-0xAE): F2, F3, F4, F6, VIS, VIS, FD  
		uint16_t f2  = (raw[15] << 8) | raw[14];
		uint16_t f3  = (raw[17] << 8) | raw[16];
		uint16_t f4  = (raw[19] << 8) | raw[18];
		uint16_t f6  = (raw[21] << 8) | raw[20];
		uint16_t vis2= (raw[23] << 8) | raw[22];
		
		// Cycle 3 (registers 0xB0-0xBC): F1, F7, F8, F5, VIS, VIS, FD
		uint16_t f1  = (raw[29] << 8) | raw[28];
		uint16_t f7  = (raw[31] << 8) | raw[30];
		uint16_t f8  = (raw[33] << 8) | raw[32];
		uint16_t f5  = (raw[35] << 8) | raw[34];
		uint16_t vis3= (raw[37] << 8) | raw[36];

		// ESP_LOGI(TAG, "F1=%d F2=%d FZ=%d F3=%d F4=%d F5=%d FY=%d FXL=%d F6=%d F7=%d F8=%d NIR=%d",
		// 		f1, f2, fz, f3, f4, f5, fy, fxl, f6, f7, f8, nir);

					    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        ESP_LOGI(TAG, "  ── Visible spectrum (ordered by wavelength) ────────");
        ESP_LOGI(TAG, "  F1  405nm violet     : %5d", f1);
        ESP_LOGI(TAG, "  F2  425nm indigo     : %5d", f2);
        ESP_LOGI(TAG, "  FZ  450nm blue-viol  : %5d", fz);
        ESP_LOGI(TAG, "  F3  475nm blue       : %5d", f3);
        ESP_LOGI(TAG, "  F4  515nm cyan       : %5d", f4);
        ESP_LOGI(TAG, "  F5  550nm grn-yellow : %5d", f5);
        ESP_LOGI(TAG, "  FY  555nm green      : %5d", fy);
        ESP_LOGI(TAG, "  FXL 600nm orange     : %5d", fxl);
        ESP_LOGI(TAG, "  F6  640nm red        : %5d", f6);
        ESP_LOGI(TAG, "  F7  690nm deep red   : %5d", f7);
        ESP_LOGI(TAG, "  F8  745nm far red    : %5d", f8);
        ESP_LOGI(TAG, "  ── Non-visible ─────────────────────────────────────");
        ESP_LOGI(TAG, "  NIR 855nm infrared   : %5d", nir);
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	}



    // while (1) {
    //     // Cycle 1: F1(ADC0), F2(ADC1), F3(ADC2), F4(ADC3), VIS(ADC4), NIR(ADC5)
    //     measure_cycle(SMUX_C1, ch);
    //     sensor_data.f1     = ch[0]; // 405nm violet
    //     sensor_data.f2     = ch[1]; // 425nm indigo
    //     sensor_data.f3     = ch[2]; // 475nm blue
    //     sensor_data.f4     = ch[3]; // 515nm cyan
    //     sensor_data.vis_c1 = ch[4];
    //     sensor_data.nir    = ch[5]; // 855nm

    //     // Cycle 2: F6(ADC0), F7(ADC1), F8(ADC2), F5(ADC3), VIS(ADC4), NIR(ADC5)
    //     measure_cycle(SMUX_C2, ch);
    //     sensor_data.f6     = ch[0]; // 640nm red
    //     sensor_data.f7     = ch[1]; // 690nm deep red
    //     sensor_data.f8     = ch[2]; // 745nm far red
    //     sensor_data.f5     = ch[3]; // 550nm green-yellow
    //     sensor_data.vis_c2 = ch[4];

    //     // Cycle 3: FZ(ADC0), FY(ADC1), FXL(ADC2), NIR(ADC3), VIS(ADC4), FD(ADC5)
    //     measure_cycle(SMUX_C3, ch);
    //     sensor_data.fz     = ch[0]; // 450nm blue-violet
    //     sensor_data.fy     = ch[1]; // 555nm green
    //     sensor_data.fxl    = ch[2]; // 600nm orange
    //     sensor_data.vis_c3 = ch[4];
    //     sensor_data.fd     = ch[5];

    //     compute_lux();

    //     float vis_avg = (sensor_data.vis_c1 + sensor_data.vis_c2 + sensor_data.vis_c3) / 3.0f;

    //     ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    //     ESP_LOGI(TAG, "  LUX: %.1f  |  Gain: %.0fx  |  %s",
    //              sensor_data.lux, gain_mult(),
    //              sensor_data.saturated ? "⚠ SATURATED" : "OK");
    //     ESP_LOGI(TAG, "  ── Visible spectrum (ordered by wavelength) ────────");
    //     ESP_LOGI(TAG, "  F1  405nm violet     : %5d", sensor_data.f1);
    //     ESP_LOGI(TAG, "  F2  425nm indigo     : %5d", sensor_data.f2);
    //     ESP_LOGI(TAG, "  FZ  450nm blue-viol  : %5d", sensor_data.fz);
    //     ESP_LOGI(TAG, "  F3  475nm blue       : %5d", sensor_data.f3);
    //     ESP_LOGI(TAG, "  F4  515nm cyan       : %5d", sensor_data.f4);
    //     ESP_LOGI(TAG, "  F5  550nm grn-yellow : %5d", sensor_data.f5);
    //     ESP_LOGI(TAG, "  FY  555nm green      : %5d", sensor_data.fy);
    //     ESP_LOGI(TAG, "  FXL 600nm orange     : %5d", sensor_data.fxl);
    //     ESP_LOGI(TAG, "  F6  640nm red        : %5d", sensor_data.f6);
    //     ESP_LOGI(TAG, "  F7  690nm deep red   : %5d", sensor_data.f7);
    //     ESP_LOGI(TAG, "  F8  745nm far red    : %5d", sensor_data.f8);
    //     ESP_LOGI(TAG, "  ── Non-visible ─────────────────────────────────────");
    //     ESP_LOGI(TAG, "  NIR 855nm infrared   : %5d", sensor_data.nir);
    //     ESP_LOGI(TAG, "  VIS clear avg        : %.0f", vis_avg);
    //     ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    //     // TODO: BLE notify (επόμενο βήμα)

    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
}