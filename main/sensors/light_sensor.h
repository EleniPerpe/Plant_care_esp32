// #ifndef LIGHT_SENSOR_H
// #define LIGHT_SENSOR_H

// #include "driver/i2c_master.h"
// #include "bluetooth/bluetooth.h"

// #define I2C_MASTER_SCL_IO   2
// #define I2C_MASTER_SDA_IO   1
// #define I2C_MASTER_NUM      0
// #define AS7341_ADDR         0x39

// // AS7341 gain values (register 0xAA)
// #define GAIN_0_5X   0x00
// #define GAIN_1X     0x01
// #define GAIN_2X     0x02
// #define GAIN_4X     0x03
// #define GAIN_8X     0x04
// #define GAIN_16X    0x05
// #define GAIN_32X    0x06
// #define GAIN_64X    0x07
// #define GAIN_128X   0x08
// #define GAIN_256X   0x09
// #define GAIN_512X   0x0A

// #define SATURATION_THRESHOLD  58981   // 90% of 65535
// #define LOW_LIGHT_THRESHOLD   500

// typedef struct {
//     // Cycle 1: F1-F4, VIS, NIR
//     uint16_t f1;        // 405nm violet
//     uint16_t f2;        // 435nm indigo
//     uint16_t f3;        // 470nm blue
//     uint16_t f4;        // 515nm cyan
//     uint16_t vis_c1;
//     uint16_t nir;       // 855nm

//     // Cycle 2: F5-F8, VIS
//     uint16_t f5;        // 555nm green-yellow
//     uint16_t f6;        // 590nm orange
//     uint16_t f7;        // 630nm red
//     uint16_t f8;        // 680nm deep red
//     uint16_t vis_c2;

//     // Cycle 3: FZ, FY, FXL, VIS, FD
//     uint16_t fz;        // 450nm blue-violet
//     uint16_t fy;        // 555nm green
//     uint16_t fxl;       // 600nm orange
//     uint16_t vis_c3;
//     uint16_t fd;        // flicker detect

//     float    lux;
//     uint8_t  gain;
//     bool     saturated;
// } as7341_data_t;

// extern i2c_master_bus_handle_t bus_handle;
// extern i2c_master_dev_handle_t dev_handle;
// extern as7341_data_t sensor_data;

// void light_sensor_init(void);
// void light_sensor_task(void *pvParameters);

// #endif

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bluetooth/bluetooth.h"

#define I2C_MASTER_SCL_IO   2
#define I2C_MASTER_SDA_IO   1
#define I2C_MASTER_NUM      0
#define AS7341_ADDR         0x39

// AS7341 gain values (register 0xAA)
#define GAIN_0_5X   0x00
#define GAIN_1X     0x01
#define GAIN_2X     0x02
#define GAIN_4X     0x03
#define GAIN_8X     0x04
#define GAIN_16X    0x05
#define GAIN_32X    0x06
#define GAIN_64X    0x07
#define GAIN_128X   0x08
#define GAIN_256X   0x09
#define GAIN_512X   0x0A

#define SATURATION_THRESHOLD  58981   // 90% of 65535
#define LOW_LIGHT_THRESHOLD   500

typedef struct {
    uint16_t f1;        // 415nm violet
    uint16_t f2;        // 435nm indigo
    uint16_t f3;        // 470nm blue
    uint16_t f4;        // 515nm cyan
    uint16_t f5;        // 555nm green
    uint16_t f6;        // 590nm orange
    uint16_t f7;        // 630nm red
    uint16_t f8;        // 680nm deep red
    uint16_t vis_c1;
    uint16_t vis_c2;
    uint16_t nir;       // 855nm
    float    lux;
    uint8_t  gain;
    bool     saturated;
} as7341_data_t;

extern i2c_master_bus_handle_t bus_handle;
extern i2c_master_dev_handle_t dev_handle;
extern as7341_data_t sensor_data;

void light_sensor_init(void);
void light_sensor_task(void *pvParameters);

#endif