#include "HttpRequestsHandlers.hpp"
#include "DeviceProvisioning.hpp"
#include "LocalDB.hpp"
#include "UserManager.hpp"
#include "JCErrors.h"
#include "Device.hpp"
#include "SessionManager.hpp"
#include <esp_log.h>
#include "Parser.hpp"
#include "cJSON.h" 

static const char *TAG = "HttpRequestsHandlers";

static esp_err_t validateToken(httpd_req_t *req) {
    char header[128];
    esp_err_t ret = httpd_req_get_hdr_value_str(req, "Authorization", header, sizeof(header));
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s", missingAuthorizationHeader.responseJson);
        httpd_resp_send_err(req, missingAuthorizationHeader.status, missingAuthorizationHeader.responseJson);
        return ESP_FAIL;
    }

    // Check if the header starts with "Bearer "
    const char* bearerPrefix = "Bearer ";
    if (strncmp(header, bearerPrefix, strlen(bearerPrefix)) != 0) {
        ESP_LOGE(TAG, "%s", invalidHeaderFormat.responseJson);
        httpd_resp_send_err(req, invalidHeaderFormat.status, invalidHeaderFormat.responseJson);
        return ESP_FAIL;
    }

    // Extract the token part
    char* token = header + strlen(bearerPrefix);

    // Verify the token
    TokenStatus tokenStatus = SessionManager::verifyToken(token);
    if (TOKEN_VALID != tokenStatus) {
        ErrorResponse response = (TOKEN_INVALID == tokenStatus) ? invalidToken : expiredToken;
        ESP_LOGE(TAG, "%s", response.responseJson);
        httpd_resp_send_err(req, response.status, response.responseJson);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* An HTTP GET handler */
esp_err_t helloworld(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET request for /helloWorld");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, helloWorld.responseJson, strlen(helloWorld.responseJson));
    return ESP_OK;
}

esp_err_t getData(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;
    httpd_resp_set_type(req, "application/json");

    // Retrieve enabled engine ids
    std::vector<std::string> enabledEngineIds;
    LocalDB& db = LocalDB::getInstance();
    std::vector<std::map<std::string, std::string>> enabledEngines = db.search("Engines", {"id"}, {{"enabled", "1"}});
    for (auto& engine : enabledEngines) {
        enabledEngineIds.push_back(engine["id"]);
    }

    std::map<std::string, std::vector<std::map<std::string, std::string>>> jsonData;

    // Function to fetch and combine data for a given table
    auto fetchAndCombineData = [&](const char* tableName) 
    {
        std::vector<std::map<std::string, std::string>> combinedData;
        for (auto& engineId : enabledEngineIds) 
        {
            // Retrieve data from the table for this engine_id
            std::vector<std::map<std::string, std::string>> tableData = db.search(tableName, {"*"}, {{"engine_id", engineId}}, true);

            // Combine tableData into combinedData
            combinedData.insert(combinedData.end(), tableData.begin(), tableData.end());
        }
        return combinedData;
    };

    if (enabledEngineIds.empty()) {
        // No enabled engines found, send error response
        httpd_resp_send_err(req, noEnabledEngines.status, noEnabledEngines.responseJson);
        return ESP_FAIL;
    }

    // Combine data for the specified table
    jsonData[tableName] = fetchAndCombineData(tableName);

    const std::map<std::string, std::string> responseParams = {
        {"status", dataRetrieved.statusString},
        {"message", dataRetrieved.message}
    };

    // Convert jsonData to JSON
    char* resp = Parser::convertMapToJSON(jsonData, responseParams);
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

esp_err_t getEngines(httpd_req_t *req) {

    httpd_resp_set_type(req, "application/json");
    // Map to hold the search results
    std::map<std::string, std::vector<std::map<std::string, std::string>>> jsonData;
    
    // Retrieve data from Engines table to get all ids
    LocalDB& db = LocalDB::getInstance();
    std::vector<std::map<std::string, std::string>> enginesData = db.search("Engines", {"*"}, {});

    jsonData["Engines"] = enginesData;

    const std::map<std::string, std::string> responseParams = {
        {"status", dataRetrieved.statusString},
        {"message", dataRetrieved.message}
    };

    // Convert jsonData to JSON
    char* resp = Parser::convertMapToJSON(jsonData, responseParams);
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

esp_err_t setConfigurations(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;
    httpd_resp_set_type(req, "application/json");

    // Validate token
    esp_err_t tokenValidationResult = validateToken(req);
    if (tokenValidationResult != ESP_OK) 
    {
        return tokenValidationResult;
    }

    // Allocate memory for the request body
    int total_len = req->content_len;
    char* buf = (char*) malloc(total_len + 1);
    if (buf == NULL) 
    {
        httpd_resp_send_err(req, memoryAllocationFailed.status, memoryAllocationFailed.responseJson);
        return ESP_FAIL;
    }

    // Read the request body
    int ret = httpd_req_recv(req, buf, total_len);
    if (ret <= 0) 
    {
        free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) 
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[total_len] = '\0'; // Null-terminate the request body

    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) 
    {
        httpd_resp_send_err(req, invalidJson.status, invalidJson.responseJson);
        return ESP_FAIL;
    }

    // Extract data array from JSON
    cJSON *data_array = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsArray(data_array)) 
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, invalidJsonStructure.status, invalidJsonStructure.responseJson);
        return ESP_FAIL;
    }

    // Process each configuration in the data array
    cJSON *config = NULL;
    LocalDB& db = LocalDB::getInstance();

    cJSON_ArrayForEach(config, data_array) 
    {
        if (!cJSON_IsObject(config)) 
        {
            continue;
        }

        std::map<std::string, std::string> updateValues;
        std::string engine_id;

        // Extract required parameters
        cJSON *engine_id_item = cJSON_GetObjectItem(config, "engine_id");
        if (cJSON_IsString(engine_id_item)) 
        {
            engine_id = engine_id_item->valuestring;
        }

        std::vector<std::string> requiredParams = {"enabled", "fuel_type", "engine_type", "position", "volume_per_cycle", "fuel_volume"};

        bool atLeastOneParamPresent = false;
        for (const auto& param : requiredParams) 
        {
            cJSON *item = cJSON_GetObjectItem(config, param.c_str());
            if (cJSON_IsString(item) || cJSON_IsNumber(item)) 
            {
                updateValues[param] = item->valuestring;
                atLeastOneParamPresent = true;
            }
        }

        if (!atLeastOneParamPresent) 
        {
            cJSON_Delete(root);
            httpd_resp_send_err(req, missingConfigurationParams.status, missingConfigurationParams.responseJson);
            return ESP_FAIL;
        }

        // Update the database
        if (!engine_id.empty()) 
        {
            std::map<std::string, std::string> filter;
            filter["id"] = engine_id;

            // Check if engine_id exists in the database
            auto searchResults = db.search(tableName, {"*"}, filter);
            if (!searchResults.empty()) 
            {
                db.update(tableName, updateValues, filter);
            } 
            else 
            {
                cJSON_Delete(root);
                httpd_resp_send_err(req, engineIdNotFound.status, engineIdNotFound.responseJson);
                return ESP_FAIL;
            }
        }
        else
        {
            cJSON_Delete(root);
            httpd_resp_send_err(req, engineIdNotProvided.status, engineIdNotProvided.responseJson);
        }
    }

    cJSON_Delete(root);

    // Send success response
    httpd_resp_send(req, configurationsUpdated.responseJson, strlen(configurationsUpdated.responseJson));
    return ESP_OK;
}

