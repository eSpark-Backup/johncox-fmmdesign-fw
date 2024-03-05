#ifndef LOCAL_DB_H
#define LOCAL_DB_H

#include <map>
#include <vector>
#include "sqlite3.h"

class LocalDB 
{
    public:
        LocalDB(const char *filename);
        ~LocalDB();

        void init();
        bool open();
        void close();
        void createTable(const char *tableName, const char *columns);
        int executeSQL(const char *sql, int (*callback)(void*, int, char**, char**) = nullptr, void *callbackData = (void *)"TABLE");
        void insert(const char *tableName, std::map<std::string, std::string> data);
        template<typename... Args>
        std::vector<std::map<std::string, std::string>> search(const char* tableName, Args&&... args);

    private:
        const char *filename;
        sqlite3 *db;

        // Other methods...
        template<typename T>
        void constructCondition(std::stringstream& condition, T&& key, T&& value);
        template<typename T, typename... Args>
        void constructCondition(std::stringstream& condition, T&& key, T&& value, Args&&... args);
};

void testDatabases();

#endif /* LOCAL_DB_H */
