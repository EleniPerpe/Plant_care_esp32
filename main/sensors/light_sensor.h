#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "driver/i2c_master.h"
#include "bluetooth/bluetooth.h"

#define I2C_MASTER_SCL_IO           2      
#define I2C_MASTER_SDA_IO           1      
#define I2C_MASTER_NUM              0      
#define AS7341_ADDR                 0x39   

extern i2c_master_bus_handle_t bus_handle;
extern i2c_master_dev_handle_t dev_handle;

void light_sensor_init();
void light_sensor_task(void *pvParameters);

#endif