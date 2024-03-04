#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "sqlite3.h"

static const char *TAG = "sqlite3_spiffs";
typedef int (*sqlite_callback)(void*, int, char**, char**);

void initialize_spiffs() {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

static int print_callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int execute_sql(sqlite3 *db, const char *sql, sqlite_callback callback) {
    char *zErrMsg = 0;
    printf("%s\n", sql);
    int64_t start = esp_timer_get_time();
    int rc = sqlite3_exec(db, sql, callback, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        printf("Operation done successfully\n");
    }
    printf("Time taken: %lld\n", esp_timer_get_time()-start);
    return rc;
}

void create_table(sqlite3 *db, const char *table_name, const char *columns) {
    char sql[512];
    snprintf(sql, sizeof(sql), "CREATE TABLE %s(%s);", table_name, columns);
    execute_sql(db, sql, NULL);
}

int open_database(const char *filename, sqlite3 **db) {
    int rc = sqlite3_open(filename, db);
    if (rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    } else {
        printf("Opened database successfully\n");
    }
    return rc;
}

void close_database(sqlite3 *db) {
    sqlite3_close(db);
}

void test_databases() {
    sqlite3 *db;
    int rc;

    sqlite3_initialize();

    // remove existing file
    unlink("/spiffs/test.db");
    
    // Open the database
    if (open_database("/spiffs/test.db", &db)) {
        return;
    }

        // Create SensorData table
    create_table(db, "SensorData", "id INTEGER PRIMARY KEY NOT NULL, "
                                    "engine_id INTEGER NOT NULL, "
                                    "sensor_data_id INTEGER NOT NULL, "
                                    "fuel_management_data_id INTEGER NOT NULL, "
                                    "timestamp_unix INTEGER NOT NULL");

    // Create SensorsDataBlock table
    create_table(db, "SensorsDataBlock", "id INTEGER PRIMARY KEY NOT NULL, "
                                         "fuel_supply FLOAT NOT NULL, "
                                         "fuel_return FLOAT NOT NULL, "
                                         "temperature_supply FLOAT NOT NULL, "
                                         "temperature_return FLOAT NOT NULL");

    // Create Configurations table
    create_table(db, "Configurations", "engine_id INTEGER NOT NULL, "
                                       "time_interval INTEGER NOT NULL, "
                                       "status BOOLEAN NOT NULL");

    // Create FuelManagement table
    create_table(db, "FuelManagement", "id INTEGER PRIMARY KEY NOT NULL, "
                                       "total_fuel_burned INTEGER NOT NULL, "
                                       "remaining_fuel INTEGER NOT NULL, "
                                       "distance_to_empty INTEGER NOT NULL");

    // Create Engines table
    create_table(db, "Engines", "id INTEGER PRIMARY KEY NOT NULL");

    // Insert data into tables
    execute_sql(db, "INSERT INTO SensorData (id, engine_id, sensor_data_id, fuel_management_data_id, timestamp_unix) VALUES (1, 1001, 2001, 3001, 1646659900)", print_callback);
    execute_sql(db, "INSERT INTO SensorsDataBlock (id, fuel_supply, fuel_return, temperature_supply, temperature_return) VALUES (1, 10.5, 8.3, 25.6, 30.2)", print_callback);
    execute_sql(db, "INSERT INTO Configurations (engine_id, time_interval, status) VALUES (1001, 60, 1)", print_callback);
    execute_sql(db, "INSERT INTO FuelManagement (id, total_fuel_burned, remaining_fuel, distance_to_empty) VALUES (1, 100, 50, 200)", print_callback);
    execute_sql(db, "INSERT INTO Engines (id) VALUES (1001)", print_callback);

    // Read data from tables
    execute_sql(db, "SELECT * FROM SensorData", print_callback);
    execute_sql(db, "SELECT * FROM SensorsDataBlock", print_callback);
    execute_sql(db, "SELECT * FROM Configurations", print_callback);
    execute_sql(db, "SELECT * FROM FuelManagement", print_callback);
    execute_sql(db, "SELECT * FROM Engines", print_callback);

    // Close the database
    close_database(db);
}

void app_main() {
    initialize_spiffs();
    test_databases();
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}
