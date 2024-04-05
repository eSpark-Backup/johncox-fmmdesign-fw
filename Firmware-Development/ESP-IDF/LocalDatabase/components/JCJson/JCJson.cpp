#include "JCJson.hpp"
#include <cJSON.h>

char* JCJson::create(const std::map<std::string, std::variant<std::string, int>>& keyValues) {
    cJSON *root = cJSON_CreateObject();

    // Add key-value pairs to the JSON object
    for (const auto& [key, value] : keyValues) {
        if (std::holds_alternative<std::string>(value)) {
            cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateString(std::get<std::string>(value).c_str()));
        } else if (std::holds_alternative<int>(value)) {
            cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateNumber(std::get<int>(value)));
        }
    }

    // Serialize the JSON object to a string
    char* serialized_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return serialized_json;
}

std::map<std::string, std::variant<std::string, int>> JCJson::parse(const char* json) {
    std::map<std::string, std::variant<std::string, int>> result;

    cJSON *root = cJSON_Parse(json);
    if (root) {
        cJSON *item = root->child;
        while (item) {
            if (item->type == cJSON_String) {
                result[item->string] = std::string(item->valuestring);
            } else if (item->type == cJSON_Number) {
                result[item->string] = item->valueint;
            }
            item = item->next;
        }
        cJSON_Delete(root);
    }

    return result;
}