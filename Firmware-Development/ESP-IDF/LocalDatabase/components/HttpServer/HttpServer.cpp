#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "HttpServer.hpp"
#include "LocalDB.hpp"
#include "UserManager.hpp"

static const char *TAG = "HTTPS_SERVER";

extern const uint8_t favicon_data_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_data_end[] asm("_binary_favicon_ico_end");

static esp_err_t createUser(httpd_req_t *req);
static esp_err_t removeUser(httpd_req_t *req);
static esp_err_t updatePassword(httpd_req_t *req);
static esp_err_t userLogin(httpd_req_t *req);
static esp_err_t setConfigurations(httpd_req_t *req);
static esp_err_t parseConfigParams(const char* query, std::map<std::string, std::string>& updateValues);
static esp_err_t getData(httpd_req_t *req);
static esp_err_t helloworld(httpd_req_t *req);
static esp_err_t faviconGetHandler(httpd_req_t *req);
static std::map<std::string, std::string> getQueryParameters(const char* queryString);
char* convertSearchResultsToJSON(const std::vector<std::map<std::string, std::string>>& searchResults);

/* URI handlers array */
httpd_uri_t uri_handlers[] = 
{
    { .uri = "/helloWorld",         .method = HTTP_GET,     .handler = helloworld,          .user_ctx  = NULL},
    { .uri = "/getSensorData",      .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"SensorData"},
    { .uri = "/getFuelManagement",  .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"FuelManagement"},
    { .uri = "/getConfigurations",  .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"Configurations"},
    { .uri = "/getEngines",         .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"Engines"},
    { .uri = "/setConfigurations",  .method = HTTP_GET,     .handler = setConfigurations,   .user_ctx  = (void*)"Configurations"},
    // { .uri = "/createUser",         .method = HTTP_GET,     .handler = createUser,          .user_ctx  = NULL},
    // { .uri = "/removeUser",         .method = HTTP_GET,     .handler = removeUser,          .user_ctx  = NULL},
    // { .uri = "/updatePassword",     .method = HTTP_GET,     .handler = updatePassword,      .user_ctx  = NULL},
    { .uri = "/userLogin",          .method = HTTP_GET,     .handler = userLogin,      .user_ctx  = NULL},
    { .uri = "/favicon.ico",        .method = HTTP_GET,     .handler = faviconGetHandler,   .user_ctx  = NULL}
};

/* Start the HTTP server */
void HttpServer::start(void)
{
    // httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    config.httpd.max_uri_handlers = 15;
    config.httpd.max_open_sockets = 6;
    config.session_tickets = false;

    /*certificates and key*/
    extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    config.cacert_pem = cacert_pem_start;
    config.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    config.prvtkey_pem = prvtkey_pem_start;
    config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &config);

    if (ret == ESP_OK) 
    {
        for (int i = 0; i < sizeof(uri_handlers) / sizeof(uri_handlers[0]); i++) 
        {
            if (httpd_register_uri_handler(server, &uri_handlers[i]) != ESP_OK) 
            {
                ESP_LOGE(TAG, "Failed to register URI handler");
            }
        }
        ESP_LOGI(TAG, "HTTPS Server started");
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to start HTTP Server : %s", esp_err_to_name(ret));
    }
}

// Function to convert search results to JSON
char* convertSearchResultsToJSON(const std::vector<std::map<std::string, std::string>>& searchResults) {
    cJSON* root = cJSON_CreateArray(); // Create a JSON array
    if (!root) {
        return nullptr;
    }

    for (const auto& result : searchResults) {
        cJSON* item = cJSON_CreateObject(); // Create a JSON object for each row
        if (!item) {
            cJSON_Delete(root);
            return nullptr;
        }

        for (const auto& pair : result) {
            cJSON_AddStringToObject(item, pair.first.c_str(), pair.second.c_str());
        }

        cJSON_AddItemToArray(root, item); // Add object to the array
    }

    char* jsonStr = cJSON_Print(root); // Convert JSON array to string
    cJSON_Delete(root); // Clean up cJSON objects

    return jsonStr;
}

static std::map<std::string, std::string> getQueryParameters(const char* queryString) {
    std::map<std::string, std::string> parameters;

    char* token;
    char* queryCopy = strdup(queryString);  // Create a copy of queryString to avoid modifying the original

    token = strtok(queryCopy, "&");
    while (token != nullptr) {
        char* separator = strchr(token, '=');
        if (separator != nullptr) {
            *separator = '\0';  // Split the token into parameter and value
            std::string key(token);
            std::string value(separator + 1);
            parameters[key] = value;
        }
        token = strtok(nullptr, "&");
    }

    free(queryCopy);  // Free the allocated memory for the copy
    return parameters;
}

// Check if a specific key exists in the map
bool hasKey(const std::map<std::string, std::string>& parameters, const std::string& key) {
    return parameters.find(key) != parameters.end();
}

