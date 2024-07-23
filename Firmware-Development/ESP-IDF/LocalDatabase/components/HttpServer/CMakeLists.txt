idf_component_register(SRCS	"HttpServer.cpp" "HttpRequestsHandlers.cpp"
						INCLUDE_DIRS "."
                        REQUIRES 
                            DeviceProvisioning
                            nvs_flash
                            esp_http_client
                            esp_https_server
                            esp_netif
                            esp_wifi
                            UserManager
                            LocalDB
                            Device
                            JCErrors
                            SessionManager
                            Websockets
                            json

                            EMBED_TXTFILES 
                            "favicon.ico"
                            "currentCerts/cacert.pem"
                            "currentCerts/prvtkey.pem"
						)