esp_err_t faviconGetHandler(httpd_req_t *req)
{
    extern const uint8_t favicon_data_start[] asm("_binary_favicon_ico_start");
    extern const uint8_t favicon_data_end[] asm("_binary_favicon_ico_end");
    
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_data_start, favicon_data_end - favicon_data_start);
    return ESP_OK;
}

esp_err_t userLogin(httpd_req_t *req)
{
    char* buffer = nullptr;

    esp_err_t ret = ESP_OK;

    httpd_resp_set_type(req, "application/json");
    // Check content length
    int content_length = req->content_len;
    if (content_length <= 0) 
    {
        ESP_LOGE(TAG, "%s", emptyRequestBody.responseJson);
        httpd_resp_send_err(req, emptyRequestBody.status, emptyRequestBody.responseJson);
        return ESP_FAIL;
    }

    // Allocate memory for request body
    buffer = (char*)malloc(content_length + 1);
    if (!buffer) 
    {
        ESP_LOGE(TAG, "%s", memoryAllocationFailed.responseJson);
        httpd_resp_send_err(req, memoryAllocationFailed.status, memoryAllocationFailed.responseJson);
        return ESP_FAIL;
    }

    // Read request body
    ret = httpd_req_recv(req, buffer, content_length);
    if (ret <= 0) 
    {
        ESP_LOGE(TAG, "%s", receivingRequestBody.responseJson);
        free(buffer);
        httpd_resp_send_err(req, receivingRequestBody.status, receivingRequestBody.responseJson);
        return ESP_FAIL;
    }

    // Null-terminate the received data
    buffer[ret] = '\0';

    // Parse the request body to get query parameters
    auto credentials = Parser::parseCredentials(buffer);

    // Check if username and password are present
    if (credentials.empty() || !Parser::hasKey(credentials, "username") || !Parser::hasKey(credentials, "password")) 
    {
        httpd_resp_send_err(req, missingCredentials.status, missingCredentials.responseJson);
        free(buffer);
        return ESP_FAIL;
    }

    // Authenticate user
    UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
    ret = userMan.authenticateUser(credentials["username"], credentials["password"]);
    if (ret != ESP_OK) 
    {
        httpd_resp_send_err(req, authenticationFailed.status, authenticationFailed.responseJson);
        free(buffer);
        return ESP_FAIL;
    }

    // Send response
    std::string token = SessionManager::getBearerToken();
    std::string response = JSON_MESSAGE_WITH_TOKEN(true, "Login successful", token);
    httpd_resp_send(req, response.c_str(), response.length());

    // Free allocated memory
    free(buffer);

    return ESP_OK;
}

