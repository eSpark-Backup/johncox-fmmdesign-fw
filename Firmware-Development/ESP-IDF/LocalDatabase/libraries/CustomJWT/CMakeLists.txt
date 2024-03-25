idf_component_register(SRCS "src/Base64URL.cpp"
                            "src/hmac_sha256.c"
                            "src/sha256.c"
                    INCLUDE_DIRS "src"
                    REQUIRES LocalDB esp32-idf-sqlite3)
