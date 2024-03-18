#include "Device.hpp"
#include <esp_wifi.h>
#include <esp_log.h>
#include "CustomJWT.h"
#include "cJSON.h"

static const char* TAG = "Device";

std::string Device::username;
std::string Device::password;

std::pair<std::string, std::string> Device::generateCredentials() {
    // Get MAC address of ESP32
    uint8_t mac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if(ret != ESP_OK)
    {
        return std::make_pair("", "");
    }

    // Convert MAC address to string
    std::stringstream stream;
    stream << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        stream << std::setw(2) << static_cast<int>(mac[i]);
    }
    std::string macAddress = stream.str();

    ESP_LOGE(TAG, "%s", macAddress.c_str());
    // Generate username and password from MAC address
    username = "ESP_" + macAddress.substr(0, 6);
    password = macAddress.substr(6);

    return std::make_pair(username, password);
}

std::pair<std::string, std::string> Device::getCredentials()
{
    if(!username.empty() && !password.empty())
    {
        return std::make_pair(username, password);
    }

    return generateCredentials();
}

void printData(char *data, size_t dataLen) {
    ESP_LOGI(TAG, "Data: %s Data Length: %d", data, dataLen);
}

std::string Device::getBearerToken() {
    // Retrieve credentials
    std::pair<std::string, std::string> credentials = getCredentials();
    
    // Construct payload
    std::string payload = "{\"username\":\"" + credentials.first + "\"}";

    // Set JWT key
    std::string key = credentials.first + "_ash_" + credentials.second;

    // Create CustomJWT object
    CustomJWT jwt(const_cast<char*>(key.c_str()), 256);

    // Allocate memory for JWT
    jwt.allocateJWTMemory();

    // Encode JWT
    ESP_LOGI(TAG, "Generating a JWT");
    bool ret = jwt.encodeJWT(const_cast<char*>(payload.c_str()));

    if (ret)
    {
        // Log JWT parts
        ESP_LOGI(TAG, "Header Info");
        printData(jwt.header, jwt.headerLength);
        ESP_LOGI(TAG, "Payload Info");
        printData(jwt.payload, jwt.payloadLength);
        ESP_LOGI(TAG, "Signature Info");
        printData(jwt.signature, jwt.signatureLength);
        ESP_LOGI(TAG, "Final Output Info");
        printData(jwt.out, jwt.outputLength);
    }

    std::string token(jwt.out);
    // Clear JWT
    jwt.clear();
    return token;
}

// Function to parse JSON payload using cJSON
std::string parseUsername(const char* payload) {
    cJSON* json = cJSON_Parse(payload);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON payload");
        return "";
    }

    cJSON* usernameJson = cJSON_GetObjectItemCaseSensitive(json, "username");
    if (!usernameJson || !cJSON_IsString(usernameJson)) {
        ESP_LOGE(TAG, "Username not found in JSON payload");
        cJSON_Delete(json);
        return "";
    }

    std::string username = usernameJson->valuestring;

    cJSON_Delete(json);
    return username;
}

esp_err_t Device::verifyToken(char* token) {
    // Retrieve credentials
    std::pair<std::string, std::string> credentials = getCredentials();
    
    // Construct payload
    std::string payload = "{\"username\":\"" + credentials.first + "\"}";

    // Set JWT key
    std::string key = credentials.first + "_ash_" + credentials.second;
    
    // Create CustomJWT object
    CustomJWT jwt(const_cast<char*>(key.c_str()), 256);

    jwt.allocateJWTMemory();

    int ret = jwt.decodeJWT(token);
    if (!ret)
    {
        ESP_LOGI(TAG, "Header Info");
        printData(jwt.header, jwt.headerLength);
        ESP_LOGI(TAG, "Payload Info");
        printData(jwt.payload, jwt.payloadLength);
        ESP_LOGI(TAG, "Signature Info");
        printData(jwt.signature, jwt.signatureLength);
        if(parseUsername(jwt.payload) == credentials.first)
        {
            jwt.clear();
            return ESP_OK;
        }
    }
    
    jwt.clear();
    return ESP_FAIL;
}