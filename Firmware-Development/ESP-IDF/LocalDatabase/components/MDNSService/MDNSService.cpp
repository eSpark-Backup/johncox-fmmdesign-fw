#include "MDNSService.hpp"
#include <esp_log.h>
#include <mdns.h>

static const char* TAG = "MDNSService";

esp_err_t MDNSService::start(const char* hostName) 
{
    esp_err_t ret = mdns_init();
    if (ret) 
    {
        ESP_LOGE(TAG, "Failed to initialize mDNS: %d", ret);
        return ret;
    }

    mdns_hostname_set(hostName);
    mdns_instance_name_set(hostName);
    mdns_service_add(NULL, "_websocket", "_tcp", 80, NULL, 0);

    ESP_LOGI(TAG, "mDNS Service Started");
    return ret;
}