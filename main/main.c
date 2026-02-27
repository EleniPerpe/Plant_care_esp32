#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "bluetooth/bluetooth.h"

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nimble_port_init();
	
	bluetooth_init();

    ble_svc_gap_init();
    ble_svc_gap_device_name_set("Solar_Panel_Hub");

    ble_hs_cfg.sync_cb = on_sync;

    nimble_port_freertos_init(nimble_host_task);
}
