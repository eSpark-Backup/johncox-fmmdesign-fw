# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

set(EXTRA_COMPONENT_DIRS libraries)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "v1.0.6")
project(LocalDatabase)

# Specify the output folder for the binaries
set(CUSTOM_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Binaries")

# # If required to copy binaries in project parent folders
# # Get the parent directory of ${CMAKE_SOURCE_DIR}
# get_filename_component(PARENT_DIR "${CMAKE_SOURCE_DIR}" DIRECTORY)
# # Get the grandparent directory (two levels above ${CMAKE_SOURCE_DIR})
# get_filename_component(GRANDPARENT_DIR "${PARENT_DIR}" DIRECTORY)
# # Set the output directory to a folder named 'Binaries' in the grandparent directory
# set(CUSTOM_OUTPUT_DIR "${GRANDPARENT_DIR}/Binaries")


# Define the paths to binary files
set(PROJECT_BIN "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin")
set(PROJECT_ELF "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf")
set(BOOTLOADER_BIN "${CMAKE_BINARY_DIR}/bootloader/bootloader.bin")
set(PARTITION_TABLE_BIN "${CMAKE_BINARY_DIR}/partition_table/partition-table.bin")

# Add a custom target to run the shell script
add_custom_target(copy_binaries ALL
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/post_build.sh
    "${CUSTOM_OUTPUT_DIR}" "${PROJECT_BIN}" "${PROJECT_ELF}" "${BOOTLOADER_BIN}" "${PARTITION_TABLE_BIN}"
    DEPENDS "${PROJECT_BIN}" "${PROJECT_ELF}" "${BOOTLOADER_BIN}" "${PARTITION_TABLE_BIN}"
    COMMENT "Running shell script to copy binary files..."
)
