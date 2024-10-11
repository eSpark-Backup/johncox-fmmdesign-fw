idf_component_register(SRCS "LocalDB.cpp" 
                    INCLUDE_DIRS "."
                    REQUIRES esp32-idf-sqlite3
                             spiffs )
