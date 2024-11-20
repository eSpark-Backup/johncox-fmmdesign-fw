#include "Parser.hpp"
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <esp_log.h>
#include "cJSON.h" 

const char* TAG = "Parser";

namespace Parser {

    std::map<std::string, std::string> getQueryParameters(const char* queryString) 
    {
        std::map<std::string, std::string> parameters;

        char* token;
        char* queryCopy = strdup(queryString);  // Create a copy of queryString to avoid modifying the original

        token = strtok(queryCopy, "&");
        while (token != nullptr) 
        {
            char* separator = strchr(token, '=');
            if (separator != nullptr) 
            {
                *separator = '\0';  // Split the token into parameter and value
                std::string key(token);
                std::string value(separator + 1);
                parameters[key] = value;
            }
            token = strtok(nullptr, "&");
        }

        free(queryCopy);  // Free the allocated memory for the copy
        return parameters;
    }

    // Check if a specific key exists in the map
    bool hasKey(const std::map<std::string, std::string>& parameters, const std::string& key) 
    {
        return parameters.find(key) != parameters.end();
    }

    // Function to parse JSON body and extract key-value pairs
    std::map<std::string, std::string> parseCredentials(const char* body) 
    {
        std::map<std::string, std::string> params;

        // Parse JSON
        cJSON *root = cJSON_Parse(body);
        if (root == NULL) 
        {
            ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
            return params;
        }

        // Get username
        cJSON *usernameJson = cJSON_GetObjectItemCaseSensitive(root, "username");
        if (cJSON_IsString(usernameJson) && (usernameJson->valuestring != NULL)) 
        {
            params["username"] = usernameJson->valuestring;
        }

        // Get password
        cJSON *passwordJson = cJSON_GetObjectItemCaseSensitive(root, "password");
        if (cJSON_IsString(passwordJson) && (passwordJson->valuestring != NULL)) 
        {
            params["password"] = passwordJson->valuestring;
        }

        cJSON_Delete(root);
        return params;
    }

    char* convertMapToJSON(const std::map<std::string, std::vector<std::map<std::string, std::string>>>& jsonData,
                                 const std::map<std::string, std::string>& additionalParams)
    {
        cJSON *root = cJSON_CreateObject(); // Create JSON root object

        // Add additional parameters to the root object if provided
        for (const auto& [key, value] : additionalParams) 
        {
            cJSON_AddStringToObject(root, key.c_str(), value.c_str());
        }
        
        cJSON *dataObject = cJSON_CreateObject(); // Create JSON object for data

        // Iterate over each key-value pair in the map
        for (auto const& [tableName, data] : jsonData) 
        {
            cJSON *dataArray = cJSON_CreateArray(); // Create JSON array for table data

            // Iterate over each data item in the table
            for (const auto& item : data) 
            {
                cJSON *itemObject = cJSON_CreateObject(); // Create JSON object for each data item

                // Add key-value pairs to the item object
                for (const auto& [key, value] : item) 
                {
                    cJSON_AddStringToObject(itemObject, key.c_str(), value.c_str());
                }

                // Add the item object to the data array
                cJSON_AddItemToArray(dataArray, itemObject);
            }

            // Add the data array to the data object with the table name as the key
            cJSON_AddItemToObject(dataObject, tableName.c_str(), dataArray);
        }

        // Add the data object to the root object with the "data" key
        cJSON_AddItemToObject(root, "data", dataObject);

        // Convert cJSON root object to JSON string
        char *jsonString = cJSON_Print(root);

        // Clean up cJSON resources
        cJSON_Delete(root);

        // Return JSON string
        return jsonString;
    }

