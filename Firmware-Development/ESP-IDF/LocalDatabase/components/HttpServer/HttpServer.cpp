#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "HttpServer.hpp"
#include "Websockets.hpp"
#include "HttpRequestsHandlers.hpp"

static const char *TAG = "HTTPS_SERVER";
static const size_t max_clients = 4;

/* URI handlers array */
httpd_uri_t uri_handlers[] = 
{
    { .uri = "/helloWorld",         .method = HTTP_GET,     .handler = helloworld,          .user_ctx  = NULL,                      .is_websocket = false,     .handle_ws_control_frames = false},
    { .uri = "/getSensorData",      .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"SensorData",      .is_websocket = false,     .handle_ws_control_frames = false},
    { .uri = "/getFuelManagement",  .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"FuelManagement",  .is_websocket = false,     .handle_ws_control_frames = false},
    { .uri = "/getEngines",         .method = HTTP_GET,     .handler = getEngines,             .user_ctx  = (void*)"Engines",          .is_websocket = false,     .handle_ws_control_frames = false},
    // { .uri = "/getConfigurations",  .method = HTTP_GET,     .handler = getData,             .user_ctx  = (void*)"Engines",  .is_websocket = false,     .handle_ws_control_frames = false},
    { .uri = "/setConfigurations",  .method = HTTP_POST,     .handler = setConfigurations,   .user_ctx  = (void*)"Engines",  .is_websocket = false,     .handle_ws_control_frames = false},
    // { .uri = "/createUser",         .method = HTTP_GET,     .handler = createUser,          .user_ctx  = NULL},
    // { .uri = "/removeUser",         .method = HTTP_GET,     .handler = removeUser,          .user_ctx  = NULL},
    // { .uri = "/updatePassword",     .method = HTTP_GET,     .handler = updatePassword,      .user_ctx  = NULL},
    { .uri = "/userLogin",          .method = HTTP_POST,    .handler = userLogin,           .user_ctx  = NULL,                      .is_websocket = false,     .handle_ws_control_frames = false},
    { .uri = "/favicon.ico",        .method = HTTP_GET,     .handler = faviconGetHandler,   .user_ctx  = NULL,                      .is_websocket = false,     .handle_ws_control_frames = false},
    { .uri = "/ws",                 .method = HTTP_GET,     .handler = ws_handler,          .user_ctx  = NULL,                     .is_websocket = true,      .handle_ws_control_frames = true},
    { .uri = "/startprovision",     .method = HTTP_POST,     .handler = startProvision,     .user_ctx  = NULL,                     .is_websocket = false,     .handle_ws_control_frames = false}
};

/* Start the HTTP server */
void HttpServer::start(char* (*dataCallback)(void))
{
    # if 0
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 15;
    config.max_open_sockets = 6;

    esp_err_t ret = httpd_start(&server, &config);
    #else
    static httpd_handle_t server = NULL;

    // Prepare keep-alive engine
    wss_keep_alive_config_t keep_alive_config = KEEP_ALIVE_CONFIG_DEFAULT();
    keep_alive_config.max_clients = max_clients;
    keep_alive_config.client_not_alive_cb = client_not_alive_cb;
    keep_alive_config.check_client_alive_cb = check_client_alive_cb;
    wss_keep_alive_t keep_alive = wss_keep_alive_start(&keep_alive_config);

    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    config.httpd.ctrl_port = 55555;
    config.httpd.max_uri_handlers = 15;
    config.httpd.max_open_sockets = 6;
    config.httpd.max_open_sockets = max_clients;
    config.httpd.global_user_ctx = keep_alive;
    config.httpd.open_fn = wss_open_fd;
    config.httpd.close_fn = wss_close_fd;
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
    #endif

    if (ret == ESP_OK) 
    {
        for (int i = 0; i < sizeof(uri_handlers) / sizeof(uri_handlers[0]); i++) 
        {
            if (httpd_register_uri_handler(server, &uri_handlers[i]) != ESP_OK) 
            {
                ESP_LOGE(TAG, "Failed to register URI handler");
            }
        }
        wss_keep_alive_set_user_ctx(keep_alive, server);
        ESP_LOGI(TAG, "HTTPS Server started");
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to start HTTP Server : %s", esp_err_to_name(ret));
    }

    wss_server_send_messages(&server, dataCallback);
}


