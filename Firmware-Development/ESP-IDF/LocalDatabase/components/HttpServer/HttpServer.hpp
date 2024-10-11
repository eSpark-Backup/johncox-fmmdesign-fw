#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_https_server.h>
#include <KeepAlive.hpp>

// #define USE_SECURE_SERVER

typedef char* (*CallbackFunction)(void);

class HttpServer
{
    public:
        static void init(char* (*callback)(void) = NULL);
        static void start(void);

        static esp_err_t stopServer();
        static void wssServerTask(void* args);
       
        static bool isStarted(void)
        {
            return serverHandle != NULL;
        }

    private:

        static esp_err_t startServer(void * args);

        static void unregisterUriHandlers(void);

        static httpd_handle_t serverHandle;
        static wss_keep_alive_t keepAlive;
        static TaskHandle_t websocketTaskHandle;
};


#endif // HTTP_SERVER_H