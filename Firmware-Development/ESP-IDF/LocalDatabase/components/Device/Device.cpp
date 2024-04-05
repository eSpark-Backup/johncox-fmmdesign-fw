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

static const char* TAG = "Device";

std::string Device::username;
std::string Device::password;

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

std::pair<std::string, std::string> Device::getCredentials()
{
    if(!username.empty() && !password.empty())
    {
        return std::make_pair(username, password);
    }

    return generateCredentials();
}

char* Device::getData() {
    // Map to hold the search results
    std::map<std::string, std::vector<std::map<std::string, std::string>>> jsonData;
    
    // Retrieve data from Engines table to get all ids
    LocalDB& db = LocalDB::getInstance();
    std::vector<std::map<std::string, std::string>> enginesData = db.search("Engines", {"*"}, {});

    std::vector<std::string> enabledEngineIds;
    std::vector<std::map<std::string, std::string>> configurations = db.search("Configurations", {"engine_id"}, {{"status", "1"}});
    for (auto& config : configurations) {
        enabledEngineIds.push_back(config["engine_id"]);
    }

    // Function to fetch and combine data for a given table
    auto fetchAndCombineData = [&](const char* tableName) {
        std::vector<std::map<std::string, std::string>> combinedData;
        for (auto& engineId : enabledEngineIds) {

            // Retrieve data from the table for this engine_id
            std::vector<std::map<std::string, std::string>> tableData = db.search(tableName, {"*"}, {{"engine_id", engineId}}, true);

            // Combine tableData into combinedData
            combinedData.insert(combinedData.end(), tableData.begin(), tableData.end());
        }
        return combinedData;
    };

    // Combine data for SensorData and add it to jsonData
    jsonData["SensorData"] = fetchAndCombineData("SensorData");

    // Combine data for FuelManagement and add it to jsonData
    jsonData["FuelManagement"] = fetchAndCombineData("FuelManagement");

    jsonData["Engines"] = enginesData;

    // Convert jsonData to JSON
    char* jsonResult = Parser::convertMapToJSON(jsonData);
    
    return jsonResult;
}


// Function to create tables
void Device::createTables(LocalDB& db) {
    // Create SensorData table
    db.createTable("SensorData", "id INTEGER PRIMARY KEY NOT NULL, "
                                  "engine_id INTEGER NOT NULL, "
                                  "trip_id INTEGER NOT NULL, "
                                  "timestamp_unix INTEGER NOT NULL, "
                                  "fuel_supply FLOAT NOT NULL, "
                                  "fuel_return FLOAT NOT NULL, "
                                  "temperature_supply FLOAT NOT NULL, "
                                  "temperature_return FLOAT NOT NULL");

    // Create Configurations table
    db.createTable("Configurations", "engine_id INTEGER NOT NULL, "
                                      "status BOOLEAN NOT NULL");

    // Create FuelManagement table
    db.createTable("FuelManagement", "id INTEGER PRIMARY KEY NOT NULL, "
                                      "engine_id INTEGER NOT NULL, "
                                      "total_fuel_burned INTEGER NOT NULL, "
                                      "remaining_fuel INTEGER NOT NULL, "
                                      "distance_to_empty INTEGER NOT NULL");

    // Create Engines table
    db.createTable("Engines", "id TINYINT PRIMARY KEY NOT NULL, "
                          "name TEXT, "
                          "engine_type TEXT, "
                          "fuel_type TEXT, "
                          "volume_per_cycle FLOAT");

    // Create UserCredentials table
    db.createTable("UserCredentials", "id TINYINT PRIMARY KEY NOT NULL,"
                                      "username TEXT NOT NULL,"
                                      "password TEXT NOT NULL");

    // Create Trips table
    db.createTable("Trips", "id TINYINT PRIMARY KEY NOT NULL, "
                            "name TEXT NOT NULL");
}

// Function to insert random values into tables
void Device::insertRandomValues(LocalDB& db) {
    // Get current timestamp
    std::time_t currentTime = std::time(nullptr);

    // Insert random data into SensorData table
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> fuelDistribution(5.0, 20.0);
    std::uniform_real_distribution<float> temperatureDistribution(20.0, 40.0);

    for (int i = 1; i <= 10; ++i) 
    {
        std::map<std::string, std::string> sensorData = {
            {"engine_id", (i % 2 == 0) ? "1" : "2"},
            {"trip_id", std::to_string(i)},
            {"timestamp_unix", std::to_string(currentTime)},
            {"fuel_supply", std::to_string(fuelDistribution(gen))},
            {"fuel_return", std::to_string(fuelDistribution(gen))},
            {"temperature_supply", std::to_string(temperatureDistribution(gen))},
            {"temperature_return", std::to_string(temperatureDistribution(gen))}
        };
        db.insert("SensorData", sensorData);

        // Insert data into Trips table
        std::map<std::string, std::string> tripsData = {
            {"id", std::to_string(i)},
            {"name", "Trip " + std::to_string(i)}
        };
        db.insert("Trips", tripsData);
        
        // Insert random data into FuelManagement table
        std::map<std::string, std::string> fuelManagement = {
            {"engine_id", std::to_string(i)},
            {"total_fuel_burned", std::to_string(i * 100)},
            {"remaining_fuel", std::to_string(i * 50)},
            {"distance_to_empty", std::to_string(i * 200)}
        };
        db.insert("FuelManagement", fuelManagement);
    }

    for (int i = 1; i <= 2; ++i) 
    {
        // Insert data into Engines table
        std::map<std::string, std::string> enginesData = {
            {"id", std::to_string(i)},
            {"name", "Engine M" + std::to_string(i)},
            {"engine_type", "Type " + std::to_string(i)}, // Replace with appropriate engine type
            {"fuel_type", "Type " + std::to_string(i)},   // Replace with appropriate fuel type
            {"volume_per_cycle", std::to_string(i * 0.5)} // Replace with appropriate volume per cycle
        };
        db.insert("Engines", enginesData);

        // Insert random data into Configurations table
        std::map<std::string, std::string> configurations = {
            {"engine_id", std::to_string(i)},
            {"status", (i % 2 == 0) ? "1" : "0"} // Randomly set status to 1 or 0
        };
        db.insert("Configurations", configurations);

    }
}

void Device::init() 
{
    static const char* filename = "/spiffs/test.db";
    LocalDB& db = LocalDB::getInstance(filename);
    db.init();

    unlink("/spiffs/test.db");

    if (!db.open()) 
    {
        return;
    }

    createTables(db);
    insertRandomValues(db);

    // db.remove("SensorData");

    // std::map<std::string, std::string> updateValues = {{"engine_id", "1005"}, {"fuel_return", "20.75"}};
    // std::map<std::string, std::string> filter = {{"engine_id", "1003"}};
    // db.update("SensorData", updateValues, filter);

    auto searchResults = db.search("SensorData", {"*"}, {});

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
    db.remove("SensorData", filter);


    auto results = db.search("SensorData", {"*"}, {});

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