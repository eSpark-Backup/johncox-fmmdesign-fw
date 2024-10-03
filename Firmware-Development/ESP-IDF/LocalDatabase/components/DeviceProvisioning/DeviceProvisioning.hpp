#pragma once
#ifndef DEV_PROV_HPP
#define DEV_PROV_HPP
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <wifi_provisioning/scheme_softap.h>
#include <wifi_provisioning/manager.h>

#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_SOFTAP "softap"

#define CONFIG_EXAMPLE_PROV_TRANSPORT_SOFTAP 1
#define CONFIG_EXAMPLE_PROV_TRANSPORT 2
// #define CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE 1
#define CONFIG_PROV_MGR_MAX_RETRY_CNT 10
#define CONFIG_PROV_MGR_MAX_RETRY_CNT_DELAY 1500
#define CONFIG_EXAMPLE_RESET_PROVISIONED 1

typedef enum
{
    WIFI_PROVISIONING = 1,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_RETRY,
    WIFI_DEFAULT
}wifi_state_e;

typedef enum
{
    WIFI_PROVISION = 0,
    WIFI_CHECK_PROVISION,
    WIFI_CONNECT,
    WIFI_ENUM_MAX
}wifi_operation_e;

typedef struct {
    wifi_operation_e wifi_enums;
    const char* str;
}wifi_opt_with_str;


class DeviceProvisioning
{
    public:
        void start(void);
        static esp_err_t reset(void);
        static void provisioningTask(void *pvParameters);
        
    private:
        // static wifi_config_t wifi_backup_cfg;
        static void eventHandler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data);

        static void wifiInitSTA(void);
        static void wifiConnSTA(void);
        static void provMgrStart(void);
        static void saveWiFiCredentials(void);
        static void setWiFiCredentials(void);
        static void sendQueue(uint8_t option, QueueHandle_t queue_to_push, const char * calling_func);
        static void getDeviceServiceName(char *service_name, size_t max);
        static esp_err_t domainNameDataHandler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data);
    
};
#endif //DEV_PROV_HPP