#include "Device.hpp"
#include <string>
#include <map>
#include <random>
#include <ctime>
#include <esp_wifi.h>
#include <esp_log.h>
#include "Parser.hpp"
#include "LocalDB.hpp"
#include "DeviceStorage.hpp"
#include "Sensors.hpp"
#include "Storage.hpp"

#define STACK_SIZE (10 * 1024)

static const char* TAG = "Device";

std::string Device::username;
std::string Device::password;
TaskHandle_t Device::xHandle = NULL;
LocalDB& Device::dbPtr = LocalDB::getInstance(FILENAME);

const char* enginePosition[] = {"Port",
                                "Starboard",
                                "Keel",
                                "Port1",
                                "Port2",
                                "Starboard3",
                                "Starboard4",
                                "Genset1",
                                "Genset2"     };
const char* engineFuelType[] = {"Gasoline",
                                "Diesel",
                                "Biodiesel"   };
const char* engineType[] =     {"Drive Engine",
                                "Generator"   };

std::pair<std::string, std::string> Device::generateCredentials() 
{
    // Get MAC address of ESP32
    uint8_t mac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if(ret != ESP_OK)
    {
        return std::make_pair("", "");
    }

    // Convert MAC address to string
    std::stringstream stream;
    stream << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        stream << std::setw(2) << static_cast<int>(mac[i]);
    }
    std::string macAddress = stream.str();

    ESP_LOGE(TAG, "%s", macAddress.c_str());
    // Generate username and password from MAC address
    username = "ESP_" + macAddress.substr(0, 6);
    password = macAddress.substr(6);

    return std::make_pair(username, password);
}

std::string Device::getMAC() 
{
    // Retrieve the MAC address using ESP-IDF API
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    // Format the MAC address as a string
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Return the MAC address as a string
    return std::string(macStr);
}

std::pair<std::string, std::string> Device::getCredentials()
{
    if(!username.empty() && !password.empty())
    {
        return std::make_pair(username, password);
    }

    return generateCredentials();
}

char* Device::getData()
{
    std::vector<engine_t> data;

    std::map<std::string, std::string> vesselTableData;
    std::vector<std::map<std::string, std::string>> vesselTable = dbPtr.search("Vessel", {"*"}, {}, true);
    // Retrieve data from Engines table and enabled engine ids
    std::vector<std::map<std::string, std::string>> enginesTable = dbPtr.search("Engines", {"*"}, {});

    if (data.empty()) 
    {
        for (int y = 0; y < enginesTable.size(); y++)
        {
            data.emplace_back();
        }
    }

    // Fetch and combine data for a given table for all enabled engines
    auto fetchCombinedDataForTable = [&](const std::string& tableName, const char* index) {
        std::map<std::string, std::string> combinedData;
            auto tableData = dbPtr.search(tableName.c_str(), {"*"}, {{"engine_id", index}}, true);
            for (const auto& mapData : tableData) {
                combinedData.insert(mapData.begin(), mapData.end());
            }
        combinedData.erase("id");
        combinedData.erase("engine_id");
        return combinedData;
    };

    int x = 0;
    if(!enginesTable.empty())
    {
        for (const auto& engine : enginesTable) 
        {
            data[x].engineData = engine;
            if (data[x].engineData.at("enabled") == "1") {
                data[x].tablesData["SensorData"] = fetchCombinedDataForTable("SensorData", data[x].engineData.at("id").c_str());
                data[x].tablesData["FuelManagement"] = fetchCombinedDataForTable("FuelManagement", data[x].engineData.at("id").c_str());
            }
            else
            {
                std::map<std::string, std::string> emptyMap;
                data[x].tablesData["SensorData"] = emptyMap;
                data[x].tablesData["FuelManagement"] = emptyMap;
                // printf("=====ELSE=====\nno values added.\n");
            }
            // data[x].engineData.erase("id");
            x++;
        }
    }
    else
        ESP_LOGE(TAG, "enginesTable vector is empty.");
    
    if(!vesselTable.empty())
    {
        for (const auto& vessel : vesselTable) 
        {
            vesselTableData = vessel;
            vesselTableData.erase("id");
        }
    }
    else
        ESP_LOGE(TAG, "vesselTable vector is empty.");

    // for (int y = 0; y < data.size(); y++) 
    // {
    //     printf("data:\n");
    //     for (const auto& [key, value] : data[y].engineData) {
    //             printf("\t%s: %s\n", key.c_str(), value.c_str());
    //     }
    //     for (const auto& [key, value] : data[y].tablesData) {
    //         printf("\t\t%s: \n", key.c_str());
    //         for (const auto& [key1, value1] : value) {
    //             printf("\t\t\t%s: %s\n", key1.c_str(), value1.c_str());
    //         }
    //     }
    // }

    // Convert collected data to JSON format
    // auto parsedData = Parser::convertMapToJSON(data);
    // printf("parsed data is: %s", parsedData);
    // return "ok";
    return Parser::convertMapToJSON(data, vesselTableData);
}

