#include "HttpRequestsHandlers.hpp"
#include "LocalDB.hpp"
#include "UserManager.hpp"
#include "JCErrors.h"
#include "Device.hpp"
#include "SessionManager.hpp"
#include <esp_log.h>
#include "Parser.hpp"

static const char *TAG = "HttpRequestsHandlers";

/* An HTTP GET handler */
esp_err_t helloworld(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET request for /helloWorld");
    httpd_resp_send(req, helloWorldResponse, strlen(helloWorldResponse));
    return ESP_OK;
}

esp_err_t getData(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;

    char query[256];
    char engineId[10];

    esp_err_t ret = ESP_OK; // Initialize to ESP_OK

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));  
    if (ret == ESP_ERR_NOT_FOUND)
    {
        LocalDB& db = LocalDB::getInstance();
        bool latest = true;
        if (std::string("Configurations") == tableName)
        {
            latest = false;
        }
        auto searchResults = db.search(tableName, {"*"}, {}, latest);
        if (searchResults.empty()) 
        {
            // No matching data found
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "data not found");
            return ESP_FAIL;
        }
        std::map<std::string, std::vector<std::map<std::string, std::string>>> jsonData;
        jsonData[tableName] = searchResults;
        char* resp = Parser::convertMapToJSON(jsonData);
        httpd_resp_send(req, resp, strlen(resp));
        free(resp);

        return ESP_OK;
    }
    else if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error parsing query parameters");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "error parsing query parameters");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Parse the query parameters using getQueryParameters function
    auto queryParams = Parser::getQueryParameters(query);

    if (!Parser::hasKey(queryParams, "engine_id")) 
    {
        ESP_LOGE(TAG, "missing or invalid engine_id parameter");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing or invalid engine_id parameter");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "engine_id: %s", engineId);

    LocalDB& db = LocalDB::getInstance();

    std::map<std::string, std::string> filter;
    filter["engine_id"] = queryParams["engine_id"].c_str();

    bool latest = true;
    if (std::string("Configurations") == tableName)
    {
        latest = false;
    }
    
    auto searchResults = db.search(tableName, {"*"}, filter, latest);

    if (searchResults.empty())
    {
        // No matching data found
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "data not found");
        return ESP_FAIL;
    }

    std::map<std::string, std::vector<std::map<std::string, std::string>>> jsonData;
    jsonData[tableName] = searchResults;
    char* resp = Parser::convertMapToJSON(jsonData);
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

esp_err_t setConfigurations(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;

    char query[256];
    std::map<std::string, std::string> updateValues;
    LocalDB& db = LocalDB::getInstance();

    esp_err_t ret = ESP_OK; // Initialize to ESP_OK

    char header[128];
    ret = httpd_req_get_hdr_value_str(req, "Authorization", header, sizeof(header));
    if (ESP_OK != ret) 
    {
        ESP_LOGE(TAG, "%s", missingAuthorizationHeaderResponse);
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, missingAuthorizationHeaderResponse);
        return ESP_FAIL;
    }

    // Check if the header starts with "Bearer "
    const char* bearerPrefix = "Bearer ";
    if (strncmp(header, bearerPrefix, strlen(bearerPrefix)) != 0) 
    {
        ESP_LOGE(TAG, "%s", invalidHeaderFormatResponse);
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, invalidHeaderFormatResponse);
        return ESP_FAIL;
    }

    // Extract the token part
    char* token = header + strlen(bearerPrefix);

    // Verify the token
    TokenStatus tokenStatus = SessionManager::verifyToken(token);
    if (TOKEN_VALID != tokenStatus) 
    {
        if(TOKEN_INVALID == tokenStatus)
        {
            ESP_LOGE(TAG, "%s", invalidTokenResponse);
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, invalidTokenResponse);
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGE(TAG, "%s", expiredTokenResponse);
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, expiredTokenResponse);
            return ESP_FAIL;
        }
    }

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "%s", queryParseErrorResponse);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, queryParseErrorResponse);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Get query parameters
    std::map<std::string, std::string> queryParams = Parser::getQueryParameters(query);

    if (!Parser::hasKey(queryParams, "status") && !Parser::hasKey(queryParams, "time_interval")) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, missingConfigurationsResponse);
        return ESP_FAIL;
    }

    if (Parser::hasKey(queryParams, "status")) 
    {
        updateValues["status"] = queryParams["status"];
    }

    if(Parser::hasKey(queryParams, "time_interval")) 
    {
        updateValues["time_interval"] = queryParams["time_interval"];
    }

    // Parse the query parameters
    if (!Parser::hasKey(queryParams, "engine_id")) 
    {
        db.update(tableName, updateValues);
        httpd_resp_send(req, configurationsUpdatedResponse, strlen(configurationsUpdatedResponse));
        return ESP_OK;
    }

    ESP_LOGI(TAG, "engine_id: %s", queryParams["engine_id"].c_str());

    std::map<std::string, std::string> filter;
    filter["engine_id"] = queryParams["engine_id"].c_str();
    // Check if engine_id exists in the database
    auto searchResults = db.search(tableName, {"engine_id"}, filter);
    if (!searchResults.empty()) 
    {
        std::map<std::string, std::string> filter = {{"engine_id", queryParams["engine_id"].c_str()}};
        db.update(tableName, updateValues, filter);
    } 
    else 
    {
        ESP_LOGE(TAG, "engine_id not found in the database");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "engine_id not found in the database");
        return ESP_FAIL;
    }

    // Send success response
    httpd_resp_send(req, configurationsUpdatedResponse, strlen(configurationsUpdatedResponse));
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
        ESP_LOGE(TAG, "%s", emptyRequestBodyResponse);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, emptyRequestBodyResponse);
        return ESP_FAIL;
    }

    // Allocate memory for request body
    buffer = (char*)malloc(content_length + 1);
    if (!buffer) 
    {
        ESP_LOGE(TAG, "%s", memoryAllocationFailedResponse);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, memoryAllocationFailedResponse);
        return ESP_FAIL;
    }

    // Read request body
    ret = httpd_req_recv(req, buffer, content_length);
    if (ret <= 0) 
    {
        ESP_LOGE(TAG, "%s", receivingRequestBodyResponse);
        free(buffer);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, receivingRequestBodyResponse);
        return ESP_FAIL;
    }

    // Null-terminate the received data
    buffer[ret] = '\0';

    // Parse the request body to get query parameters
    auto credentials = Parser::parseCredentials(buffer);

    // Check if username and password are present
    if (credentials.empty() || !Parser::hasKey(credentials, "username") || !Parser::hasKey(credentials, "password")) 
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, missingCredentialsResponse);
        free(buffer);
        return ESP_FAIL;
    }

    // Authenticate user
    UserCredentialsManager& userMan = UserCredentialsManager::getInstance();
    ret = userMan.authenticateUser(credentials["username"], credentials["password"]);
    if (ret != ESP_OK) 
    {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, authenticationFailedResponse);
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