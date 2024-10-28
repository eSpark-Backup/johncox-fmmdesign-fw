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

static const char* TAG = "Device";

std::string Device::username;
std::string Device::password;
LocalDB& Device::dbPtr = LocalDB::getInstance(filename);

char* enginePosition[] = {"Port",
                          "Starboard",
                          "Keel",
                          "Port1",
                          "Port2",
                          "Starboard3",
                          "Starboard4",
                          "Genset1",
                          "Genset2"     };
char* engineFuelType[] = {"Gasoline",
                          "Diesel",
                          "Biodiesel"   };
char* engineType[] =     {"Drive Engine",
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

    for (const auto& vessel : vesselTable) 
    {
        vesselTableData = vessel;
        vesselTableData.erase("id");
    }

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
void Device::createTables(LocalDB& db)
{
    // Create SensorData table
    db.createTable("SensorData", "id INTEGER PRIMARY KEY NOT NULL, "
                                  "engine_id INTEGER NOT NULL, "
                                  "timestamp_unix INTEGER NOT NULL, "
                                  "fuel FLOAT NOT NULL, "
                                  "temperature_supply FLOAT NOT NULL, "
                                  "temperature_return FLOAT NOT NULL");

    // Create FuelManagement table
    db.createTable("FuelManagement", "id INTEGER PRIMARY KEY NOT NULL, "
                                      "engine_id INTEGER NOT NULL, "
                                      "total_fuel_burned INTEGER NOT NULL, "
                                      "remaining_fuel INTEGER NOT NULL, "
                                      "distance_to_empty INTEGER NOT NULL");

    // Create Engines table
    db.createTable("Engines", "id TINYINT PRIMARY KEY NOT NULL, "
                              "name TEXT, "
                              "enabled BOOLEAN NOT NULL, "
                              "engine_type TEXT, "
                              "fuel_type TEXT, "
                              "position TEXT, "
                              "volume_per_cycle FLOAT");

    // Create Vessel table
    db.createTable("Vessel", "id INTEGER PRIMARY KEY NOT NULL, "
                             "speed FLOAT NOT NULL");

    // Create UserCredentials table
    db.createTable("UserCredentials", "id TINYINT PRIMARY KEY NOT NULL,"
                                      "username TEXT NOT NULL,"
                                      "password TEXT NOT NULL");

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
    static float fuel_burned = 1.0;
    float fuel_factor = 1;          // fuel conversion
    float vol_factor = 1;           // tank volumetric capacity


    // std::vector<std::map<std::string, std::string>> fuel_type = dbPtr.search("Engines", {"fuel_type"}, {});

    // for (const auto& type : fuel_type) 
    // {
    //     std::map<std::string, std::string> fuelType = type;
    //     if (fuelType.at("fuel_type") == "Gasoline")          {     fuel_factor = ;   }
    //     else if (fuelType.at("fuel_type") == "Diesel")       {     fuel_factor = ;   }
    //     else if (fuelType.at("fuel_type") == "Biodiesel")    {     fuel_factor = ;   }
    // }

    // Initialize random number generator with the seed
    std::mt19937 gen(static_cast<std::mt19937::result_type>(seed));
    std::uniform_real_distribution<float> speedDistribution(50.0, 80.0);

    // ESP_LOGW(TAG, "================");
    for (int i = 1; i <= 2; i++)
    // for (int i = 1; i <= 2; ++i)
    {
        std::map<std::string, std::string> sensorData = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"timestamp_unix", std::to_string(seed)},
            {"fuel", std::to_string(Sensors::getFuel((ENGINE_e)i, PROBE_1))},
            {"temperature_supply", std::to_string(Sensors::getTemp((ENGINE_e)i, PROBE_1))},
            {"temperature_return", std::to_string(Sensors::getTemp((ENGINE_e)i, PROBE_2))}
        };
        // printf("sensorData:\n");
        // for (const auto& pair : sensorData) {
        //     printf("%s: %s\n", pair.first.c_str(), pair.second.c_str());
        // }
        dbPtr.insert("SensorData", sensorData);
        
        // Insert random data into FuelManagement table
        float fuel = Sensors::getFuel((ENGINE_e)i, PROBE_1);
        std::map<std::string, std::string> fuelManagement = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"total_fuel_burned", std::to_string((100-fuel) * fuel_factor)},
            {"remaining_fuel", std::to_string(fuel * vol_factor)},
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

void Device::init() 
{
    // static const char* filename = "/spiffs/test";
    // LocalDB& db = LocalDB::getInstance(filename);
    // dbPtr = &db;

    dbPtr.init();

    unlink(filename);

    if (!dbPtr.open()) 
    {
        return;
    }

    createTables(dbPtr);
    insertRandomValues(dbPtr);
    // insertHardCodeValues();

    // db.remove("SensorData");

    // std::map<std::string, std::string> updateValues = {{"engine_id", "1005"}, {"fuel_return", "20.75"}};
    // std::map<std::string, std::string> filter = {{"engine_id", "1003"}};
    // db.update("SensorData", updateValues, filter);

    auto searchResults = dbPtr.search("SensorData", {"*"}, {});

    // // Display the search results
    for (const auto& result : searchResults) 
    {
        for (const auto& pair : result) {
            printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
        }
        printf("\n");
    }

    std::map<std::string, std::string> filter = {
        {"engine_id", "1005"},
        {"temperature_return", "30.2"},
    };
    dbPtr.remove("SensorData", filter);


    auto results = dbPtr.search("SensorData", {"*"}, {});

    // // Display the search results
    for (const auto& result : results) {
        for (const auto& pair : result) {
            printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
        }
        printf("\n");
    }

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