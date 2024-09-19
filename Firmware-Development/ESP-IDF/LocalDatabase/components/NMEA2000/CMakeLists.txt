idf_component_register(SRCS "N2kMsg.cpp"  "N2kStream.cpp"  "N2kMessages.cpp"  "N2kTimer.cpp"  "Seasmart.cpp"  "N2kDeviceList.cpp"
                            "N2kGroupFunction.cpp"  "N2kGroupFunctionDefaultHandlers.cpp"  "N2kMaretron.cpp"  "NMEA2000.cpp"  "NMEA2000_esp32.cpp"
                        INCLUDE_DIRS "."  "includes"
                      )

# if(ESP_PLATFORM)
#   # using this library as a component in an ESP-IDF project
#   return() # ignore the rest of this file
# endif()

# # probably running unit tests with catch on a host
# add_library(nmea2000 ${srcs})

# target_include_directories(nmea2000
#   PUBLIC
#   ${CMAKE_CURRENT_SOURCE_DIR}
# )
