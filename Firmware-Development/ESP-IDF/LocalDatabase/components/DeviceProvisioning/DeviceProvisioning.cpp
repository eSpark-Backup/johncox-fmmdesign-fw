#include "DeviceProvisioning.hpp"
#include "JCJson.hpp"
#include "Device.hpp"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "HttpServer.hpp"

const int WIFI_CONNECTED_EVENT = BIT0;
const int PROV_END_EVENT = BIT1;
static const char *TAG = "DeviceProvisioning";

static EventGroupHandle_t wifi_event_group;
static bool is_provisioned = false;
static wifi_state_e wifi_state = WIFI_DEFAULT;

static QueueHandle_t wifi_prov_queue = NULL;
static wifi_config_t wifi_backup_cfg;

/* Event handler for catching system events */
void DeviceProvisioning::eventHandler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
            case WIFI_PROV_START:
                xEventGroupClearBits(wifi_event_group, PROV_END_EVENT);
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV:
            {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                wifi_sta_cfg->failure_retry_cnt = CONFIG_PROV_MGR_MAX_RETRY_CNT;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                            "\n\tSSID         : %s\n\tPassword     : %s\n\tRetry Count  : %d",
                        (const char *)wifi_sta_cfg->ssid,
                        (const char *)wifi_sta_cfg->password,
                        wifi_sta_cfg->failure_retry_cnt
                        );

                wifi_config_t confg
                {
                    .sta = *wifi_sta_cfg
                };
                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &confg));
                break;
            }
            case WIFI_PROV_CRED_FAIL:
            {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s",
                        (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed." : "Wi-Fi access-point not found.");
                
                if(!is_provisioned)
                {
                    ESP_LOGW(TAG, "Please retry provisioning.");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                }
                else if (is_provisioned)
                {
                    wifi_prov_mgr_deinit();
                    // xEventGroupSetBits(wifi_event_group, PROV_END_EVENT);
                    setWiFiCredentials();
                    ESP_LOGW(TAG, "Fetching wifi credentials from NVS and attempting connection to AP.");
                    sendQueue(WIFI_CHECK_PROVISION, wifi_prov_queue, __func__);
                }
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                is_provisioned = true;
                break;
            case WIFI_PROV_END:
            {
                /* De-initialize manager once provisioning is finished */
                ESP_LOGI(TAG, "Event: WIFI_PROV_END");
                wifi_prov_mgr_deinit();
                // wifi_state = WIFI_CONNECTED;
                // xEventGroupSetBits(wifi_event_group, PROV_END_EVENT);
                break;
            }
            case WIFI_PROV_DEINIT:
            {
                xEventGroupSetBits(wifi_event_group, PROV_END_EVENT);
                ESP_LOGI(TAG, "Event: WIFI_PROV_DEINIT");
                HttpServer::start();
                if (!Device::isDomainNameValid())
                {
                    ESP_LOGW(TAG, "Domain name is invalid, restarting provisioning sequence.");
                    reset();
                }
                break;
            }
            default:
            {
                ESP_LOGW(TAG, "event_id %d is not implemented for %s", event_id, event_base);
                break;
            }
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
            case IP_EVENT_STA_GOT_IP:
            {
                wifi_state = WIFI_CONNECTED;
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
                
                // Change wifi connection failure count back to 0
                wifi_config_t confg;
                esp_wifi_get_config(WIFI_IF_STA, &confg);
                confg.sta.failure_retry_cnt = 0;
                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &confg));
                
                /* Signal main application to continue execution */
                xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
                break;
            }
            default:
            {
                ESP_LOGW(TAG, "event_id %d is not implemented for %s", event_id, event_base);
                break;
            }
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
            {
                ESP_LOGI(TAG, "Event: WIFI_EVENT_STA_START");
                if (!is_provisioned && WIFI_PROVISIONING != wifi_state)
                {
                    sendQueue(WIFI_PROVISION, wifi_prov_queue, __func__);
                }
                else if (is_provisioned && WIFI_DISCONNECTED == wifi_state)
                {
                    ESP_LOGI(TAG, "Disconnected. Connecting to the AP.");
                    sendQueue(WIFI_CONNECT, wifi_prov_queue, __func__);
                }
                wifi_state = WIFI_CONNECTING;
                break;
            }
            case WIFI_EVENT_STA_CONNECTED:
            {
                ESP_LOGI(TAG, "Event: WIFI_EVENT_STA_CONNECTED");
                // wifi_state = WIFI_CONNECTED;
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_EVENT);
                ESP_LOGI(TAG, "Event: WIFI_EVENT_STA_DISCONNECTED");

                if (!is_provisioned && (WIFI_PROVISIONING != wifi_state))
                {
                    // sendQueue(WIFI_PROVISION, wifi_prov_queue, __func__);
                }
                else if (WIFI_CONNECTED == wifi_state)
                {
                    ESP_LOGI(TAG, "Disconnected. Connecting to the AP.");
                    sendQueue(WIFI_CONNECT, wifi_prov_queue, __func__);
                }
                wifi_state = WIFI_DISCONNECTED;
                break;
            }
            default:
            {
                ESP_LOGW(TAG, "event_id %d is not implemented for %s", event_id, event_base);
                break;
            }
        }
    }
}

