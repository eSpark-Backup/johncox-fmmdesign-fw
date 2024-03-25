#include <stdio.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "LocalDB.hpp"
#include "Storage.hpp"
#include "HttpServer.hpp"
#include "UserManager.hpp"
#include "Device.hpp"
// #include "wifi.h"
#include "CustomJWT.h"
#include "JCErrors.h"
#include "DeviceProvisioning.hpp"
#include "MDNSService.hpp"

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
    
    Storage::init();
    LocalDB& db = LocalDB::getInstance();
    db.init();
    if (!db.open()) {
        return;
    }
    testDatabases();

    DeviceProvisioning devProv;
    devProv.start();
    MDNSService::start("timesp1");

    vTaskDelay(3000/portTICK_RATE_MS);
    auto credentials = Device::getCredentials();
    ESP_LOGI(TAG, "Generated username: %s", credentials.first.c_str());
    ESP_LOGI(TAG, "Generated password: %s", credentials.second.c_str());

    UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
    userMan.createUser(credentials.first, credentials.second);

    HttpServer::start();
    
}
