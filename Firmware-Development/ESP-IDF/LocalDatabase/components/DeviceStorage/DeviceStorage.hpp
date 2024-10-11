#pragma once

#include <string>
#include "esp_err.h"

class DeviceStorage 
{
    public:
        static esp_err_t saveDomainName(const std::string& domainName);
        static std::string getDomainName();
};
