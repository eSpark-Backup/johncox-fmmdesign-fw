#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "LocalDB.hpp"
#include "Storage.hpp"

static const char *TAG = "app_main";

extern "C" void app_main() 
{
    Storage::init();
    testDatabases();
    Storage::deinit();
    ESP_LOGI(TAG, "SPIFFS unmounted");
}
