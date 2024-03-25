#pragma once

#include <esp_err.h>
class MDNSService
{
    public:
        static esp_err_t start(const char* hostName);

};