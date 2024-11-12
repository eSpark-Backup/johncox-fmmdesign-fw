#pragma once

#include <stdio.h>
#include <stdint.h>
#include <esp_event.h>
#include <esp_https_server.h>
#include "KeepAlive.hpp"

#define MAX_CLIENTS 4

esp_err_t ws_handler(httpd_req_t *req);
esp_err_t wss_open_fd(httpd_handle_t hd, int sockfd);
void wss_close_fd(httpd_handle_t hd, int sockfd);
bool client_not_alive_cb(wss_keep_alive_t h, int fd);
bool check_client_alive_cb(wss_keep_alive_t h, int fd);
httpd_handle_t start_wss_echo_server(void);
void stop_wss_echo_server(httpd_handle_t server);
void wss_server_send_messages(httpd_handle_t* server, char* (*dataCallback)(void) = NULL);