// Function to create tables
void Device::createTables()
{
    // Create SensorData table
    dbPtr.createTable("SensorData", "id INTEGER PRIMARY KEY NOT NULL, "
                                  "engine_id INTEGER NOT NULL, "
                                  "timestamp_unix INTEGER NOT NULL, "
                                  "temperature_supply FLOAT NOT NULL, "
                                  "temperature_return FLOAT NOT NULL");

    // Create FuelManagement table
    dbPtr.createTable("FuelManagement", "id INTEGER PRIMARY KEY NOT NULL, "
                                      "engine_id INTEGER NOT NULL, "
                                      "total_fuel_burned INTEGER NOT NULL, "
                                      "current_fuel_consumption INTEGER NOT NULL, "
                                      "distance_to_empty INTEGER NOT NULL");

    // Create Engines table
    dbPtr.createTable("Engines", "id TINYINT PRIMARY KEY NOT NULL, "
                              "name TEXT, "
                              "enabled BOOLEAN NOT NULL, "
                              "engine_type TEXT, "
                              "fuel_type TEXT, "
                              "position TEXT, "
                              "volume_per_cycle FLOAT");

    // Create Vessel table
    dbPtr.createTable("Vessel", "id INTEGER PRIMARY KEY NOT NULL, "
                             "speed FLOAT NOT NULL, "
                             "tank1_fuel FLOAT NOT NULL, "
                             "tank2_fuel FLOAT NOT NULL, "
                             "tank3_fuel FLOAT NOT NULL, "
                             "tank4_fuel FLOAT NOT NULL"        );

    // Create UserCredentials table
    dbPtr.createTable("UserCredentials", "id TINYINT PRIMARY KEY NOT NULL,"
                                      "username TEXT NOT NULL,"
                                      "password TEXT NOT NULL");
}

void Device::initDBTables()
{
    for (int i = 1; i <= 2; ++i) 
    {
        // Insert data into Engines table
        std::map<std::string, std::string> enginesData = {
            {"id", std::to_string(i)},
            {"name", "Engine M" + std::to_string(i)},
            {"engine_type", std::string(engineType[i-1])}, // Replace with appropriate engine type
            {"fuel_type", std::string(engineFuelType[i])},   // Replace with appropriate fuel type
            {"volume_per_cycle", std::to_string(i * 0.5)}, // Replace with appropriate volume per cycle
            {"enabled", std::to_string(1)},
            // {"enabled", std::to_string(i % 2)},
            {"position", std::string(enginePosition[i*2])} // Replace with appropriate volume per cycle
            // {"position", "P" + std::to_string(i * 0.5)} // Replace with appropriate volume per cycle
        };
        dbPtr.insert("Engines", enginesData);
    }
}

