#include <sstream>
#include <utility>
#include "esp_log.h"
#include "esp_timer.h"
#include "LocalDB.hpp"

static const char *TAG = "LocalDB";

static int searchCallback(void* data, int argc, char** argv, char** azColName);

LocalDB* LocalDB::instance = nullptr;

LocalDB::LocalDB(const char *filename) : filename(filename), db(nullptr) {}

LocalDB::~LocalDB() 
{
    close();
}

LocalDB& LocalDB::getInstance(const char *filename) 
{
    if (!instance) 
    {
        instance = new LocalDB(filename);
    }
    return *instance;
}

void LocalDB::init() 
{
    sqlite3_initialize();
}

bool LocalDB::open() 
{
    int rc = sqlite3_open(filename, &db);
    if (rc) 
    {
        ESP_LOGE(TAG, "Can't open database: %s", sqlite3_errmsg(db));
        return false;
    } 
    else 
    {
        ESP_LOGI(TAG, "Opened database successfully");
        return true;
    }
}

void LocalDB::close() 
{
    if (db) 
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

void LocalDB::createTable(const char *tableName, const char *columns) 
{
    char sql[512];
    snprintf(sql, sizeof(sql), "CREATE TABLE %s(%s);", tableName, columns);
    executeSQL(sql);
}

int LocalDB::executeSQL(const char *sql, int (*callback)(void*, int, char**, char**), void *callbackData) 
{
    char *zErrMsg = nullptr;
    ESP_LOGI(TAG, "%s", sql);
    int64_t start = esp_timer_get_time();
    int rc = sqlite3_exec(db, sql, callback, callbackData, &zErrMsg);
    if (rc != SQLITE_OK) 
    {
        ESP_LOGE(TAG, "SQL error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    else 
    {
        ESP_LOGI(TAG, "Operation done successfully");
    }
    ESP_LOGI(TAG, "Time taken: %lldms", (esp_timer_get_time() - start) / 1000);
    return rc;
}

int LocalDB::insert(const char *tableName, std::map<std::string, std::string> data) 
{
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

std::string LocalDB::constructWhereCondition(const std::map<std::string, std::string>& where) 
{
    std::ostringstream condition;
    if (!where.empty()) 
    {
        condition << " WHERE ";
        size_t entryCount = 0;
        for (const auto& entry : where) 
        {
            if (entryCount != 0) 
            {
                condition << " AND ";
            }
            condition << entry.first << "='" << entry.second << "'";
            ++entryCount;
        }
    }
    return condition.str();
}

std::vector<std::map<std::string, std::string>> LocalDB::search(const char* tableName, std::vector<const char*> select, const std::map<std::string, std::string>& where, bool latest) 
{
    std::vector<std::map<std::string, std::string>> results;

    std::string condition = constructWhereCondition(where);

    std::ostringstream selectStr;
    for (size_t i = 0; i < select.size(); ++i) 
    {
        if (i != 0) 
        {
            selectStr << ", ";
        }
        selectStr << select[i];
    }

    std::string sql = "SELECT " + selectStr.str() + " FROM " + std::string(tableName);
    if (!condition.empty()) 
    {
        sql += condition;
    }
    if (latest)
    {
        sql += " ORDER BY id DESC LIMIT 1"; // Assuming 'id' is the primary key
    }
    

    executeSQL(sql.c_str(), searchCallback, &results);

    return results;
}

int LocalDB::update(const char* tableName, const std::map<std::string, std::string>& update, const std::map<std::string, std::string>& where) 
{
    std::stringstream updateValues;
    constructUpdateValues(updateValues, update);

    std::string condition = constructWhereCondition(where);

    std::string sql = "UPDATE " + std::string(tableName) + " SET " + updateValues.str();
    if (!condition.empty()) 
    {
        sql += condition;
    }

    return executeSQL(sql.c_str(), nullptr);
}

template<typename... Args>
void LocalDB::constructUpdateValues(std::stringstream& updateValues, const std::map<std::string, std::string>& values) 
{
    for (const auto& pair : values) 
    {
        updateValues << pair.first << "='" << pair.second << "',";
    }
    // Remove the trailing comma
    std::string valuesStr = updateValues.str();
    if (!valuesStr.empty()) 
    {
        valuesStr.pop_back();
    }
    updateValues.str(valuesStr);
}

int LocalDB::remove(const char* tableName, const std::map<std::string, std::string>& where) 
{
    std::string sql;
    if (where.empty())
    {
        sql = "DROP TABLE IF EXISTS " + std::string(tableName);
        return executeSQL(sql.c_str(), nullptr);
    }

    std::string condition = constructWhereCondition(where);

    sql = "DELETE FROM " + std::string(tableName);
    if (!condition.empty()) 
    {
        sql += condition;
    }
    
    return executeSQL(sql.c_str(), nullptr);
}

void LocalDB::printLastError() 
{
    if (db) 
    {
        const char* errorMsg = sqlite3_errmsg(db);
        ESP_LOGE(TAG, "Last error: %s", errorMsg ? errorMsg : "Unknown error");
    } 
    else 
    {
        ESP_LOGE(TAG, "No database connection.");
    }
}

void testDatabases() 
{
    static const char* filename = "/spiffs/test.db";
    LocalDB& db = LocalDB::getInstance(filename);
    db.init();

    // unlink("/spiffs/test.db");

    if (!db.open()) 
    {
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

    db.createTable("UserCredentials", "id INTEGER PRIMARY KEY,"
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

static int searchCallback(void* data, int argc, char** argv, char** azColName) 
{
    std::map<std::string, std::string> row;
    for (int i = 0; i < argc; i++) 
    {
        row[azColName[i]] = argv[i] ? argv[i] : "NULL";
    }
    static_cast<std::vector<std::map<std::string, std::string>>*>(data)->push_back(row);
    return 0;
}

int printCallback(void *data, int argc, char **argv, char **azColName) 
{
    int i;
    printf("%s: \n", (const char*)data);
    for (i = 0; i < argc; i++) 
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}