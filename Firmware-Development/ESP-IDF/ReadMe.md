## ESP-IDF Projects

# Flashing Binaries to ESP32

## Required Software
- [ESP32 Flash Download Tool](https://www.espressif.com/en/support/download/other-tools)
- [Termite](https://termite.software.informer.com/3.4/)

## Testing Procedure
1. Connect the ESP32 Dev Module to the PC via MicroUSB cable.
2. Open the ESP32 Flash Download Tool and configure options as indicated below:
   - ![image](https://github.com/eSpark-Consultants/johncox-fmmdesign-fw/assets/106241042/2c6a9e3f-96d3-44c4-bc56-f84080c71e19)

   - Click OK to proceed.
3. Upload the provided binaries (`Speaker_binaries` or `Button_binaries`) following these steps:
   - `app.bootloader.bin` -> 0x1000
   - `app.partitions.bin` -> 0x8000
   - `app.bin` -> 0x10000
4. Select the appropriate COM port and baud rate (115200) for ESP32 connection.
5. Press the start button to initiate the flashing process as depicted in the screenshot below:
   - ![image](https://github.com/eSpark-Consultants/johncox-fmmdesign-fw/assets/106241042/4f2e196c-e57b-4286-8298-ee749b916379)

6. Once the flashing process is complete, close the software and reset the ESP32 using the EN button on the Dev Module.
7. Open a terminal emulator (e.g., Termite) and connect to the ESP32 serial port at a baud rate of 115200 to observe the console log.

## Erasing Stored WiFi Credentials
Click the erase button on the new window that opens to completely wipe off the ESP32 memory.
