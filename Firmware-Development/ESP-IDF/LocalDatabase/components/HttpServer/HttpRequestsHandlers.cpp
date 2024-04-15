#include "HttpRequestsHandlers.hpp"
#include "LocalDB.hpp"
#include "UserManager.hpp"
#include "JCErrors.h"
#include "Device.hpp"
#include "SessionManager.hpp"
#include <esp_log.h>
#include "Parser.hpp"

static const char *TAG = "HttpRequestsHandlers";

static esp_err_t validateToken(httpd_req_t *req) {
    char header[128];
    esp_err_t ret = httpd_req_get_hdr_value_str(req, "Authorization", header, sizeof(header));
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s", missingAuthorizationHeader.errorMessage);
        httpd_resp_send_err(req, missingAuthorizationHeader.errorCode, missingAuthorizationHeader.errorMessage);
        return ESP_FAIL;
    }

    // Check if the header starts with "Bearer "
    const char* bearerPrefix = "Bearer ";
    if (strncmp(header, bearerPrefix, strlen(bearerPrefix)) != 0) {
        ESP_LOGE(TAG, "%s", invalidHeaderFormat.errorMessage);
        httpd_resp_send_err(req, invalidHeaderFormat.errorCode, invalidHeaderFormat.errorMessage);
        return ESP_FAIL;
    }

    // Extract the token part
    char* token = header + strlen(bearerPrefix);

    // Verify the token
    TokenStatus tokenStatus = SessionManager::verifyToken(token);
    if (TOKEN_VALID != tokenStatus) {
        ErrorResponse response = (TOKEN_INVALID == tokenStatus) ? invalidToken : expiredToken;
        ESP_LOGE(TAG, "%s", response.errorMessage);
        httpd_resp_send_err(req, response.errorCode, response.errorMessage);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* An HTTP GET handler */
esp_err_t helloworld(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET request for /helloWorld");
    httpd_resp_send(req, helloWorld.errorMessage, strlen(helloWorld.errorMessage));
    return ESP_OK;
}

esp_err_t getData(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;

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
        httpd_resp_send_err(req, noEnabledEngines.errorCode, noEnabledEngines.errorMessage);
        return ESP_FAIL;
    }

    // Combine data for the specified table
    jsonData[tableName] = fetchAndCombineData(tableName);

    // Convert jsonData to JSON
    char* resp = Parser::convertMapToJSON(jsonData);
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

esp_err_t setConfigurations(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;

    // Validate token
    esp_err_t tokenValidationResult = validateToken(req);
    if (tokenValidationResult != ESP_OK) 
    {
        return tokenValidationResult;
    }

    char query[256];
    std::map<std::string, std::string> updateValues;
    LocalDB& db = LocalDB::getInstance();

    esp_err_t ret = ESP_OK; // Initialize to ESP_OK

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "%s", queryParseError.errorMessage);
        httpd_resp_send_err(req, queryParseError.errorCode, queryParseError.errorMessage);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Get query parameters
    std::map<std::string, std::string> queryParams = Parser::getQueryParameters(query);

    // List of required parameters
    std::vector<std::string> requiredParams = {"enabled", "engine_id", "fuel_type", "engine_type", "position", "volume_per_cycle", "fuel_volume"};

    // Check if at least one required parameter is present
    bool atLeastOneParamPresent = false;
    for (const auto& param : requiredParams) 
    {
        if (Parser::hasKey(queryParams, param)) 
        {
            atLeastOneParamPresent = true;
            break;
        }
    }

    // If none of the required parameters are present, send an error response
    if (!atLeastOneParamPresent) 
    {
        httpd_resp_send_err(req, missingConfigurationParams.errorCode, missingConfigurationParams.errorMessage);
        return ESP_FAIL;
    }

    // Loop through each required parameter and copy if exists
    for (const auto& param : requiredParams) 
    {
        if (Parser::hasKey(queryParams, param)) 
        {
            updateValues[param] = queryParams[param];
        }
    }
    updateValues.erase("engine_id");

    // If engine_id is not provided, update all entries in the table
    if (!Parser::hasKey(queryParams, "engine_id")) {
        db.update(tableName, updateValues);
        httpd_resp_send(req, configurationsUpdated.errorMessage, strlen(configurationsUpdated.errorMessage));
        return ESP_OK;
    }

    // Update entries with specified engine_id
    std::map<std::string, std::string> filter;
    filter["id"] = queryParams["engine_id"];

    // Check if engine_id exists in the database
    auto searchResults = db.search(tableName, {"*"}, filter);
    if (!searchResults.empty()) 
    {
        db.update(tableName, updateValues, filter);
    } 
    else 
    {
        ESP_LOGE(TAG, "%s", engineIdNotFound.errorMessage);
        httpd_resp_send_err(req, engineIdNotFound.errorCode, engineIdNotFound.errorMessage);
        return ESP_FAIL;
    }

    // Send success response
    httpd_resp_send(req, configurationsUpdated.errorMessage, strlen(configurationsUpdated.errorMessage));
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

    // Check content length
    int content_length = req->content_len;
    if (content_length <= 0) 
    {
        ESP_LOGE(TAG, "%s", emptyRequestBody.errorMessage);
        httpd_resp_send_err(req, emptyRequestBody.errorCode, emptyRequestBody.errorMessage);
        return ESP_FAIL;
    }

    // Allocate memory for request body
    buffer = (char*)malloc(content_length + 1);
    if (!buffer) 
    {
        ESP_LOGE(TAG, "%s", memoryAllocationFailed.errorMessage);
        httpd_resp_send_err(req, memoryAllocationFailed.errorCode, memoryAllocationFailed.errorMessage);
        return ESP_FAIL;
    }

    // Read request body
    ret = httpd_req_recv(req, buffer, content_length);
    if (ret <= 0) 
    {
        ESP_LOGE(TAG, "%s", receivingRequestBody.errorMessage);
        free(buffer);
        httpd_resp_send_err(req, receivingRequestBody.errorCode, receivingRequestBody.errorMessage);
        return ESP_FAIL;
    }

    // Null-terminate the received data
    buffer[ret] = '\0';

    // Parse the request body to get query parameters
    auto credentials = Parser::parseCredentials(buffer);

    // Check if username and password are present
    if (credentials.empty() || !Parser::hasKey(credentials, "username") || !Parser::hasKey(credentials, "password")) 
    {
        httpd_resp_send_err(req, missingCredentials.errorCode, missingCredentials.errorMessage);
        free(buffer);
        return ESP_FAIL;
    }

    // Authenticate user
    UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
    ret = userMan.authenticateUser(credentials["username"], credentials["password"]);
    if (ret != ESP_OK) 
    {
        httpd_resp_send_err(req, authenticationFailed.errorCode, authenticationFailed.errorMessage);
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