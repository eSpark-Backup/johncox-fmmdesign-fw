idf_component_register(SRCS	"HttpServer.c"
						INCLUDE_DIRS "."
                        REQUIRES
                            nvs_flash
                            esp_http_client
                            esp_https_server
                            esp_netif
                            esp_wifi
                            spiffs

                            EMBED_TXTFILES 
                            "favicon.ico"
                            "certs/cacert.pem"
                            "certs/prvtkey.pem"
						)