#pragma once
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
#define CONFIG_EXAMPLE_RESET_PROVISIONED 1

typedef enum
{
    WIFI_PROVISION = 1,
    WIFI_CHECK_PROVISION,
    WIFI_CONNECT
}wifi_operation_e;

class DeviceProvisioning
{
    public:
        void start(void);
        void reset(void);
        static void provisioningTask(void *pvParameters);
        
    private:
        static void eventHandler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data);

        static void wifiInitSTA(void);
        static void wifiConnSTA(void);
        static void provMgrStart(void);
        static void getDeviceServiceName(char *service_name, size_t max);
        static esp_err_t domainNameDataHandler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data);
    
};
