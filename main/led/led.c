#include "led.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "LED";
static led_strip_handle_t led_strip = NULL;
static SemaphoreHandle_t led_mutex = NULL;
static bool led_loop_enabled = false;

#define LED_PIN 38

void led_loop_task(void *pvParameters) {
    uint8_t colors[][3] = {
        {255, 0, 0}, {255, 165, 0}, {0, 255, 0}, {0, 0, 255}, {255, 0, 255}
    };
    int num_colors = sizeof(colors) / sizeof(colors[0]);

    while (1) {
        if (led_loop_enabled) {
            for (int i = 0; i < num_colors && led_loop_enabled; i++) {
                if (xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    led_strip_set_pixel(led_strip, 0, colors[i][0], colors[i][1], colors[i][2]);
                    led_strip_refresh(led_strip);
                    xSemaphoreGive(led_mutex);
                }
                vTaskDelay(pdMS_TO_TICKS(400));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void led_init(void) {
    led_strip_config_t strip_config = { .strip_gpio_num = LED_PIN, .max_leds = 1 };
    led_strip_rmt_config_t rmt_config = { .resolution_hz = 10 * 1000 * 1000, .flags.with_dma = true };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);

    led_mutex = xSemaphoreCreateMutex();
    xTaskCreate(led_loop_task, "led_loop_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LED module initialized");
}

void led_start_loop(void) { led_loop_enabled = true; }

void led_stop_loop(void) {
    led_loop_enabled = false;
    if (xSemaphoreTake(led_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        led_strip_clear(led_strip);
        led_strip_refresh(led_strip);
        xSemaphoreGive(led_mutex);
    }
}