esp_err_t createUser(httpd_req_t *req) 
{
    char query[256];
    const char* responseSuccess = "User created successfully";
    const char* responseFail = "User creation failed";
    const char* response = responseFail;

    esp_err_t ret = ESP_FAIL;

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));

    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Query: %s", query);

        auto queryParams = Parser::getQueryParameters(query);
        if (!queryParams.empty())
        {
            if (Parser::hasKey(queryParams, "username") && Parser::hasKey(queryParams, "password"))
            {
                UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
                if (ESP_OK == userMan.createUser(queryParams["username"], queryParams["password"]))
                {
                    response = responseSuccess;
                }
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error parsing query parameters: %s", esp_err_to_name(ret));
    }

    // Send response
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
}

esp_err_t removeUser(httpd_req_t *req) 
{
    char query[256];
    const char* responseSuccess = "User removed successfully";
    const char* responseFail = "User removal failed";
    const char* response = responseFail;

    esp_err_t ret = ESP_FAIL;

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Query: %s", query);

        auto queryParams = Parser::getQueryParameters(query);
        if (!queryParams.empty())
        {
            if (Parser::hasKey(queryParams, "username"))
            {
                UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
                if(ESP_OK == userMan.removeUser(queryParams["username"]))
                {
                    response = responseSuccess;
                }
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error parsing query parameters: %s", esp_err_to_name(ret));
    }

    // Send response
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
}

esp_err_t updatePassword(httpd_req_t *req) 
{
    char query[256];
    const char* responseSuccess = "Password updated successfully";
    const char* responseFail = "Password update failed";
    const char* response = responseFail;

    esp_err_t ret = ESP_FAIL;

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Query: %s", query);

        auto queryParams = Parser::getQueryParameters(query);
        if (!queryParams.empty())
        {
            if (Parser::hasKey(queryParams, "username") && Parser::hasKey(queryParams, "old_password") && Parser::hasKey(queryParams, "new_password"))
            {
                UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
                if(ESP_OK == userMan.updatePassword(queryParams["username"], queryParams["old_password"], queryParams["new_password"]))
                {
                    response = responseSuccess;
                }
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error parsing query parameters: %s", esp_err_to_name(ret));
    }

    // Send response
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
}

esp_err_t startProvision(httpd_req_t *req) 
{
    ESP_LOGI(TAG, "Provisioning request received.");
    httpd_resp_send(req, "Provisioning request received.", HTTPD_RESP_USE_STRLEN);
    if(ESP_OK != DeviceProvisioning::reset())
    {
        httpd_resp_send(req, "Device already in provisioning state.", HTTPD_RESP_USE_STRLEN);
    }
    else{
        httpd_resp_send(req, "Started provisioning", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}