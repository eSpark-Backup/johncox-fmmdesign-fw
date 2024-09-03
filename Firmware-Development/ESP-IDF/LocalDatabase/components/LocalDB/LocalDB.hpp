#ifndef LOCAL_DB_H
#define LOCAL_DB_H

#include <map>
#include <vector>
#include <sqlite3.h>

class LocalDB 
{
public:
    static LocalDB& getInstance(const char *filename = "/spiffs/test");
    ~LocalDB();

    void init();
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

    template<typename... Args>
    void constructUpdateValues(std::stringstream& updateValues, const std::map<std::string, std::string>& values);
    
    std::string constructWhereCondition(const std::map<std::string, std::string>& where);

};

void testDatabases();

#endif /* LOCAL_DB_H */
