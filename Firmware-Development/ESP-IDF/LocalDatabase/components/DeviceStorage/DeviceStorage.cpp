#include "DeviceStorage.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "DeviceStorage";

esp_err_t DeviceStorage::saveDomainName(const std::string& domainName) {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS Flash initialization failed");
        return err;
    }

    nvs_handle_t nvsHandle;
    err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS");
        return err;
    }

    err = nvs_set_str(nvsHandle, "domain_name", domainName.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting domain name to NVS");
        nvs_close(nvsHandle);
        return err;
    }

    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing domain name to NVS");
        nvs_close(nvsHandle);
        return err;
    }

    nvs_close(nvsHandle);
    return ESP_OK;
}

std::string DeviceStorage::getDomainName() {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS Flash initialization failed");
        return "";
    }

    nvs_handle_t nvsHandle;
    err = nvs_open("storage", NVS_READONLY, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS");
        nvs_close(nvsHandle);
        return "";
    }

    char domainName[60]; 
    size_t len = sizeof(domainName);
    err = nvs_get_str(nvsHandle, "domain_name", domainName, &len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error retrieving domain name from NVS");
        nvs_close(nvsHandle);
        return "";
    }

    nvs_close(nvsHandle);
    return std::string(domainName);
}
