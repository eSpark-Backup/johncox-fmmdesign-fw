#include <sstream>
#include <utility>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_spiffs.h"
#include "LocalDB.hpp"
#include <cstring>
#include "Storage.hpp"

static const char *TAG = "LocalDB";

static int searchCallback(void* data, int argc, char** argv, char** azColName);

LocalDB* LocalDB::instance = nullptr;

LocalDB::LocalDB(const char *filename)
: filename(filename), db(nullptr)
{
    mutex = xSemaphoreCreateMutex();
}

LocalDB::~LocalDB() 
{
    close();
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
    }
    mutex = NULL;
}

LocalDB& LocalDB::getInstance(const char *filename) 
{
    if (!instance) 
    {
        instance = new LocalDB(filename);
    }
    return *instance;
}

void LocalDB::init(const char *mountPath) 
{
    filename = concatenateFilePath(mountPath, filename);
    unlink(filename);
    sqlite3_initialize();
}

bool LocalDB::open()
{
    close();
    int64_t start = esp_timer_get_time();
    int rc = sqlite3_open(filename, &db);
    if (rc) 
    {
        ESP_LOGE(TAG, "Can't open database: %s", sqlite3_errmsg(db));
        return false;
    } 
    else 
    {
        ESP_LOGI(TAG, "Opened database successfully in %lldms", (esp_timer_get_time() - start) / 1000);
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
    // ESP_LOGW(TAG, "executeSQL called from %s", __func__);
    executeSQL(sql);
}

int LocalDB::executeSQL(const char *sql, int (*callback)(void*, int, char**, char**), void *callbackData) 
{
    char *zErrMsg = nullptr;
    int rc = SQLITE_PERM;
    // ESP_LOGI(TAG, "%s", sql);
    if(lock())
    {
        // int64_t start = esp_timer_get_time();

        rc = sqlite3_exec(db, sql, callback, callbackData, &zErrMsg);
        if (rc != SQLITE_OK) 
        {
            ESP_LOGE(TAG, "SQL error: %s", zErrMsg);
            Storage::storageStats();
            sqlite3_free(zErrMsg);
        } 
        else 
        {
            // ESP_LOGI(TAG, "Operation done successfully");
        }
        // ESP_LOGI(TAG, "Time taken: %lldms", (esp_timer_get_time() - start) / 1000);
        unlock();
    }
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

    // ESP_LOGW(TAG, "executeSQL called from %s and returning to %s", __func__, __builtin_return_address(0));
    // ESP_LOGW(TAG, "executeSQL called from %s", __func__);
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
    
    // ESP_LOGW(TAG, "executeSQL called from %s", __func__);
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

    // ESP_LOGW(TAG, "executeSQL called from %s", __func__);
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
        // ESP_LOGW(TAG, "executeSQL called from %s", __func__);
        return executeSQL(sql.c_str(), nullptr);
    }

    std::string condition = constructWhereCondition(where);

    sql = "DELETE FROM " + std::string(tableName);
    if (!condition.empty()) 
    {
        sql += condition;
    }
    
    // ESP_LOGW(TAG, "executeSQL called from %s", __func__);
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

static int searchCallback(void* data, int argc, char** argv, char** azColName) 
{
    std::map<std::string, std::string> row;
    for (int i = 0; i < argc; i++) 
    {
        row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        // ESP_LOGE(TAG, "%s: %s", azColName[i], argv[i]);
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

const char* concatenateFilePath(const char* base, const char* filename)
{
    if((filename == NULL))
        return "invalid FileName";
    if((base == NULL))
        return "invalid Base";

    int pathLen = strlen(base) + strlen(filename) + 1;
    char* path = (char *)malloc(pathLen);
    memset(path, 0, pathLen);
    snprintf(path, pathLen, "%s%s", base, filename);
    return path;
}