    char* convertMapToJSON(const std::map<std::string, std::vector<std::map<std::string, std::string>>>& engineData,
                            const std::map<std::string, std::vector<std::map<std::string, std::string>>>& jsonData,
                            std::vector<std::string> enabledEngineIds, 
                            const std::map<std::string, std::string>& additionalParams)
    {
        cJSON *root = cJSON_CreateObject(); // Create JSON root object

        // Add additional parameters to the root object if provided
        for (const auto& [key, value] : additionalParams) 
        {
            cJSON_AddStringToObject(root, key.c_str(), value.c_str());
        }
        
        cJSON *dataObject = cJSON_CreateObject(); // Create JSON object for data

        // Iterate over each key-value pair in the map
        for (auto const& [tableName, data] : engineData) 
        {
            cJSON *dataArray = cJSON_CreateArray(); // Create JSON array for table data

            // Iterate over each data item in the table
            int counter = 0;
            for (const auto& item : data) 
            {
                cJSON *itemObject = cJSON_CreateObject(); // Create JSON object for each data item
                
                // Add key-value pairs to the item object
                for (const auto& [key, value] : item) 
                {
                    cJSON_AddStringToObject(itemObject, key.c_str(), value.c_str());
                }

                if (enabledEngineIds.size() < 2)
                {
                    for (auto const& [tableNameNested, dataNested] : jsonData) 
                    {
                        cJSON *itemObjectNested = cJSON_CreateObject(); // Create JSON object for each data item
                        int enableNumber = std::stoi(enabledEngineIds[0]);
                        if(enableNumber == (counter+1))
                        {
                            // Add key-value pairs to the item object
                            for (const auto& [key, value] : dataNested[0]) 
                            {
                                cJSON_AddStringToObject(itemObjectNested, key.c_str(), value.c_str());
                            }
                        }
                        cJSON_AddItemToObject(itemObject, tableNameNested.c_str(), itemObjectNested);
                    } 
                }
                else 
                {
                    for (auto const& [tableNameNested, dataNested] : jsonData) 
                    {
                        cJSON *itemObjectNested = cJSON_CreateObject(); // Create JSON object for each data item
                        // Add key-value pairs to the item object
                        for (const auto& [key, value] : dataNested[counter]) 
                        {
                            cJSON_AddStringToObject(itemObjectNested, key.c_str(), value.c_str());
                        }
                        // ESP_LOGE("", "Counter %d -------------", counter);
                        cJSON_AddItemToObject(itemObject, tableNameNested.c_str(), itemObjectNested);
                    }
                }

                // Add the item object to the data array
                cJSON_AddItemToArray(dataArray, itemObject);
                counter++;
            }
            

            // Add the data array to the data object with the table name as the key
            cJSON_AddItemToObject(dataObject, tableName.c_str(), dataArray);
        }

        // Add the data object to the root object with the "data" key
        cJSON_AddItemToObject(root, "data", dataObject);

        // Convert cJSON root object to JSON string
        char *jsonString = cJSON_Print(root);

        // Clean up cJSON resources
        cJSON_Delete(root);

        // Return JSON string
        return jsonString;
    }

    char* convertMapToJSON(const std::vector<engine_t>& jsonData,
                            const std::map<std::string, std::string>& additionalParams)
    {
        cJSON *root = cJSON_CreateObject(); // Create JSON root object

        cJSON *tanksArray = cJSON_CreateArray();
        // Add additional parameters to the root object if provided
        for (const auto& [key, value] : additionalParams)
        {
            if(key.find("tank") != std::string::npos)
            {
                cJSON_AddItemToArray(tanksArray, cJSON_CreateNumber(value.c_str()));
            }
            else
                cJSON_AddStringToObject(root, key.c_str(), value.c_str());
        }
        cJSON_AddItemToObject(root, "tanks", tanksArray);
        
        cJSON *dataArray = cJSON_CreateArray();
        for (auto const& data : jsonData)
        {
            cJSON *itemObject = cJSON_CreateObject();
            // Iterate over each data item in engineData
            for (const auto& [key, value] : data.engineData)
            {
                    cJSON_AddStringToObject(itemObject, key.c_str(), value.c_str());
            }
            // Iterate over each data item in tablesData
            for (const auto& [tableName, tableValues] : data.tablesData)
            {
                cJSON *tableObject = cJSON_CreateObject(); // Create JSON object for each table item
                for (const auto& [key, value] : tableValues)
                {
                    cJSON_AddStringToObject(tableObject, key.c_str(), value.c_str());
                }
                cJSON_AddItemToObject(itemObject, tableName.c_str(), tableObject);
            }
            cJSON_AddItemToArray(dataArray, itemObject);
        }
        cJSON_AddItemToObject(root, "Engines", dataArray);

        // Convert cJSON root object to JSON string
        char *jsonString = cJSON_Print(root);

        // Clean up cJSON resources
        cJSON_Delete(root);

        return jsonString;
    }

}