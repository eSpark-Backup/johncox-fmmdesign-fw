#include <sstream>
#include <utility>
#include "esp_log.h"
#include "esp_timer.h"
#include "LocalDB.hpp"

static const char *TAG = "LocalDB";

LocalDB* LocalDB::instance = nullptr;

LocalDB::LocalDB(const char *filename) : filename(filename), db(nullptr) {}

LocalDB::~LocalDB() {
    close();
}

LocalDB& LocalDB::getInstance(const char *filename) {
    if (!instance) {
        instance = new LocalDB(filename);
    }
    return *instance;
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

int LocalDB::insert(const char *tableName, std::map<std::string, std::string> data) {
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
    return executeSQL(sql, nullptr);
}

void LocalDB::constructCondition(std::stringstream& condition, const std::pair<const char*, std::string>& pair) {
    condition << pair.first << "='" << pair.second << "'";
}

template<typename... Args>
void LocalDB::constructCondition(std::stringstream& condition, const std::pair<const char*, std::string>& pair, Args&&... args) {
    condition << " AND " << pair.first << "='" << pair.second << "'";
    constructCondition(condition, std::forward<Args>(args)...);
}

template<typename... Args>
std::vector<std::map<std::string, std::string>> LocalDB::search(const char* tableName, std::initializer_list<const char*> select, Args&&... where) {
    std::vector<std::map<std::string, std::string>> results;

    std::stringstream condition;
    if constexpr (sizeof...(where) > 0) {
        constructCondition(condition, std::forward<Args>(where)...);
    }

    std::string selectStr = "";
    for (const char* key : select) {
        selectStr += (selectStr.empty() ? "" : ", ") + std::string(key);
    }

    std::string sql = "SELECT " + selectStr + " FROM " + std::string(tableName);
    if (!condition.str().empty()) {
        sql += " WHERE " + condition.str();
    }

    executeSQL(sql.c_str(), [](void* data, int argc, char** argv, char** azColName) {
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        static_cast<std::vector<std::map<std::string, std::string>>*>(data)->push_back(row);
        return 0;
    }, &results);

    return results;
}

// Template specialization for search function without where conditions
template<>
std::vector<std::map<std::string, std::string>> LocalDB::search(const char* tableName, std::initializer_list<const char*> select) {
    std::vector<std::map<std::string, std::string>> results;

    std::string selectStr = "";
    for (const char* key : select) {
        selectStr += (selectStr.empty() ? "" : ", ") + std::string(key);
    }

    std::string sql = "SELECT " + selectStr + " FROM " + std::string(tableName);

    executeSQL(sql.c_str(), [](void* data, int argc, char** argv, char** azColName) {
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        static_cast<std::vector<std::map<std::string, std::string>>*>(data)->push_back(row);
        return 0;
    }, &results);

    return results;
}

template<typename... Args>
int LocalDB::update(const char* tableName, const std::map<std::string, std::string>& update, Args&&... where) {
    std::stringstream updateValues;
    constructUpdateValues(updateValues, update);

    std::stringstream condition;
    if constexpr (sizeof...(where) > 0) {
        constructCondition(condition, std::forward<Args>(where)...);
    }

    std::string sql = "UPDATE " + std::string(tableName) + " SET " + updateValues.str();
    if (!condition.str().empty()) {
        sql += " WHERE " + condition.str();
    }

    return executeSQL(sql.c_str(), nullptr);
}

template<>
int LocalDB::update(const char* tableName, const std::map<std::string, std::string>& update) {
    std::stringstream updateValues;
    constructUpdateValues(updateValues, update);

    std::string sql = "UPDATE " + std::string(tableName) + " SET " + updateValues.str();

    return executeSQL(sql.c_str(), nullptr);
}

template<typename... Args>
void LocalDB::constructUpdateValues(std::stringstream& updateValues, const std::map<std::string, std::string>& values) {
    for (const auto& pair : values) {
        updateValues << pair.first << "='" << pair.second << "',";
    }
    // Remove the trailing comma
    std::string valuesStr = updateValues.str();
    if (!valuesStr.empty()) {
        valuesStr.pop_back();
    }
    updateValues.str(valuesStr);
}

template<typename... Args>
int LocalDB::remove(const char* tableName, Args&&... args) {
    std::stringstream condition;
    constructDeleteCondition(condition, std::forward<Args>(args)...);

    std::string sql = "DELETE FROM " + std::string(tableName);
    if (!condition.str().empty()) {
        sql += " WHERE " + condition.str();
    }

    return executeSQL(sql.c_str(), nullptr);
}

template<typename... Args>
void LocalDB::constructDeleteCondition(std::stringstream& condition, Args&&... args) {
    if constexpr (sizeof...(args) > 0) {
        condition << "1=1 AND ";
        ((condition << args.first << "='" << args.second << "' AND "), ...);
    }
    // Remove the trailing "AND"
    std::string conditionStr = condition.str();
    if (!conditionStr.empty()) {
        conditionStr = conditionStr.substr(0, conditionStr.size() - 5);
    }
    condition.str(conditionStr);
}

int LocalDB::remove(const char* tableName) {
    std::string sql = "DROP TABLE IF EXISTS " + std::string(tableName);
    return executeSQL(sql.c_str());
}

void LocalDB::printLastError() {
    if (db) {
        const char* errorMsg = sqlite3_errmsg(db);
        ESP_LOGE(TAG, "Last error: %s", errorMsg ? errorMsg : "Unknown error");
    } else {
        ESP_LOGE(TAG, "No database connection.");
    }
}

void testDatabases() 
{
    static const char* filename = "/spiffs/test.db";
    LocalDB& db = LocalDB::getInstance(filename);
    db.init();

    unlink("/spiffs/test.db");

    if (!db.open()) {
        return;
    }

    // Create SensorData table
    db.createTable("SensorData", "id INTEGER PRIMARY KEY NOT NULL, "
                                    "engine_id INTEGER NOT NULL, "
                                    "timestamp_unix INTEGER NOT NULL, "
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
                                        "engine_id INTEGER NOT NULL, "  
                                        "total_fuel_burned INTEGER NOT NULL, "
                                        "remaining_fuel INTEGER NOT NULL, "
                                        "distance_to_empty INTEGER NOT NULL");

    // Create Engines table
    db.createTable("Engines", "id INTEGER PRIMARY KEY NOT NULL,"
                                "name TEXT");

    db.createTable("UserCredentials", "device_id INTEGER PRIMARY KEY,"
                                    "username TEXT NOT NULL,"
                                    "password TEXT NOT NULL");

    // Insert data into tables using insert method
    std::map<std::string, std::string> sensorData = {
        {"engine_id", "1003"},
        {"timestamp_unix", "1646659900"},
        {"fuel_supply", "10.5"},
        {"fuel_return", "8.3"},
        {"temperature_supply", "25.6"},
        {"temperature_return", "30.2"}
    };
    db.insert("SensorData", sensorData);

    std::map<std::string, std::string> configurations = {
        {"engine_id", "1002"},
        {"time_interval", "60"},
        {"status", "1"}
    };
    db.insert("Configurations", configurations);

    std::map<std::string, std::string> fuelManagement = {
        {"engine_id", "1001"},
        {"total_fuel_burned", "100"},
        {"remaining_fuel", "50"},
        {"distance_to_empty", "200"}
    };
    db.insert("FuelManagement", fuelManagement);

    // Insert data into Engines table
    std::map<std::string, std::string> enginesData = {
        {"name", "Engine M1"},
    };
    db.insert("Engines", enginesData);

    // db.remove("SensorData");

    // Search for data in SensorData table where engine_id is '1003'
    auto searchResults = db.search("SensorData", {"*"}, std::make_pair("engine_id", "1003"));

    std::map<std::string, std::string> updateValues = {{"engine_id", "1005"}, {"sensor_data_id", "4567"}};
    db.update("SensorData", updateValues, std::make_pair("engine_id", "1004"));

    // auto searchResults = db.search("SensorData", {"*"}, std::make_pair("engine_id", "1005"));

    // // Display the search results
    for (const auto& result : searchResults) {
        for (const auto& pair : result) {
            printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
        }
        printf("\n");
    }

    // db.remove("SensorData", std::make_pair("engine_id", "1005"));
    // auto results = db.search("SensorData", {"*"}, std::make_pair("engine_id", "1005"));

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

int printCallback(void *data, int argc, char **argv, char **azColName) {
    int i;
    printf("%s: \n", (const char*)data);
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}