void DeviceProvisioning::wifiInitSTA(void)
{
    /* Start Wi-Fi in station mode */
    esp_wifi_stop();        // stop the wifi if its running
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void DeviceProvisioning::wifiConnSTA(void)
{
    // wifi_state = WIFI_DISCONNECTED;
    esp_err_t err;
    wifi_ap_record_t ap_info;
    uint8_t retry_count = CONFIG_PROV_MGR_MAX_RETRY_CNT;
    do
    {
        // ESP_LOGI(TAG, "Attempting connection with wifi.");
        esp_wifi_connect();
        err = esp_wifi_sta_get_ap_info(&ap_info);
        // ESP_LOGW(TAG, "error %d", err);
        retry_count--;
        vTaskDelay(CONFIG_PROV_MGR_MAX_RETRY_CNT_DELAY/portTICK_RATE_MS);
    } while (retry_count && ESP_OK != err);

    if(ESP_OK == err)
    {
        ESP_LOGI(TAG, "Connected to wifi.");
        // wifi_state = WIFI_CONNECTED;
    }
    else
    {
        ESP_LOGE(TAG, "Re-Connection Timed out, Activating AP Provisioning mode.");
        sendQueue(WIFI_PROVISION, wifi_prov_queue, __func__);
        wifi_state = WIFI_PROVISIONING;
    }
}

void DeviceProvisioning::getDeviceServiceName(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "TIMESP_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

// Parse "type" field from incoming JSON
static esp_err_t parseJsonType(const uint8_t *inbuf, ssize_t inlen, char **type) {
    if (!inbuf || inlen <= 0 || !type) {
        ESP_LOGE(TAG, "Invalid input parameters");
        return ESP_ERR_INVALID_ARG;
    }

    std::map<std::string, std::variant<std::string, int>> keyValues = JCJson::parse(reinterpret_cast<const char*>(inbuf));
    if (keyValues.find("type") == keyValues.end() || !std::holds_alternative<std::string>(keyValues["type"])) {
        ESP_LOGE(TAG, "Invalid JSON format: 'type' field is missing or not a string");
        return ESP_ERR_INVALID_ARG;
    }

    *type = strdup(std::get<std::string>(keyValues["type"]).c_str());

    return ESP_OK;
}

// Handle domain request
static esp_err_t handleDomainRequest(uint8_t **outbuf, ssize_t *outlen) 
{
    // Get the previously assigned domain name (if available)
    std::string previously_assigned_domain = Device::getDomainName(); // Your implementation to retrieve previously assigned domain

    // Create JSON response
    std::map<std::string, std::variant<std::string, int>> response_keyValues;
    response_keyValues["type"] = std::string("domain_response");
    response_keyValues["domain_name"] = previously_assigned_domain.c_str();
    response_keyValues["mac_address"] = Device::getMAC();

    char* response_str = JCJson::create(response_keyValues);

    // Set response data
    *outbuf = reinterpret_cast<uint8_t *>(response_str);
    *outlen = strlen(response_str);

    return ESP_OK;
}

// Handle domain assignment
static esp_err_t handleDomainAssignment(const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen) 
{
    std::map<std::string, std::variant<std::string, int>> keyValues = JCJson::parse(reinterpret_cast<const char*>(inbuf));
    if (keyValues.find("domain_name") == keyValues.end() || !std::holds_alternative<std::string>(keyValues["domain_name"])) {
        ESP_LOGE(TAG, "Invalid JSON format: 'domain_name' field is missing or not a string");
        return ESP_ERR_INVALID_ARG;
    }

    std::string assigned_domain = std::get<std::string>(keyValues["domain_name"]);

    // Perform domain assignment (your implementation)
    esp_err_t assignment_result = Device::saveDomainName(assigned_domain);

    // Create JSON response
    std::map<std::string, std::variant<std::string, int>> response_keyValues;
    response_keyValues["type"] = std::string("domain_assignment_response");
    response_keyValues["mac_address"] = Device::getMAC();
    response_keyValues["status"] = (assignment_result == ESP_OK) ? "success" : "fail";

    char* response_str = JCJson::create(response_keyValues);

    // Set response data
    *outbuf = reinterpret_cast<uint8_t *>(response_str);
    *outlen = strlen(response_str);

    return ESP_OK;
}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
esp_err_t DeviceProvisioning::domainNameDataHandler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                   uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    #if 1
    char *type = NULL;

    // Parse "type" field from incoming JSON
    esp_err_t err = parseJsonType(inbuf, inlen, &type);
    if (err != ESP_OK) {
        return err;
    }

    // Process based on the type of request
    if (strcmp(type, "domain_request") == 0) 
    {
        err = handleDomainRequest(outbuf, outlen);
    } 
    else if (strcmp(type, "domain_assignment") == 0) 
    {
        err = handleDomainAssignment(inbuf, inlen, outbuf, outlen);
    } 
    else 
    {
        ESP_LOGE(TAG, "Invalid 'type' field in JSON: %s", type);
        err = ESP_ERR_INVALID_ARG;
    }

    free(type);
    return err;
    #else
    if (inbuf)
    {
        ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL)
    {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
    #endif
}

#define to_string(x) #x

wifi_opt_with_str wifi_enum_str[WIFI_ENUM_MAX] = {
    {.wifi_enums = WIFI_PROVISION, .str = to_string(WIFI_PROVISION)},
    {.wifi_enums = WIFI_CHECK_PROVISION, .str = to_string(WIFI_CHECK_PROVISION)},
    {.wifi_enums = WIFI_CONNECT, .str = to_string(WIFI_CONNECT)}
};

void DeviceProvisioning::sendQueue(uint8_t option, QueueHandle_t queue_to_push, const char *calling_func)
{
    ESP_LOGI(TAG, "%s: Writing %s to queue from %s.", __func__, wifi_enum_str[option].str, calling_func);
    if(pdPASS != xQueueSend(queue_to_push, ( void * ) &option, ( TickType_t ) 10 ))
    {
        ESP_LOGW(TAG, "Failed to write %s in Queue", wifi_enum_str[option].str);
    }
    else
    {
        ESP_LOGI(TAG, "%s written to queue", wifi_enum_str[option].str);
    }
}

void DeviceProvisioning::start(void)
{
    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP, and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
#ifdef CONFIG_EXAMPLE_PROV_TRANSPORT_SOFTAP
    esp_netif_create_default_wifi_ap();
#endif /* CONFIG_EXAMPLE_PROV_TRANSPORT_SOFTAP */

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    wifi_prov_queue = xQueueCreate(1, sizeof(uint8_t));
    sendQueue(WIFI_CHECK_PROVISION, wifi_prov_queue, __func__);

    BaseType_t xReturned = xTaskCreate(provisioningTask, 
                                        "WIFI_PROVISIONING_MONITOR", 
                                        4096, NULL, 4, NULL );
    if( xReturned == pdPASS )
    {
        /* The task was created.  Use the task's handle to delete the task. */
        // vTaskDelete( xHandle );
        ESP_LOGI(TAG, "provisioningTask was created successfully.");
    }

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT | PROV_END_EVENT, pdFALSE, pdTRUE, portMAX_DELAY);
}

void DeviceProvisioning::provMgrStart(void)
{
    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = 
    {
    #ifdef CONFIG_EXAMPLE_PROV_TRANSPORT_SOFTAP
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    #endif /* CONFIG_EXAMPLE_PROV_TRANSPORT_SOFTAP */
    };

    /* Initialize provisioning manager with the
    * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    ESP_LOGI(TAG, "%s: WiFi provision manager initialized.", __func__);
}

void DeviceProvisioning::provisioningTask(void *pvParameters)
{
    ESP_LOGI(TAG, "provisioningTask Started.");
    uint8_t buf;
    bool run_task = true;

    while(run_task)
    {
        xQueueReceive(wifi_prov_queue, &buf, portMAX_DELAY);
        ESP_LOGI(TAG, "%s: Running switch cases.", __func__);
        switch (buf)
        {
            // case STOP_TASK:
            //     run_task = false;
            //     break;
            // case START_SERVER:
            //     break;
            case WIFI_CHECK_PROVISION:
            {
                ESP_LOGW(TAG, "%s: case WIFI_CHECK_PROVISION.", __func__);
                esp_err_t prv_err = wifi_prov_mgr_is_provisioned(&is_provisioned);
                if(ESP_ERR_INVALID_STATE == prv_err)
                {
                    provMgrStart();
                    sendQueue(WIFI_CHECK_PROVISION, wifi_prov_queue, __func__);
                }
                else if (!is_provisioned)
                {
                    ESP_LOGW(TAG, "%s: WiFi is not provisioned.", __func__);
                    sendQueue(WIFI_PROVISION, wifi_prov_queue, __func__);
                }
                else if(is_provisioned)
                {
                    ESP_LOGI(TAG, "%s: WiFi is provisioned. Starting WiFi.", __func__);
                    wifi_state = WIFI_DISCONNECTED;
                    wifi_prov_mgr_deinit();
                    wifiInitSTA();
                }
                break;
            }
            case WIFI_PROVISION:
            {
                ESP_LOGW(TAG, "%s: case WIFI_PROVISION.", __func__);
                HttpServer::stopServer();

                if(!is_provisioned || (WIFI_CONNECTED != wifi_state))
                {
                    ESP_LOGI(TAG, "Starting provisioning manager");
                    saveWiFiCredentials();
                    wifi_state = WIFI_PROVISIONING;
                    
                    wifi_prov_mgr_endpoint_create("domain-name");

                    char service_name[20];
                    getDeviceServiceName(service_name, sizeof(service_name));

                    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
                    const char *pop = "abcd1234";
                    const char *service_key = NULL;

                    /* Start provisioning service */
                    esp_err_t err_prov = wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key);
                    // ESP_LOGE(TAG, "error no. %d occured when starting provision manager", err_prov);
                    if(ESP_ERR_INVALID_STATE == err_prov)
                    {
                        wifi_state = WIFI_DISCONNECTED;
                        sendQueue(WIFI_PROVISION, wifi_prov_queue, __func__);
                        provMgrStart();
                        break;
                    }
                    // ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));
                    wifi_prov_mgr_endpoint_register("domain-name", DeviceProvisioning::domainNameDataHandler, NULL);
                    // xEventGroupWaitBits(wifi_event_group, PROV_END_EVENT, pdFALSE, pdTRUE, portMAX_DELAY);
                    // xEventGroupSetBits(wifi_event_group, STARTUP_END_EVENT);
                    // is_provisioned = true;
                }
                break;
            }
            case WIFI_CONNECT:
            {
                ESP_LOGW(TAG, "%s: case WIFI_CONNECT.", __func__);
                wifiConnSTA();
                break;
            }
            default:
                break;
        }
    }
    vQueueDelete(wifi_prov_queue);
    vTaskDelete(NULL);
}

esp_err_t DeviceProvisioning::reset(void)
{
    if(WIFI_PROVISIONING == wifi_state)
    {
        // if its already in prov state than how can it communicate with the app???
        ESP_LOGW(TAG, "Device already in provisioning state.");
        return ESP_FAIL;
    }
    else
    {
        wifi_state = WIFI_DISCONNECTED;
        is_provisioned = false;
        sendQueue(WIFI_PROVISION, wifi_prov_queue, __func__);
        return ESP_OK;
    }
    // wifi_prov_mgr_reset_provisioning();
}

void DeviceProvisioning::setWiFiCredentials(void)
{
    esp_wifi_set_config(WIFI_IF_STA, &wifi_backup_cfg);
}

void DeviceProvisioning::saveWiFiCredentials(void)
{
    if (esp_wifi_get_config(WIFI_IF_STA, &wifi_backup_cfg) != ESP_OK)
    {
       ESP_LOGE(TAG, "Error encountered while saving WiFi credentials.");
    }
    wifi_backup_cfg.sta.failure_retry_cnt = 0;
}