#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>
#include "esp_log.h"
#include "led/led.h"
#include "string.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/semphr.h"
/* NimBLE Includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gatt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern uint16_t light_val_handle;
extern uint16_t current_light_value;

void start_advertising(void);
void on_sync(void);
void nimble_host_task(void *param);
void bluetooth_init(void);

#endif