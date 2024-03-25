#include "Device.hpp"
#include <esp_wifi.h>
#include <esp_log.h>

static const char* TAG = "Device";

std::string Device::username;
std::string Device::password;

std::pair<std::string, std::string> Device::generateCredentials() 
{
    // Get MAC address of ESP32
    uint8_t mac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if(ret != ESP_OK)
    {
        return std::make_pair("", "");
    }

    // Convert MAC address to string
    std::stringstream stream;
    stream << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        stream << std::setw(2) << static_cast<int>(mac[i]);
    }
    std::string macAddress = stream.str();

    ESP_LOGE(TAG, "%s", macAddress.c_str());
    // Generate username and password from MAC address
    username = "ESP_" + macAddress.substr(0, 6);
    password = macAddress.substr(6);

    return std::make_pair(username, password);
}

std::pair<std::string, std::string> Device::getCredentials()
{
    if(!username.empty() && !password.empty())
    {
        return std::make_pair(username, password);
    }

    return generateCredentials();
}
