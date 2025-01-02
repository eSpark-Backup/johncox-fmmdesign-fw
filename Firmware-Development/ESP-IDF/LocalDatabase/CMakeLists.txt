cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

set(EXTRA_COMPONENT_DIRS libraries)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "v1.0.6")
project(LocalDatabase)

# Specify the output folder for the binaries
set(CUSTOM_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Binaries")

# Define the paths to binary files
set(PROJECT_BIN "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin")
set(PROJECT_ELF "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf")
set(BOOTLOADER_BIN "${CMAKE_BINARY_DIR}/bootloader/bootloader.bin")
set(PARTITION_TABLE_BIN "${CMAKE_BINARY_DIR}/partition_table/partition-table.bin")

# Add a custom target to run the shell script
add_custom_target(copy_binaries ALL
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/post_build.sh
        "${CUSTOM_OUTPUT_DIR}" "${PROJECT_BIN}" "${PROJECT_ELF}" "${BOOTLOADER_BIN}" "${PARTITION_TABLE_BIN}"
    COMMENT "Running shell script to copy binary files..."
)

# Ensure the custom target depends on the actual build output
add_dependencies(copy_binaries app)

# Prevent errors if files don't exist during the first build
add_custom_command(
    OUTPUT "${PROJECT_BIN}" "${PROJECT_ELF}" "${BOOTLOADER_BIN}" "${PARTITION_TABLE_BIN}"
    COMMAND ${CMAKE_COMMAND} -E echo "Ensuring output files exist"
    COMMENT "Creating placeholders for missing binary files if needed"
)