// Function to insert random values into tables
void Device::insertRandomValues(LocalDB& db)
{
    // Get current timestamp
    uint64_t seed = esp_timer_get_time();

    // Insert random data into SensorData table
    std::mt19937 gen(static_cast<std::mt19937::result_type>(seed));
    std::uniform_real_distribution<float> fuelDistribution(5.0, 20.0);
    std::uniform_real_distribution<float> temperatureDistribution(20.0, 40.0);
    std::uniform_real_distribution<float> speedDistribution(50.0, 80.0);

    for (int i = 1; i <= 10; ++i) 
    {
        std::map<std::string, std::string> sensorData = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"timestamp_unix", std::to_string(seed)},
            // {"speed", std::to_string(speedDistribution(gen))},
            {"fuel", std::to_string(fuelDistribution(gen))},
            {"temperature_supply", std::to_string(temperatureDistribution(gen))},
            {"temperature_return", std::to_string(temperatureDistribution(gen))}
        };
        db.insert("SensorData", sensorData);
        
        // Insert random data into FuelManagement table
        std::map<std::string, std::string> fuelManagement = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"total_fuel_burned", std::to_string(i * 100)},
            {"remaining_fuel", std::to_string(i * 500)},
            {"distance_to_empty", std::to_string(i * 200)}
        };
        db.insert("FuelManagement", fuelManagement);
    }
    static bool flag = false;
    if(!flag) {
        for (int i = 1; i <= 2; ++i) 
        {
            // Insert data into Engines table
            std::map<std::string, std::string> enginesData = {
                {"id", std::to_string(i)},
                {"name", "Engine M" + std::to_string(i)},
                {"engine_type", std::string(engineType[i-1])}, // Replace with appropriate engine type
                {"fuel_type", std::string(engineFuelType[i])},   // Replace with appropriate fuel type
                {"volume_per_cycle", std::to_string(i * 0.5)}, // Replace with appropriate volume per cycle
                {"enabled", std::to_string(1)},
                // {"enabled", std::to_string(i % 2)},
                {"position", std::string(enginePosition[i*2])} // Replace with appropriate volume per cycle
                // {"position", "P" + std::to_string(i * 0.5)} // Replace with appropriate volume per cycle
            };
            db.insert("Engines", enginesData);

        }
        // Insert data into Vessel table
        std::map<std::string, std::string> VesselData = {
            {"speed", std::to_string(speedDistribution(gen))}
        };
        db.insert("Vessel", VesselData);

        flag = true;
    }
}
// Function to insert random values into tables
void Device::insertHardCodeValues()
{
    // Get current time in microseconds to use as a seed
    uint64_t seed = esp_timer_get_time();
    static float fuel_burned = 1.0;

    // Initialize random number generator with the seed
    std::mt19937 gen(static_cast<std::mt19937::result_type>(seed));
    std::uniform_real_distribution<float> fuelDistribution(5.0, 20.0);
    std::uniform_real_distribution<float> temperatureDistribution(20.0, 40.0);
    std::uniform_real_distribution<float> speedDistribution(50.0, 80.0);

    // ESP_LOGW(TAG, "================");
    for (int i = 1; i <= 2; i++)
    // for (int i = 1; i <= 2; ++i)
    {
        std::map<std::string, std::string> sensorData = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"timestamp_unix", std::to_string(seed)},
            {"fuel", std::to_string(fuelDistribution(gen))},
            {"temperature_supply", std::to_string(temperatureDistribution(gen))},
            {"temperature_return", std::to_string(temperatureDistribution(gen))}
        };
        // printf("sensorData:\n");
        // for (const auto& pair : sensorData) {
        //     printf("%s: %s\n", pair.first.c_str(), pair.second.c_str());
        // }
        dbPtr.insert("SensorData", sensorData);
        
        // Insert random data into FuelManagement table
        std::map<std::string, std::string> fuelManagement = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"total_fuel_burned", std::to_string(i * (100 + fuel_burned))},
            {"remaining_fuel", std::to_string(i * (500 - fuel_burned))},
            {"distance_to_empty", std::to_string(i * (200 - (fuel_burned/2)))}
        };
        // printf("fuelManagement:\n");
        // for (const auto& pair : fuelManagement) {
        //     printf("%s: %s\n", pair.first.c_str(), pair.second.c_str());
        // }
        dbPtr.insert("FuelManagement", fuelManagement);
    }

    // Insert data into Vessel table
    std::map<std::string, std::string> VesselData = {
        {"speed", std::to_string(speedDistribution(gen))},
    };
    dbPtr.insert("Vessel", VesselData);
    // printf("Vessel:\n");
    // for (const auto& pair : VesselData) {
    //     printf("%s: %s\n", pair.first.c_str(), pair.second.c_str());
    // }

    fuel_burned += 1.4;
}

