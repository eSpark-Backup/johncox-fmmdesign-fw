idf_component_register(SRCS	"HttpServer.cpp"
						INCLUDE_DIRS "."
                        REQUIRES
                            nvs_flash
                            esp_http_client
                            esp_https_server
                            esp_netif
                            esp_wifi
                            spiffs
                            json
                            UserManager
                            LocalDB

                            EMBED_TXTFILES 
                            "favicon.ico"
                            "certs/cacert.pem"
                            "certs/prvtkey.pem"
						)