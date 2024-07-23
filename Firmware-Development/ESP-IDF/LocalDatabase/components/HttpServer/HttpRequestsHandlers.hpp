#pragma once
#include <esp_https_server.h>
#include <esp_err.h>

esp_err_t helloworld(httpd_req_t *req);
esp_err_t getData(httpd_req_t *req);
esp_err_t getEngines(httpd_req_t *req);
esp_err_t setConfigurations(httpd_req_t *req);
esp_err_t createUser(httpd_req_t *req);
esp_err_t removeUser(httpd_req_t *req);
esp_err_t updatePassword(httpd_req_t *req);
esp_err_t userLogin(httpd_req_t *req);
esp_err_t faviconGetHandler(httpd_req_t *req);
esp_err_t startProvision(httpd_req_t *req);

