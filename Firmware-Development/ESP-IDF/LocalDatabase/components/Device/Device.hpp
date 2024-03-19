#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <string>
#include <utility>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <esp_err.h>

class Device 
{
    private:
        static std::string username;
        static std::string password;
        static std::pair<std::string, std::string> generateCredentials();
        
    public:
        static std::pair<std::string, std::string> getCredentials();

};

#endif // DEVICE_HPP
