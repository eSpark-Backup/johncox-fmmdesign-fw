#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <string>
#include <utility>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <esp_err.h>
#include "LocalDB.hpp"

#define FAULT_INDICATION_LED        GPIO_NUM_4
#define POWER_INDICATION_LED        GPIO_NUM_23
#define NMEA_INDICATION_LED         GPIO_NUM_19

#define SENSOR_DATA_ACQUIRE_TIME    5000

#define DOMAIN_NAME_PREFIX "timesp"
#define FILENAME "/test"

typedef struct
{
    std::map<std::string, std::string> engineData;
    std::map<std::string, std::map<std::string, std::string>> tablesData;
}engine_t;

void acquireSensorData(void * pvParameters);

class Device 
{
    private:
        static LocalDB& dbPtr;
        static std::string username;
        static std::string password;
        static TaskHandle_t xHandle;
        static std::pair<std::string, std::string> generateCredentials();
        static void createTables();
        static void initDBTables();
        static void createDataAcquisitionTask();
        
    public:
        static void insertRandomValues(LocalDB& db);
        static void insertHardCodeValues();
        static void insertSensorValues();
        static void startDataAcquisitionTask();
        static void suspendDataAcquisitionTask();
        static char* getData();
        static std::pair<std::string, std::string> getCredentials();
        static void init();
        static esp_err_t saveDomainName(const std::string& domainName);
        static std::string getDomainName();
        static bool isDomainNameValid();
        static std::string getMAC();
};

#endif // DEVICE_HPP
