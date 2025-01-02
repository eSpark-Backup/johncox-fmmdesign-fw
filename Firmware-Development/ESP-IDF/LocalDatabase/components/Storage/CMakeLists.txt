idf_component_register(SRCS "Storage.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_littlefs
                             fatfs
                             sdmmc  
                             spiffs
                             )

