#pragma once
#include "common.h"
#include "esp_websocket_client.h"

esp_err_t connect_ws(void);

extern esp_websocket_client_handle_t ws_client;