#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_https_server.h>

class HttpServer
{
    public:
        static void start(void);

};

#endif // HTTP_SERVER_H