// Function to insert sensor values into tables
void Device::insertSensorValues()
{
    // Get current time in microseconds to use as a seed
    uint64_t seed = esp_timer_get_time();
    static float fuel_burned[ENGINE_MAX] = {0.0f};
    float fuel_factor = 1;      // set it to suitable volumetric value depending on tank

    // Storage::storageStats();

    // Initialize random number generator with the seed
    std::mt19937 gen(static_cast<std::mt19937::result_type>(seed));
    std::uniform_real_distribution<float> fuelDistribution(0, 100.0);
    std::uniform_real_distribution<float> temperatureDistribution(20.0, 40.0);
    std::uniform_real_distribution<float> speedDistribution(50.0, 80.0);
    std::uniform_real_distribution<float> flowDistribution(0.0, 960.0);

    auto fetchFlowData = [&](ENGINE_e engine, PROBE_e probe)
    {
        float val;
        if (Sensors::isTempSensorConnected())
            val = Sensors::getFlow(engine, probe);
        else
            val = flowDistribution(gen);
        return val;
    };

    auto fetchFuelData = [&](TANK_e tank) // Capture by reference
    {
        float val;
        if (Sensors::isFuelSensorConnected())
            val = Sensors::getFuel(tank);
        else
            val = fuelDistribution(gen);
        return val;
    };

    auto fetchTempData = [&](ENGINE_e engine, PROBE_e probe)
    {
        float val;
        if (Sensors::isTempSensorConnected())
            val = Sensors::getTemp(engine, probe);
        else
            val = temperatureDistribution(gen);
        return val;
    };

    auto fetchSpeedData = [&]()
    {
        float val;
        if (Sensors::isNMEA200BUSAvailable())
            // val = Sensors::getSpeed();       // Implement speed fetching sequence from NMEA2000 bus
            asm volatile ("nop"::);
        else
            val = speedDistribution(gen);
        return val;
    };

    float tankData[TANK_MAX] = {0};
    float remainingFuel = 0;
    for(int i=0; i<(TANK_MAX); i++)
    {
        tankData[(TANK_e)i] = fetchFuelData((TANK_e)i) * fuel_factor;
        remainingFuel += tankData[(TANK_e)i];
    }
    float vesselSpeed = fetchSpeedData();

    int error = SQLITE_OK;
    // ESP_LOGW(TAG, "================");
    for (int i = 1; i <= 2; i++)
    {
        float currentFuelConsumption = abs(fetchFlowData((ENGINE_e)(i-1), SUPPLY_FLOW) - fetchFlowData((ENGINE_e)(i-1), RETURN_FLOW));
        fuel_burned[i-1] += (currentFuelConsumption*((SENSOR_DATA_ACQUIRE_TIME/1000.0f)/3600.0f));

        std::map<std::string, std::string> sensorData = {
            {"engine_id",          std::to_string(i)},
            {"timestamp_unix",     std::to_string(seed)},
            {"temperature_supply", std::to_string(fetchTempData((ENGINE_e)(i-1), PROBE_1))},
            {"temperature_return", std::to_string(fetchTempData((ENGINE_e)(i-1), PROBE_2))},
        };
        error = dbPtr.insert("SensorData", sensorData);
        

        std::map<std::string, std::string> fuelManagement = {
            {"engine_id",          std::to_string(i)},
            {"total_fuel_burned",  std::to_string(fuel_burned[i-1])},
            {"current_fuel_consumption",     std::to_string(currentFuelConsumption)},
            {"distance_to_empty",  std::to_string(vesselSpeed*((remainingFuel/currentFuelConsumption)/60))},        // in Km
        };
        error = dbPtr.insert("FuelManagement", fuelManagement);
    }

    // Insert data into Vessel table
    std::map<std::string, std::string> VesselData = {
        {"speed", std::to_string(vesselSpeed)},
        {"tank1_fuel", std::to_string(tankData[TANK_1])},
        {"tank2_fuel", std::to_string(tankData[TANK_2])},
        {"tank3_fuel", std::to_string(tankData[TANK_3])},
        {"tank4_fuel", std::to_string(tankData[TANK_4])},
    };
    error = dbPtr.insert("Vessel", VesselData);

    // printf("Vessel:\n");
    // for (const auto& pair : VesselData) {
    //     printf("%s: %s\n", pair.first.c_str(), pair.second.c_str());
    // }
    if(error)
        gpio_set_level(FAULT_INDICATION_LED, 1);
}

