#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "LocalDB.hpp"
#include "Storage.hpp"
#include "HttpServer.h"
#include "wifi.h"

static const char *TAG = "app_main";

extern "C" void app_main() 
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Storage::init();
    // testDatabases();
    // Storage::deinit();
    wifi_init_sta();
    httpsServerStart();
    ESP_LOGI(TAG, "SPIFFS unmounted");
}
