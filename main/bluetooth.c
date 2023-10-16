#include "bluetooth.h"

static const char *BT_HF_TAG = "Bluetooth: ";
#define MobileBT "sync_b"
#define OurBT "ESP32"

static esp_bd_addr_t peer_addr = {0};
static char peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
static uint8_t peer_bdname_len;

#define ESP_HFP_RINGBUF_SIZE 3600
static RingbufHandle_t m_rb = NULL;

static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir)
    {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname)
    {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname)
    {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
        {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname)
        {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len)
        {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

void confirmWSAndFreeMemo(void *arg)
{
            size_t item_size = 0;
    while (1)
    {
        if (m_rb)
        {
            uint8_t *data = xRingbufferReceiveUpTo(m_rb, &item_size, 0, 120);
            int done = esp_websocket_client_send_bin(ws_client, (const char *)data, item_size, portMAX_DELAY);
            if(done > 0){
            vRingbufferReturnItem(m_rb, data);
            }
            item_size = 0;
            vTaskDelay(50);
        }
    }

    vTaskDelete(NULL);

}

void incoming_cb(const uint8_t *buf, uint32_t sz)
{
    if (!m_rb)
    {
        return;
    };
    int done = esp_websocket_client_send_bin(ws_client, (const char *)buf, sz, portMAX_DELAY);
    if(done < 1){
        ESP_LOGE(BT_HF_TAG, "Failed");
    }
    // BaseType_t done = xRingbufferSend(m_rb, (uint8_t *)buf, sz, 0);
    // if (!done)
    // {
    //     ESP_LOGE(BT_HF_TAG, "rb send fail");
    // }

    esp_hf_client_outgoing_data_ready();
}
uint32_t outgoing_cb(uint8_t *p_buf, uint32_t sz)
{
    if (!m_rb)
    {
        return 0;
    }
    return 0;
    //     size_t item_size = 0;
    // uint8_t *data = xRingbufferReceiveUpTo(m_rb, &item_size, 0, sz);
    // if (item_size == sz) {
    //     ESP_LOGI(BT_HF_TAG, "size is %lu", sz);
    //     memcpy(p_buf, data, item_size);
    //     vRingbufferReturnItem(m_rb, data);
    //     return sz;
    // } else if (0 < item_size) {
    //     vRingbufferReturnItem(m_rb, data);
    //     return 0;
    // } else {
    //     // data not enough, do not read
    //     return 0;
    // }
}

void start_discovery()
{
    ESP_LOGI(BT_HF_TAG, "Starting device discovery...");
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
}

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_DISC_RES_EVT:
    {
        for (int i = 0; i < param->disc_res.num_prop; i++)
        {
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR && get_name_from_eir((uint8_t *)param->disc_res.prop[i].val, peer_bdname, &peer_bdname_len))
            {
                ESP_LOGI(BT_HF_TAG, "%s", peer_bdname);
                if (strcmp(peer_bdname, MobileBT) == 0)
                {
                    memcpy(peer_addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
                    ESP_LOGI(BT_HF_TAG, "Found a target device address: \n");
                    ESP_LOGI(BT_HF_TAG, "Connecting....\n");
                    esp_hf_client_connect(peer_addr);
                    ESP_LOGI(BT_HF_TAG, "Connected....\n");
                    esp_hf_client_connect_audio(peer_addr);
                    ESP_LOGI(BT_HF_TAG, "Audio Connected....\n");
                    esp_bt_gap_cancel_discovery();
                }
            }
        }
        break;
    }

    // case ESP_BT_GAP_AUTH_CMPL_EVT:
    // {
    //     if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
    //     {
    //         ESP_LOGI(BT_HF_TAG, "authentication success: %s", param->auth_cmpl.device_name);
    //         esp_log_buffer_hex(BT_HF_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
    //     }
    //     else
    //     {
    //         ESP_LOGE(BT_HF_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
    //     }
    //     break;
    // }
    // case ESP_BT_GAP_PIN_REQ_EVT:
    // {
    //     ESP_LOGI(BT_HF_TAG, "BT Pin Requested min_16_digit:%d", param->pin_req.min_16_digit);
    //     if (param->pin_req.min_16_digit)
    //     {
    //         ESP_LOGI(BT_HF_TAG, "Input pin code: 0000 0000 0000 0000");
    //         esp_bt_pin_code_t pin_code = {0};
    //         esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
    //     }
    //     else
    //     {
    //         ESP_LOGI(BT_HF_TAG, "Input pin code: 1234");
    //         esp_bt_pin_code_t pin_code;
    //         pin_code[0] = '1';
    //         pin_code[1] = '2';
    //         pin_code[2] = '3';
    //         pin_code[3] = '4';
    //         esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
    //     }
    //     break;
    // }
    default:
        break;
    }
}

void hf_client_event_handler(esp_hf_client_cb_event_t event, esp_hf_client_cb_param_t *param)
{
    if (event == ESP_HF_CLIENT_CONNECTION_STATE_EVT && param->conn_stat.state == 0)
    {
        ESP_LOGI(BT_HF_TAG, "State : Disconnected");
        start_discovery();
    }

    if (event == ESP_HF_CLIENT_AUDIO_STATE_EVT)
    {
        ESP_LOGI(BT_HF_TAG, "Audio state receiving data");
        if (param->audio_stat.state == ESP_HF_CLIENT_AUDIO_STATE_CONNECTED ||
            param->audio_stat.state == ESP_HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC)
        {
            // xTaskCreate(confirmWSAndFreeMemo, "udp_receive_task", 4096, NULL, 5, NULL);
            m_rb = xRingbufferCreate(ESP_HFP_RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
            esp_hf_client_register_data_callback(incoming_cb, outgoing_cb);
        }
    }
}

void bluetooth_begin()
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_bt_dev_set_device_name(OurBT);
    esp_bt_gap_register_callback(gap_callback);
    esp_hf_client_register_callback(hf_client_event_handler);
    esp_hf_client_init();

    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '0';
    pin_code[1] = '0';
    pin_code[2] = '0';
    pin_code[3] = '0';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    /* set discoverable and connectable mode, wait to be connected */
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    start_discovery();
}