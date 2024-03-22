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
                            Device
                            CustomJWT
                            JCErrors
                            SessionManager

                            EMBED_TXTFILES 
                            "favicon.ico"
                            "currentCerts/cacert.pem"
                            "currentCerts/prvtkey.pem"
						)