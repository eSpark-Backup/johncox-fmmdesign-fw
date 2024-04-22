#include "SessionManager.hpp"
#include "CustomJWT.h"
#include "cJSON.h"
#include <chrono>
#include <ctime>
#include <random>
#include <esp_log.h>
#include "Device.hpp"

static const char* TAG = "Session Manager";

std::string SessionManager::token;
std::string SessionManager::key;
long SessionManager::previousTokenGenerationTime = 0;

void printData(char *data, size_t dataLen) 
{
    ESP_LOGI(TAG, "Data: %s Data Length: %d", data, dataLen);
}

// Function to get the current time in seconds since epoch
long SessionManager::getCurrentTimeInSeconds() 
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

long SessionManager::getTimeElapsedSinceTokenGeneration() 
{
    long currentTime = getCurrentTimeInSeconds();
    return (currentTime - previousTokenGenerationTime);
}

// Function to generate a random key
std::string SessionManager::generateRandomKey(int length) 
{
    // Characters to choose from for the key
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    // Seed for the random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Seed with current time
    gen.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    
    std::uniform_int_distribution<> dis(0, characters.size() - 1);

    std::string key;
    key.reserve(length);
    
    // Generate the key
    for (int i = 0; i < length; ++i) 
    {
        key += characters[dis(gen)];
    }

    return key;
}

std::string SessionManager::getBearerToken() 
{
    if (!((previousTokenGenerationTime == 0) || (getTimeElapsedSinceTokenGeneration() > TOKEN_EXPIRE_DURATION)))
    {
        return token;
    }

    // Retrieve credentials
    std::pair<std::string, std::string> credentials = Device::getCredentials();
    
    // Construct payload
    std::string payload = "{\"username\":\"" + credentials.first + "\"}";

    // Set JWT key
    std::string lKey = generateRandomKey();

    // Create CustomJWT object
    CustomJWT jwt(const_cast<char*>(lKey.c_str()), 256);

    // Allocate memory for JWT
    jwt.allocateJWTMemory();
    
    ESP_LOGI(TAG, "Generating a JWT");
    bool ret = jwt.encodeJWT(const_cast<char*>(payload.c_str()));
    if (ret)
    {
        previousTokenGenerationTime = getCurrentTimeInSeconds();
        token = jwt.out;
        key = lKey;
        ESP_LOGE(TAG, "key : %s", key.c_str());

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
    
    // Clear JWT
    jwt.clear();
    return token;
}

// Function to parse JSON payload using cJSON
std::string SessionManager::parseUsername(const char* payload) 
{
    cJSON* json = cJSON_Parse(payload);
    if (!json) 
    {
        ESP_LOGE(TAG, "Failed to parse JSON payload");
        return "";
    }

    cJSON* usernameJson = cJSON_GetObjectItemCaseSensitive(json, "username");
    if (!usernameJson || !cJSON_IsString(usernameJson)) 
    {
        ESP_LOGE(TAG, "Username not found in JSON payload");
        cJSON_Delete(json);
        return "";
    }

    std::string username = usernameJson->valuestring;

    cJSON_Delete(json);
    return username;
}

TokenStatus SessionManager::verifyToken(char* token) 
{
    if ((previousTokenGenerationTime == 0) || getTimeElapsedSinceTokenGeneration() > TOKEN_EXPIRE_DURATION)
    {
        return TOKEN_EXPIRED;
    }
    // Retrieve credentials
    std::pair<std::string, std::string> credentials = Device::getCredentials();
    
    // Construct payload
    std::string payload = "{\"username\":\"" + credentials.first + "\"}";
    
    ESP_LOGE(TAG, "key : %s", key.c_str());
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
        if(SessionManager::parseUsername(jwt.payload) == credentials.first)
        {
            jwt.clear();
            return TOKEN_VALID;
        }
    }
    
    jwt.clear();
    return TOKEN_INVALID;
}