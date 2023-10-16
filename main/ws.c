#include "ws.h"
static const char *TAG = "WebSocket";
static SemaphoreHandle_t isConnected = NULL;

static const esp_websocket_client_config_t websocket_cfg = {
    .uri = "ws://128.23.71.80:3000",
};

esp_websocket_client_handle_t ws_client;

static void websocket_status(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WEBSOCKET_EVENT_CONNECTED)
    {
        xSemaphoreGive(isConnected);
    }
    else if (event_id == WEBSOCKET_EVENT_CONNECTED)
    {
        ESP_LOGE(TAG, "DisConnected");
    }
}

esp_err_t connect_ws(void)
{

    isConnected = xSemaphoreCreateBinary();
    if (isConnected == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    ws_client = esp_websocket_client_init(&websocket_cfg);
    ESP_ERROR_CHECK(esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_CONNECTED, websocket_status, (void *)ws_client));
    ESP_ERROR_CHECK(esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_DISCONNECTED, websocket_status, (void *)ws_client));
    esp_websocket_client_start(ws_client);
    ESP_LOGI(TAG, "Connecting...");
    xSemaphoreTake(isConnected, portMAX_DELAY);
    ESP_LOGI(TAG, "connected");
    return ESP_OK;
};