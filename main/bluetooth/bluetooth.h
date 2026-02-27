#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>
#include "esp_log.h"

/* NimBLE Includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#include "led/led.h"

extern const char *TAG;
extern uint8_t own_addr_type;

void start_advertising(void);
void on_sync(void);
void nimble_host_task(void *param);
void bluetooth_init(void);

#endif