void acquireSensorData(void * pvParameters)
{
//   configASSERT( ( ( uint32_t ) pvParameters ) == NULL );

    for( ;; )
    {
        vTaskDelay(SENSOR_DATA_ACQUIRE_TIME / portTICK_PERIOD_MS);
        Device::insertSensorValues();
    }
}

void Device::startDataAcquisitionTask()
{
    vTaskResume(Device::xHandle);
    Sensors::startFlowRateTask();
    ESP_LOGI(TAG, "DataAcquisition task started.");
}

void Device::suspendDataAcquisitionTask()
{
    vTaskSuspend(Device::xHandle);
    Sensors::suspendFlowRateTask();
    ESP_LOGI(TAG, "DataAcquisition task suspended.");
}

void Device::createDataAcquisitionTask()
{
    BaseType_t err = xTaskCreate(
                    acquireSensorData,              /* Function that implements the task. */
                    "insertSensorValues",           /* Text name for the task. */
                    STACK_SIZE,                     /* Stack size in words, not bytes. */
                    ( void * ) NULL,                /* Parameter passed into the task. */
                    tskIDLE_PRIORITY+5,             /* Priority at which the task is created. */
                    &Device::xHandle );             /* Used to pass out the created task's handle. */
    if(pdPASS  == err)
    {
        ESP_LOGI(TAG, "DataAcquisition task created.");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to allocate memory for DataAcquisition task.");
    }
    
    suspendDataAcquisitionTask();
    
}

void Device::init() 
{
    gpio_set_direction(POWER_INDICATION_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(FAULT_INDICATION_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(NMEA_INDICATION_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(POWER_INDICATION_LED, 1);
    dbPtr.init(Storage::MOUNT_POINT);
    // Storage::storageStats();

    if (!dbPtr.open()) 
    {
        return;
    }

    createTables();
    initDBTables();
    // insertRandomValues(dbPtr);
    // insertHardCodeValues();

    createDataAcquisitionTask();


    // // Display the search results
    // for (const auto& result : searchResults) 
    // {
    //     for (const auto& pair : result) {
    //         printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
    //     }
    //     printf("\n");
    // }

    // std::map<std::string, std::string> filter = {
    //     {"engine_id", "1005"},
    //     {"temperature_return", "30.2"},
    // };
    // dbPtr.remove("SensorData", filter);


    // auto results = dbPtr.search("SensorData", {"*"}, {});

    // // // Display the search results
    // for (const auto& result : results) {
    //     for (const auto& pair : result) {
    //         printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
    //     }
    //     printf("\n");
    // }

    // db.printLastError();

    // db.close();
}

esp_err_t Device::saveDomainName(const std::string& domainName)
{
    return DeviceStorage::saveDomainName(domainName);
}

std::string Device::getDomainName()
{
    return DeviceStorage::getDomainName();
}

bool Device::isDomainNameValid()
{
    std::string domainName = DeviceStorage::getDomainName();

    if(!domainName.empty())
    {
        size_t pos = domainName.find(DOMAIN_NAME_PREFIX);
        if(pos != std::string::npos)
        {
            if(pos == 0)
            {
                return true;
            }
        }
    }

    return false;
}