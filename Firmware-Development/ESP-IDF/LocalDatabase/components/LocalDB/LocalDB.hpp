#ifndef LOCAL_DB_H
#define LOCAL_DB_H

#include <map>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <sqlite3.h>
#include "esp_log.h"

class LocalDB 
{
public:
    static LocalDB& getInstance(const char *filename = "/test");
    ~LocalDB();

    void init(const char *mountPath);
    bool open();
    void close();
    void createTable(const char *tableName, const char *columns);
    int executeSQL(const char *sql, int (*callback)(void*, int, char**, char**) = nullptr, void *callbackData = (void *)"TABLE");
    int insert(const char *tableName, std::map<std::string, std::string> data);
    int update(const char* tableName, const std::map<std::string, std::string>& update, const std::map<std::string, std::string>& where = {});
    int remove(const char* tableName, const std::map<std::string, std::string>& where);
    std::vector<std::map<std::string, std::string>> search(const char* tableName, std::vector<const char*> select, const std::map<std::string, std::string>& where, bool latest = false);

    void printLastError();
    
private:
    LocalDB(const char *filename); // Private constructor
    static LocalDB* instance; // Static instance pointer
    const char *filename;
    sqlite3 *db;
    SemaphoreHandle_t mutex = NULL;

    template<typename... Args>
    void constructUpdateValues(std::stringstream& updateValues, const std::map<std::string, std::string>& values);
    
    std::string constructWhereCondition(const std::map<std::string, std::string>& where);

    // Lock the mutex
    bool lock(TickType_t xTicksToWait = portMAX_DELAY) {
        BaseType_t err = pdFALSE;
        if(mutex)
            err = xSemaphoreTake(mutex, xTicksToWait);
        else
            return err;

        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        const char *taskName = pcTaskGetTaskName(currentTask);
        if(err == pdTRUE)
        {
            ESP_LOGD("MUTEX LOCK", "Mutex locked by task: %s.", taskName);
        }
        else
        {
            ESP_LOGD("MUTEX LOCK", "Task: %s failed to lock Mutex.", taskName);
        }
        return (err == pdTRUE);
    }

    // Unlock the mutex
    void unlock() {
        BaseType_t err = pdFALSE;
        if(mutex)
            err = xSemaphoreGive(mutex);
        else
            return;

        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        const char *taskName = pcTaskGetTaskName(currentTask);
        if(err == pdTRUE)
        {
            ESP_LOGD("MUTEX UNLOCK", "Mutex Unlocked by task: %s.", taskName);
        }
        else
        {
            ESP_LOGD("MUTEX UNLOCK", "Task: %s failed to unlock Mutex.", taskName);
        }
    }
};

// void testDatabases();
const char* concatenateFilePath(const char* base, const char* filename);

#endif /* LOCAL_DB_H */
