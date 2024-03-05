#include <sstream>
#include <utility>
#include "esp_log.h"
#include "esp_timer.h"
#include "LocalDB.hpp"

static const char *TAG = "LocalDB";

int printCallback(void *data, int argc, char **argv, char **azColName) {
    int i;
    printf("%s: \n", (const char*)data);
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

LocalDB::LocalDB(const char *filename) : filename(filename), db(nullptr) {}

LocalDB::~LocalDB() {
    close();
}

void LocalDB::init() {
    sqlite3_initialize();
}

bool LocalDB::open() {
    int rc = sqlite3_open(filename, &db);
    if (rc) {
        ESP_LOGE(TAG, "Can't open database: %s", sqlite3_errmsg(db));
        return false;
    } else {
        ESP_LOGI(TAG, "Opened database successfully");
        return true;
    }
}

void LocalDB::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

void LocalDB::createTable(const char *tableName, const char *columns) {
    char sql[512];
    snprintf(sql, sizeof(sql), "CREATE TABLE %s(%s);", tableName, columns);
    executeSQL(sql);
}

int LocalDB::executeSQL(const char *sql, int (*callback)(void*, int, char**, char**), void *callbackData) {
    char *zErrMsg = nullptr;
    ESP_LOGI(TAG, "%s", sql);
    int64_t start = esp_timer_get_time();
    int rc = sqlite3_exec(db, sql, callback, callbackData, &zErrMsg);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "SQL error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        ESP_LOGI(TAG, "Operation done successfully");
    }
    ESP_LOGI(TAG, "Time taken: %lldms", (esp_timer_get_time() - start) / 1000);
    return rc;
}

void LocalDB::insert(const char *tableName, std::map<std::string, std::string> data) {
    std::string columns;
    std::string values;

    // Construct the column names and values for the INSERT statement
    for (const auto& pair : data) 
    {
        columns += pair.first + ",";
        values += "'" + pair.second + "',";
    }

    // Remove the trailing comma
    columns.pop_back();
    values.pop_back();

    // Construct the INSERT statement
    char sql[512];
    snprintf(sql, sizeof(sql), "INSERT INTO %s (%s) VALUES (%s);", tableName, columns.c_str(), values.c_str());

    // Execute the INSERT statement
    executeSQL(sql, nullptr);
}

// std::vector<std::map<std::string, std::string>> LocalDB::search(const char *tableName, const char *condition) {
//     std::vector<std::map<std::string, std::string>> results;
//     std::string sql = "SELECT * FROM " + std::string(tableName);
//     if (condition != nullptr) {
//         sql += " WHERE " + std::string(condition);
//     }
//     executeSQL(sql.c_str(), [](void *data, int argc, char **argv, char **azColName) {
//         std::map<std::string, std::string> row;
//         for (int i = 0; i < argc; i++) {
//             row[azColName[i]] = argv[i] ? argv[i] : "NULL";
//         }
//         std::vector<std::map<std::string, std::string>> *results = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
//         results->push_back(row);
//         return 0;
//     }, &results);
//     return results;
// }

void testDatabases() 
{
    static const char* filename = "/spiffs/test.db";
    LocalDB db(filename);
    db.init();
    if (!db.open()) {
        return;
    }

    // Create SensorData table
    db.createTable("SensorData", "id INTEGER PRIMARY KEY NOT NULL, "
                                    "engine_id INTEGER NOT NULL, "
                                    "sensor_data_id INTEGER NOT NULL, "
                                    "fuel_management_data_id INTEGER NOT NULL, "
                                    "timestamp_unix INTEGER NOT NULL");

    // Create SensorsDataBlock table
    db.createTable("SensorsDataBlock", "id INTEGER PRIMARY KEY NOT NULL, "
                                         "fuel_supply FLOAT NOT NULL, "
                                         "fuel_return FLOAT NOT NULL, "
                                         "temperature_supply FLOAT NOT NULL, "
                                         "temperature_return FLOAT NOT NULL");

    // Create Configurations table
    db.createTable("Configurations", "engine_id INTEGER NOT NULL, "
                                       "time_interval INTEGER NOT NULL, "
                                       "status BOOLEAN NOT NULL");

    // Create FuelManagement table
    db.createTable("FuelManagement", "id INTEGER PRIMARY KEY NOT NULL, "
                                       "total_fuel_burned INTEGER NOT NULL, "
                                       "remaining_fuel INTEGER NOT NULL, "
                                       "distance_to_empty INTEGER NOT NULL");

    // Create Engines table
    db.createTable("Engines", "id INTEGER PRIMARY KEY NOT NULL");

    // Insert data into tables using insert method
    std::map<std::string, std::string> sensorData = {
        {"engine_id", "1001"},
        {"sensor_data_id", "2001"},
        {"fuel_management_data_id", "3001"},
        {"timestamp_unix", "1646659900"}
    };
    db.insert("SensorData", sensorData);

    std::map<std::string, std::string> sensorsDataBlock = {
        {"fuel_supply", "10.5"},
        {"fuel_return", "8.3"},
        {"temperature_supply", "25.6"},
        {"temperature_return", "30.2"}
    };
    db.insert("SensorsDataBlock", sensorsDataBlock);

    std::map<std::string, std::string> configurations = {
        {"engine_id", "1001"},
        {"time_interval", "60"},
        {"status", "1"}
    };
    db.insert("Configurations", configurations);

    std::map<std::string, std::string> fuelManagement = {
        {"total_fuel_burned", "100"},
        {"remaining_fuel", "50"},
        {"distance_to_empty", "200"}
    };
    db.insert("FuelManagement", fuelManagement);

    // Insert data into Engines table
    std::map<std::string, std::string> enginesData;
    db.insert("Engines", enginesData);

    // Read data from tables
    // db.executeSQL("SELECT * FROM SensorData", printCallback);
    // db.executeSQL("SELECT * FROM SensorsDataBlock", printCallback);
    // db.executeSQL("SELECT * FROM Configurations", printCallback);
    // db.executeSQL("SELECT * FROM FuelManagement", printCallback);
    // db.executeSQL("SELECT * FROM Engines", printCallback);

    db.search("SensorData", "");
    db.search("SensorsDataBlock", "");
    db.search("Configurations", "");
    db.search("FuelManagement", "");
    db.search("Engines", "");

    // Search for data in SensorData table
    std::vector<std::map<std::string, std::string>> searchResults = db.search("SensorData", "engine_id  = '1001'");
    for (const auto& result : searchResults) {
        for (const auto& pair : result) {
            printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
        }
        printf("\n");
    }

    db.close();
}

template<typename T>
void LocalDB::constructCondition(std::stringstream& condition, T&& key, T&& value) {
    condition << key << " = '" << value << "'";
}

template<typename T, typename... Args>
void LocalDB::constructCondition(std::stringstream& condition, T&& key, T&& value, Args&&... args) {
    condition << key << " = '" << value << "' AND ";
    constructCondition(condition, std::forward<Args>(args)...);
}

template<typename... Args>
std::vector<std::map<std::string, std::string>> LocalDB::search(const char* tableName, Args&&... args) {
    std::vector<std::map<std::string, std::string>> results;

    std::stringstream condition;
    constructCondition(condition, std::forward<Args>(args)...);

    std::string sql = "SELECT * FROM " + std::string(tableName) + " WHERE " + condition.str();
    executeSQL(sql.c_str(), [](void* data, int argc, char** argv, char** azColName) {
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        static_cast<std::vector<std::map<std::string, std::string>>*>(data)->push_back(row);
        return 0;
    });

    return results;
}