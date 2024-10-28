#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <string>
#include <utility>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <esp_err.h>
#include "LocalDB.hpp"

static const char* filename = "/spiffs/test";

typedef struct
{
    std::map<std::string, std::string> engineData;
    std::map<std::string, std::map<std::string, std::string>> tablesData;
}engine_t;

class Device 
{
    private:
        static LocalDB& dbPtr;
        static std::string username;
        static std::string password;
        static std::pair<std::string, std::string> generateCredentials();
        static void createTables(LocalDB& db);
        
    public:
        static void insertRandomValues(LocalDB& db);
        static void insertHardCodeValues();
        static void insertSensorValues();
        static char* getData();
        static std::pair<std::string, std::string> getCredentials();
        static void init();
        static esp_err_t saveDomainName(const std::string& domainName);
        static std::string getDomainName();
        static std::string getMAC();
};

#endif // DEVICE_HPP
