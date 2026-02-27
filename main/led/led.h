#ifndef LED_H
#define LED_H

#include "esp_err.h"

void led_init(void);
void led_start_loop(void);
void led_stop_loop(void);

#endif