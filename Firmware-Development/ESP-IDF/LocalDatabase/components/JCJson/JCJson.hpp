#pragma once
#include <string>
#include <map>
#include <variant>

class JCJson {
public:
    JCJson() = delete; // Disable constructor

    // Method to create JSON from key-value pairs and return as char*
    static char* create(const std::map<std::string, std::variant<std::string, int>>& keyValues);

    // Method to parse JSON and extract key-value pairs
    static std::map<std::string, std::variant<std::string, int>> parse(const char* json);
};
