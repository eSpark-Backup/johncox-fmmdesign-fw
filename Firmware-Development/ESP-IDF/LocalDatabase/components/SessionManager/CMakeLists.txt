idf_component_register(SRCS "SessionManager.cpp" 
                    INCLUDE_DIRS "."
                    REQUIRES CustomJWT
                            json
                            Device)
