// // #ifndef LIGHT_SENSOR_H
// // #define LIGHT_SENSOR_H

// // #include "driver/i2c_master.h"
// // #include "bluetooth/bluetooth.h"

// // #define I2C_MASTER_SCL_IO           2      
// // #define I2C_MASTER_SDA_IO           1      
// // #define I2C_MASTER_NUM              0      
// // #define AS7343_ADDR                 0x39    

// // extern i2c_master_bus_handle_t bus_handle;
// // extern i2c_master_dev_handle_t dev_handle;

// // void light_sensor_init();
// // void light_sensor_task(void *pvParameters);
// // void i2c_bus_recovery();

// // #endif

// #ifndef LIGHT_SENSOR_H
// #define LIGHT_SENSOR_H

// #include "driver/i2c_master.h"
// #include "bluetooth/bluetooth.h"

// #define I2C_MASTER_SCL_IO   2
// #define I2C_MASTER_SDA_IO   1
// #define I2C_MASTER_NUM      0
// #define AS7343_ADDR         0x39

// // GAIN levels (register 0xAA values)
// // Each step = ~2x sensitivity
// #define AS7343_GAIN_0_5X    0x00
// #define AS7343_GAIN_1X      0x01
// #define AS7343_GAIN_2X      0x02
// #define AS7343_GAIN_4X      0x03
// #define AS7343_GAIN_8X      0x04
// #define AS7343_GAIN_16X     0x05
// #define AS7343_GAIN_32X     0x06
// #define AS7343_GAIN_64X     0x07
// #define AS7343_GAIN_128X    0x08
// #define AS7343_GAIN_256X    0x09
// #define AS7343_GAIN_512X    0x0A
// #define AS7343_GAIN_1024X   0x0B
// #define AS7343_GAIN_2048X   0x0C

// // Saturation threshold — above this we consider a channel saturated
// // Max ADC value is 65535, we use 90% as the threshold
// #define SATURATION_THRESHOLD    54981
// // #define SATURATION_THRESHOLD    58981

// // Too-dark threshold — below this on the Clear channel we boost gain
// #define LOW_LIGHT_THRESHOLD     1000

// // All 14 spectral channels + Clear + FD (flicker detect)
// // Ordered by wavelength for the spectrum diagram
// typedef struct {
//     // Cycle 1: FZ, FY, FXL, NIR, Clear, FD
//     uint16_t fz;        // ~450nm  (blue-violet XYZ)
//     uint16_t fy;        // ~555nm  (green XYZ)
//     uint16_t fxl;       // ~600nm  (orange-red XYZ)
//     uint16_t nir;       // ~855nm  (near infrared)
//     uint16_t clear1;    // broadband visible (cycle 1)
//     uint16_t fd;        // flicker detect

//     // Cycle 2: F2, F3, F4, F6
//     uint16_t f2;        // ~435nm  (violet-blue)
//     uint16_t f3;        // ~475nm  (blue)
//     uint16_t f4;        // ~515nm  (green)
//     uint16_t f6;        // ~640nm  (orange-red)
//     uint16_t clear2;    // broadband visible (cycle 2)

//     // Cycle 3: F1, F5, F7, F8
//     uint16_t f1;        // ~415nm  (violet)
//     uint16_t f5;        // ~555nm  (green-yellow)
//     uint16_t f7;        // ~700nm  (red)
//     uint16_t f8;        // ~745nm  (deep red / NIR boundary)
//     uint16_t clear3;    // broadband visible (cycle 3)

//     // Computed
//     float    lux;       // calculated illuminance
//     uint8_t  gain;      // current gain register value
//     bool     saturated; // true if any channel hit the ceiling
// } as7343_data_t;

// extern i2c_master_bus_handle_t bus_handle;
// extern i2c_master_dev_handle_t dev_handle;
// extern as7343_data_t sensor_data;

// void light_sensor_init(void);
// void light_sensor_task(void *pvParameters);

// #endif

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "driver/i2c_master.h"
#include "bluetooth/bluetooth.h"

#define I2C_MASTER_SCL_IO   2
#define I2C_MASTER_SDA_IO   1
#define I2C_MASTER_NUM      0
#define AS7343_ADDR         0x39

#define GAIN_0_5X    0x00
#define GAIN_1X      0x01
#define GAIN_2X      0x02
#define GAIN_4X      0x03
#define GAIN_8X      0x04
#define GAIN_16X     0x05
#define GAIN_32X     0x06
#define GAIN_64X     0x07
#define GAIN_128X    0x08
#define GAIN_256X    0x09
#define GAIN_512X    0x0A
#define GAIN_1024X   0x0B
#define GAIN_2048X   0x0C

#define SATURATION_THRESHOLD  58981
#define LOW_LIGHT_THRESHOLD   500

typedef struct {
    // Cycle 1: F1-F4 + VIS + NIR
    uint16_t f1;       // 405nm violet
    uint16_t f2;       // 425nm indigo
    uint16_t f3;       // 475nm blue
    uint16_t f4;       // 515nm cyan
    uint16_t vis_c1;   // clear broadband
    uint16_t nir;      // 855nm infrared

    // Cycle 2: F5-F8 + VIS
    uint16_t f5;       // 550nm green-yellow
    uint16_t f6;       // 640nm red
    uint16_t f7;       // 690nm deep red
    uint16_t f8;       // 745nm far red
    uint16_t vis_c2;

    // Cycle 3: FZ, FY, FXL + VIS + FD
    uint16_t fz;       // 450nm blue-violet
    uint16_t fy;       // 555nm green
    uint16_t fxl;      // 600nm orange
    uint16_t vis_c3;
    uint16_t fd;       // flicker detect

    // Computed
    float    lux;
    uint8_t  gain;
    bool     saturated;
} as7343_data_t;

extern i2c_master_bus_handle_t bus_handle;
extern i2c_master_dev_handle_t dev_handle;
extern as7343_data_t sensor_data;

void light_sensor_init(void);
void light_sensor_task(void *pvParameters);

#endif