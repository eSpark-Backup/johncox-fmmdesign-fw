# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/.toolchains/esp/v4.4.6/esp-idf/components/bootloader/subproject"
  "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader"
  "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader-prefix"
  "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader-prefix/tmp"
  "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader-prefix/src/bootloader-stamp"
  "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader-prefix/src"
  "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/.Embedded/ESP/NMEA2000-esp32DevKit/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
