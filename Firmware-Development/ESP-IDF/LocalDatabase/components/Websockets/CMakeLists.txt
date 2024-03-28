idf_component_register(SRCS "KeepAlive.cpp" "Websockets.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_https_server

                                EMBED_TXTFILES "certs/cacertws.pem"
                                "certs/prvtkeyws.pem")
