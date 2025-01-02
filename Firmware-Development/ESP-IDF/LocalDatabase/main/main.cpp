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
#include "Sensors.hpp"

static const char *TAG = "app_main";

#define CRITICAL_HEAP_VAL   10
#define REPORTING_TIME   20000
#define HEAP_CHECK_TIME  500

#define TEST_WITHOUT_APP 0

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
    Sensors::init();
    Device::init();         // Initializing database (and inserting values)

    DeviceProvisioning devProv;

    #if TEST_WITHOUT_APP
    Device::saveDomainName("timesp1");
    #endif

    ESP_LOGE(TAG, "%s", Device::getDomainName().c_str());
    
    devProv.start();
    
    ESP_LOGE(TAG, "Assigned Domain Name: %s", Device::getDomainName().c_str());
    MDNSService::start(Device::getDomainName().c_str());

    auto credentials = Device::getCredentials();
    ESP_LOGI(TAG, "Generated username: %s", credentials.first.c_str());
    ESP_LOGI(TAG, "Generated password: %s", credentials.second.c_str());

    UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
    userMan.createUser(credentials.first, credentials.second);

    Device::startDataAcquisitionTask();
    HttpServer::init(Device::getData);

    uint64_t tick = esp_timer_get_time();
    uint64_t tickHeap = 0;
    bool tickFlag = false;
    while(true)
    {
      vTaskDelay(10/portTICK_RATE_MS);

      uint32_t free_heap = esp_get_free_heap_size();
      uint32_t watermark_heap = esp_get_minimum_free_heap_size();
      if((((esp_timer_get_time()-tick)/1000) > REPORTING_TIME))
      {
        ESP_LOGI(TAG, "______________________________________");
        ESP_LOGI(TAG, "Free Heap Size: %d B     %.4f KB", free_heap, free_heap / 1024.0);
        ESP_LOGI(TAG, "Heap Watermark: %d B     %.4f KB", watermark_heap, watermark_heap / 1024.0);
        Storage::storageStats();
        ESP_LOGI(TAG, "``````````````````````````````````````");
        tick = esp_timer_get_time();
      }

      if((((esp_timer_get_time()-tickHeap)/1000) > HEAP_CHECK_TIME) && tickFlag)
      {
        if((free_heap/1024.0) < CRITICAL_HEAP_VAL)
        {
          ESP_LOGE(TAG, "Heap memory too low, restarting device.");
          esp_restart();
        }
        tickFlag = false;
      }

      if(((free_heap/1024.0) < CRITICAL_HEAP_VAL) && !tickFlag)
      {
        tickHeap =esp_timer_get_time();
        tickFlag = true;
      }
    }
}
