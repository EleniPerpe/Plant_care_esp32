#include "bluetooth.h"
#include "string.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/semphr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* TAG = "Solar Panel Hub";
uint8_t own_addr_type;

static int device_write_callback(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
    if (ctxt->om->om_len > 0) {
        char data = ctxt->om->om_data[0];
        
        if (data == '1') 
		{
			led_start_loop();
		} 
		else if (data == '0') 
		{
			led_stop_loop();
		}
    }
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180F), // Battery Service (παράδειγμα)
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A19),
                .access_cb = device_write_callback,
                .flags = BLE_GATT_CHR_F_WRITE, // Επιτρέπουμε το γράψιμο
            },
            {0}
        },
    },
    {0}
};

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Connected! Status: %d", event->connect.status);
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected! Reason: %d. Restarting advertising...", event->disconnect.reason);
            start_advertising();
            break;

        default:
            break;
    }
    return 0;
}

void start_advertising(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)"Solar Panel Hub";
    fields.name_len = strlen((char *)fields.name);
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting advertising; rc=%d", rc);
    }
}

void on_sync(void) {
    ble_hs_id_infer_auto(0, &own_addr_type);
    start_advertising();
}

void nimble_host_task(void *param) {
    ESP_LOGI(TAG, "NimBLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void bluetooth_init(void) {

	// led_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
}