#include "Device.hpp"
#include <esp_wifi.h>

std::pair<std::string, std::string> Device::generateUsernamePassword() {
    // Get MAC address of ESP32
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    // Convert MAC address to string
    std::stringstream stream;
    stream << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        stream << std::setw(2) << static_cast<int>(mac[i]);
    }
    std::string macAddress = stream.str();

    // Generate username and password from MAC address
    std::string username = "ESP_" + macAddress.substr(0, 6); // Example: ESP_3C71BF
    std::string password = macAddress.substr(6); // Example: 3C71BF

    return std::make_pair(username, password);
}
