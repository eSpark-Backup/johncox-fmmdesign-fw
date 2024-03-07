#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_https_server.h>

#ifdef __cplusplus
extern "C" {
#endif

void httpsServerStart(void);

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H