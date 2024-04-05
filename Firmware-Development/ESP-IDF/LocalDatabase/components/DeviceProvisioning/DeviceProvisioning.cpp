#include "DeviceProvisioning.hpp"
#include "JCJson.hpp"

const int WIFI_CONNECTED_EVENT = BIT0;
const int PROV_END_EVENT = BIT1;
static const char *TAG = "app";
static EventGroupHandle_t wifi_event_group;
/* Event handler for catching system events */
void DeviceProvisioning::eventHandler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
    static int retries;
#endif
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
            retries++;
            if (retries >= CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT)
            {
                ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                wifi_prov_mgr_reset_sm_state_on_failure();
                retries = 0;
            }
#endif
            break;
        }

        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
            retries = 0;
#endif
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            xEventGroupSetBits(wifi_event_group, PROV_END_EVENT);
            break;
        default:
            break;
        }
    }
    
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
    }
}

void DeviceProvisioning::wifiInitSTA(void)
{
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
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
    const char *previously_assigned_domain = "";//getPreviouslyAssignedDomain(); // Your implementation to retrieve previously assigned domain

    // Create JSON response
    std::map<std::string, std::variant<std::string, int>> response_keyValues;
    response_keyValues["type"] = std::string("domain_response");
    response_keyValues["domain_name"] = previously_assigned_domain ? previously_assigned_domain : "";

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

    const char *assigned_domain = std::get<std::string>(keyValues["domain_name"]).c_str();

    // Perform domain assignment (your implementation)
    esp_err_t assignment_result = ESP_OK;//performDomainAssignment(assigned_domain);

    // Create JSON response
    std::map<std::string, std::variant<std::string, int>> response_keyValues;
    response_keyValues["type"] = std::string("domain_assignment_response");
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
    #if 0 
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

    bool provisioned = false;
#ifdef CONFIG_EXAMPLE_RESET_PROVISIONED
    wifi_prov_mgr_reset_provisioning();
#else
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
#endif

    /* If device is not yet provisioned start provisioning service */
    if (!provisioned)
    {
        wifi_prov_mgr_endpoint_create("domain-name");
        ESP_LOGI(TAG, "Starting provisioning");

        char service_name[20];
        getDeviceServiceName(service_name, sizeof(service_name));

        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        const char *pop = "abcd1234";
        const char *service_key = NULL;

        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));
        wifi_prov_mgr_endpoint_register("domain-name", DeviceProvisioning::domainNameDataHandler, NULL);
        xEventGroupWaitBits(wifi_event_group, PROV_END_EVENT, false, true, portMAX_DELAY);
    }
    else
    {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

        /* We don't need the manager as the device is already provisioned,
         * so let's release its resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi station */
        wifiInitSTA();
    }

    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);

}
