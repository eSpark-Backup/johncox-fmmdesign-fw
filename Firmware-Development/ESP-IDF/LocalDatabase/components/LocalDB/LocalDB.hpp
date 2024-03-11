#ifndef LOCAL_DB_H
#define LOCAL_DB_H

#include <map>
#include <vector>
#include <sqlite3.h>

class LocalDB 
{
public:
    static LocalDB& getInstance(const char *filename = "/spiffs/test.db");
    ~LocalDB();

    void init();
    bool open();
    void close();
    void createTable(const char *tableName, const char *columns);
    int executeSQL(const char *sql, int (*callback)(void*, int, char**, char**) = nullptr, void *callbackData = (void *)"TABLE");
    int insert(const char *tableName, std::map<std::string, std::string> data);

    template<typename... Args>
    std::vector<std::map<std::string, std::string>> search(const char* tableName, std::initializer_list<const char*> select, Args&&... where);

    template<typename... Args>
    int update(const char* tableName, const std::map<std::string, std::string>& update, Args&&... where);

    template<typename... Args>
    int remove(const char* tableName, Args&&... args);
    int remove(const char* tableName);
    void printLastError();

private:
    LocalDB(const char *filename); // Private constructor
    static LocalDB* instance; // Static instance pointer
    const char *filename;
    sqlite3 *db;

    // Other methods...
    // void constructCondition(std::stringstream& condition){}
    void constructCondition(std::stringstream& condition, const std::pair<const char*, std::string>& pair);

    template<typename... Args>
    void constructCondition(std::stringstream& condition, const std::pair<const char*, std::string>& pair, Args&&... args);

    template<typename... Args>
    void constructUpdateValues(std::stringstream& updateValues, const std::map<std::string, std::string>& values);

    template<typename... Args>
    void constructDeleteCondition(std::stringstream& condition, Args&&... args);

};

void testDatabases();

#endif /* LOCAL_DB_H */
