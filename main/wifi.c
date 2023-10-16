#include "wifi.h"
static const char *TAG = "WIFI";
static SemaphoreHandle_t isConnected = NULL;
// #define EAP_ID "xyz@al-ghurair.com"
// #define EAP_USERNAME "roshan.dev"
// #define EAP_PASSWORD "password"

static void wifiEventHandler(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xSemaphoreGive(isConnected);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGE(TAG, "Disconnected");
    }
}

esp_err_t connect_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    isConnected = xSemaphoreCreateBinary();
    if (isConnected == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, isConnected));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifiEventHandler, isConnected));
    // ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Shibu",
            .password = "m@dhu#15",
            // .ssid = "AG-BYOD",
            // .password = "pass",

            .scan_method = WIFI_FAST_SCAN,
            // .bssid_set = false,
            .failure_retry_cnt = 5}
            };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID)) );
    // ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME)) );
    // ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD)) );
    // ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_enable() );
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "Connecting...");

    xSemaphoreTake(isConnected, portMAX_DELAY);

    ESP_LOGI(TAG, "connected");
    return ESP_OK;
}