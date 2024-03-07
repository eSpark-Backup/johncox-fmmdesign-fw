#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_https_server.h"

static const char *TAG = "HTTPS_SERVER";

extern const uint8_t favicon_data_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_data_end[] asm("_binary_favicon_ico_end");

/* An HTTP GET handler */
esp_err_t helloworld(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET request for /hello");
    const char *response = "#Hello World";
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

esp_err_t testHandler(httpd_req_t *req)
{
    char query[256];
    char sensorId[10];

    // Get the query parameters from the request
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) 
    {
        ESP_LOGI(TAG, "%s", query);
        // Parse the query parameters
        if (httpd_query_key_value(query, "sensor_id", sensorId, sizeof(sensorId)) == ESP_OK)
        {
            ESP_LOGI(TAG, "sensor_id: %s", sensorId);
        }
    }

    // Print the value of the 'sensor_id' parameter
    // Send response
    const char* response = "Handler function executed";
    httpd_resp_send(req, response, strlen(response));

    // Free query string memory
    // free(query);

    return ESP_OK;
}


esp_err_t favicon_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_data_start, favicon_data_end - favicon_data_start);
    return ESP_OK;
}

/* URI handlers array */
httpd_uri_t uri_handlers[] = 
{
    { .uri = "/helloworld",     .method = HTTP_GET, .handler = helloworld,         .user_ctx  = NULL},
    { .uri = "/getSensorData",     .method = HTTP_GET, .handler = testHandler,         .user_ctx  = NULL},
    { .uri = "/getFuelManagement",     .method = HTTP_GET, .handler = testHandler,         .user_ctx  = NULL},
    { .uri = "/getConfigurations",     .method = HTTP_GET, .handler = testHandler,         .user_ctx  = NULL},
    { .uri = "/favicon.ico",    .method = HTTP_GET, .handler = favicon_get_handler, .user_ctx  = NULL}
};



/* Start the HTTP server */
void httpsServerStart(void)
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
