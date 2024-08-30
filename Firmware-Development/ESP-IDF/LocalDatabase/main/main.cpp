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
#include "CustomJWT.h"
#include "JCErrors.h"
#include "DeviceProvisioning.hpp"
#include "MDNSService.hpp"
#include "Websockets.hpp"

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
    
    Storage::init();        // Initializing storage (SPIFF/NAND)
    Device::init();         // Initializing database (and inserting values)

    DeviceProvisioning devProv;

    // Device::saveDomainName("timesp1");
    ESP_LOGE(TAG, "%s", Device::getDomainName().c_str());
    
    devProv.start();
    if (Device::getDomainName().empty())
    {
      ESP_LOGW(TAG, "Domain name not found, restarting device.");
      esp_restart();
    }
    ESP_LOGE(TAG, "Assigned Domain Name: %s", Device::getDomainName().c_str());
    MDNSService::start(Device::getDomainName().c_str());

    auto credentials = Device::getCredentials();
    ESP_LOGI(TAG, "Generated username: %s", credentials.first.c_str());
    ESP_LOGI(TAG, "Generated password: %s", credentials.second.c_str());

    UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
    userMan.createUser(credentials.first, credentials.second);

    HttpServer::start(Device::getData);

    // static const char* filename = "/spiffs/test.db";
    // LocalDB& db = LocalDB::getInstance(filename);

    //   Device::insertHardCodeValues(db);
    // while(1) {
    //   vTaskDelay(1000 / portTICK_PERIOD_MS);
    //   const char* data = Device::getData();
    //   ESP_LOGI(TAG, "database entry: %s", data);
    // }
}
