#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <string>
#include <utility>
#include <sstream>
#include <iomanip>
#include <cstdint>

class Device 
{
    public:
        static std::pair<std::string, std::string> generateUsernamePassword();
};

#endif // DEVICE_HPP
