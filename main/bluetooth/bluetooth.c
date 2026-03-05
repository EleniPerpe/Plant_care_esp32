#include "bluetooth.h"
#include "string.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/semphr.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/gap/ble_svc_gap.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* TAG = "Solar Panel Hub";
uint8_t own_addr_type;


// Πρόσθεσε αυτό το handle στην κορυφή του αρχείου σου για να το βρίσκει το Task
uint16_t light_val_handle;
extern uint16_t current_light_value;

static int gatt_svr_chr_access_light(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	int rc = os_mbuf_append(ctxt->om, &current_light_value, sizeof(current_light_value));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    // Αυτό επιτρέπει στο κινητό να κάνει "Read" την τελευταία τιμή αν θέλει
    return 0;
}

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
        .uuid = BLE_UUID16_DECLARE(0x180F), 
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A19), // LED Control
                .access_cb = device_write_callback,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A76), // Light Sensor Data
                .access_cb = gatt_svr_chr_access_light, // <--- ΟΧΙ NULL πλέον
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &light_val_handle,
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
    // 1. Καθαρισμός παλαιών ρυθμίσεων
    ble_gatts_reset();

    // 2. Αρχικοποίηση βασικών υπηρεσιών
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // 3. Καταμέτρηση και προσθήκη των δικών σου υπηρεσιών (gatt_svcs)
    int rc = ble_gatts_count_cfg(gatt_svcs);
    assert(rc == 0);

    rc = ble_gatts_add_svcs(gatt_svcs);
    assert(rc == 0);
}