/* An HTTP GET handler */
esp_err_t helloworld(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET request for /hello");
    const char *response = "<!DOCTYPE html> <html> <body> <h1>Electronics</h1> <p>Embedded Systems</p> </body> </html>";
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

static esp_err_t getData(httpd_req_t *req) 
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
        auto searchResults = db.search(tableName, {"*"});
        if (searchResults.empty()) {
            // No matching data found
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "data not found");
            return ESP_FAIL;
        }
        char* resp = convertSearchResultsToJSON(searchResults);
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
    auto queryParams = getQueryParameters(query);

    if (!hasKey(queryParams, "engine_id")) {
        ESP_LOGE(TAG, "missing or invalid engine_id parameter");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing or invalid engine_id parameter");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "engine_id: %s", engineId);

    LocalDB& db = LocalDB::getInstance();
    auto searchResults = db.search(tableName, {"*"}, std::make_pair("engine_id", queryParams["engine_id"].c_str()));

    if (searchResults.empty()) {
        // No matching data found
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "data not found");
        return ESP_FAIL;
    }

    char* resp = convertSearchResultsToJSON(searchResults);
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

static esp_err_t parseConfigParams(const char* query, std::map<std::string, std::string>& updateValues) 
{
    char status[10];
    char timeInterval[10];

    esp_err_t ret = httpd_query_key_value(query, "status", status, sizeof(status));
    if (ret == ESP_OK) 
    {
        updateValues["status"] = status;
    } 
    else 
    {
        ESP_LOGE(TAG, "Error parsing status: %s", esp_err_to_name(ret));
    }

    ret = httpd_query_key_value(query, "time_interval", timeInterval, sizeof(timeInterval));
    if (ret == ESP_OK) 
    {
        updateValues["time_interval"] = timeInterval;
    } 
    else 
    {
        ESP_LOGE(TAG, "Error parsing time_interval: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

static esp_err_t setConfigurations(httpd_req_t *req) {
    const char* tableName = (const char*) req->user_ctx;

    char query[256];
    char engineId[10];
    const char* responseSuccess = "Configurations updated successfully";
    const char* responseFail = "Configurations update failed";
    const char* response = responseSuccess;
    std::map<std::string, std::string> updateValues;
    LocalDB& db = LocalDB::getInstance();

    esp_err_t ret = ESP_OK; // Initialize to ESP_OK

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error parsing query parameters: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error parsing query parameters");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    // Parse the query parameters
    ret = httpd_query_key_value(query, "engine_id", engineId, sizeof(engineId));
    if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "Error parsing engine_id: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error parsing engine_id parameter");
        return ESP_FAIL;
    }
    else if (ret == ESP_ERR_NOT_FOUND) {
        parseConfigParams(query, updateValues);
        if (updateValues.empty()) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No configuration parameters provided");
            return ESP_FAIL;
        }
        db.update(tableName, updateValues);
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    }

    ESP_LOGI(TAG, "engine_id: %s", engineId);

    // Check if engine_id exists in the database
    auto searchResults = db.search(tableName, {"engine_id"}, std::make_pair("engine_id", (const char*)engineId));
    if (!searchResults.empty()) {
        parseConfigParams(query, updateValues);
        if (updateValues.empty()) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No configuration parameters provided");
            return ESP_FAIL;
        }
        db.update(tableName, updateValues, std::make_pair("engine_id", (const char*)engineId));
    } else {
        ESP_LOGE(TAG, "engine_id not found in the database");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "engine_id not found in the database");
        return ESP_FAIL;
    }

    // Send success response
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

static esp_err_t faviconGetHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_data_start, favicon_data_end - favicon_data_start);
    return ESP_OK;
}

static esp_err_t userLogin(httpd_req_t *req) 
{
    char query[256];
    const char* responseSuccess = "User login successful";
    const char* responseMissingParams = "missing username or password";
    const char* response = responseSuccess;

    esp_err_t ret = ESP_OK; // Initialize to ESP_OK

    // Get the query parameters from the request
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error parsing query parameters");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, responseMissingParams);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Query: %s", query);

    auto queryParams = getQueryParameters(query);
    if (queryParams.empty() || !hasKey(queryParams, "username") || !hasKey(queryParams, "password")) 
    {
        // If query parameters are missing or both username and password are not present
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, responseMissingParams);
        return ESP_FAIL;
    }

    ret = UserCredentialsManager::authenticateUser(queryParams["username"], queryParams["password"]);

    // Check the result of the authentication
    if (ret != ESP_OK) 
    {
        // Send an HTTP error response with appropriate status code
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Authentication failed");
        return ESP_FAIL;
    }

    // Send response
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
}

static esp_err_t createUser(httpd_req_t *req) 
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

        auto queryParams = getQueryParameters(query);
        if (!queryParams.empty())
        {
            if (hasKey(queryParams, "username") && hasKey(queryParams, "password"))
            {
                if (ESP_OK == UserCredentialsManager::createUser(queryParams["username"], queryParams["password"]))
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

static esp_err_t removeUser(httpd_req_t *req) 
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

        auto queryParams = getQueryParameters(query);
        if (!queryParams.empty())
        {
            if (hasKey(queryParams, "username"))
            {
                if(ESP_OK == UserCredentialsManager::removeUser(queryParams["username"]))
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

static esp_err_t updatePassword(httpd_req_t *req) 
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

        auto queryParams = getQueryParameters(query);
        if (!queryParams.empty())
        {
            if (hasKey(queryParams, "username") && hasKey(queryParams, "old_password") && hasKey(queryParams, "new_password"))
            {
                if(ESP_OK == UserCredentialsManager::updatePassword(queryParams["username"], queryParams["old_password"], queryParams["new_password"]))
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
