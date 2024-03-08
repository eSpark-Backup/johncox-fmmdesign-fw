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

static const char *TAG = "HTTPS_SERVER";

extern const uint8_t favicon_data_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_data_end[] asm("_binary_favicon_ico_end");

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

/* An HTTP GET handler */
esp_err_t helloworld(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET request for /hello");
    const char *response = "<!DOCTYPE html> <html> <body> <h1>Electronics</h1> <p>Embedded Systems</p> </body> </html>";
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Handler function
static esp_err_t getData(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;

    char query[256];
    char engineId[10];
    const char* response = "<!DOCTYPE html> <html> <body> <h1>Wrong Parameters</h1></body> </html>";

    esp_err_t ret = ESP_FAIL;

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Query: %s", query);

        // Parse the query parameters
        ret = httpd_query_key_value(query, "engine_id", engineId, sizeof(engineId));
        if (ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "engine_id: %s", engineId);
            LocalDB& db = LocalDB::getInstance();
            auto searchResults = db.search(tableName, {"*"}, std::make_pair("engine_id", (const char*) engineId));
            
            char* resp = convertSearchResultsToJSON(searchResults);
            httpd_resp_send(req, resp, strlen(resp));
            free(resp);
            goto success;
        }
        else
        {
            ESP_LOGE(TAG, "Error : %s", esp_err_to_name(ret));
        }
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
        LocalDB& db = LocalDB::getInstance();
        auto searchResults = db.search(tableName, {"*"});
        
        char* resp = convertSearchResultsToJSON(searchResults);
        httpd_resp_send(req, resp, strlen(resp));
        free(resp);
        goto success;
    }


    httpd_resp_send(req, response, strlen(response));

    success:
    return ESP_OK;
}

static esp_err_t setConfigurations(httpd_req_t *req) 
{
    const char* tableName = (const char*) req->user_ctx;

    char query[256];
    char engineId[10];
    char status[10];
    char timeInterval[10];

    esp_err_t ret = ESP_FAIL;

    // Get the query parameters from the request
    ret = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Query: %s", query);

        // Parse the query parameters
        ret = httpd_query_key_value(query, "engine_id", engineId, sizeof(engineId));
        if (ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "engine_id: %s", engineId);

            // Parse other parameters
            ret = httpd_query_key_value(query, "status", status, sizeof(status));
            if (ret != ESP_OK) 
            {
                ESP_LOGE(TAG, "Error parsing status: %s", esp_err_to_name(ret));
            }

            ret = httpd_query_key_value(query, "time_interval", timeInterval, sizeof(timeInterval));
            if (ret != ESP_OK) 
            {
                ESP_LOGE(TAG, "Error parsing time_interval: %s", esp_err_to_name(ret));
            }

            // Update configurations for the specified engine_id
            LocalDB& db = LocalDB::getInstance();
            std::map<std::string, std::string> updateValues = {
                {"status", status},
                {"time_interval", timeInterval}
            };
            db.update(tableName, updateValues, std::make_pair("engine_id", engineId));
        }
        else
        {
            ESP_LOGE(TAG, "Error parsing engine_id: %s", esp_err_to_name(ret));
            return ESP_FAIL;
        }
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
        // If engine_id is not provided, update configurations for all engines
        LocalDB& db = LocalDB::getInstance();
        std::map<std::string, std::string> updateValues = {
            {"status", status},
            {"time_interval", timeInterval}
        };
        // db.update(tableName, updateValues);
    }
    else
    {
        ESP_LOGE(TAG, "Error parsing query parameters: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    // Send response
    const char* response = "<!DOCTYPE html> <html> <body> <h1>Configurations updated successfully</h1></body> </html>";
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
}


esp_err_t faviconGetHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_data_start, favicon_data_end - favicon_data_start);
    return ESP_OK;
}

/* URI handlers array */
httpd_uri_t uri_handlers[] = 
{
    { .uri = "/helloWorld",         .method = HTTP_GET,     .handler = helloworld,          .user_ctx  = NULL},
    { .uri = "/getSensorData",      .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"SensorData"},
    { .uri = "/getFuelManagement",  .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"FuelManagement"},
    { .uri = "/getConfigurations",  .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"Configurations"},
    { .uri = "/getEngines",         .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"Engines"},
    { .uri = "/setConfigurations",  .method = HTTP_GET,     .handler = setConfigurations,   .user_ctx  = (void*)"Configurations"},
    { .uri = "/favicon.ico",        .method = HTTP_GET,     .handler = faviconGetHandler,   .user_ctx  = NULL}
};



/* Start the HTTP server */
void HttpServer::start(void)
{
    // httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    config.httpd.max_uri_handlers = 10;
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
