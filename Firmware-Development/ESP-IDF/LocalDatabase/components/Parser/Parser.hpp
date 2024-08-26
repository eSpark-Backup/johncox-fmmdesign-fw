#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <map>
#include <string>

namespace Parser 
{
    std::map<std::string, std::string> getQueryParameters(const char* queryString);
    std::map<std::string, std::string> parseCredentials(const char* body);
    bool hasKey(const std::map<std::string, std::string>& parameters, const std::string& key);
    char* convertMapToJSON(const std::map<std::string, std::vector<std::map<std::string, std::string>>>& jsonData,
                                 const std::map<std::string, std::string>& additionalParams = {});
    char* convertMapToJSON(const std::map<std::string, std::vector<std::map<std::string, std::string>>>& engineData,
                            const std::map<std::string, std::vector<std::map<std::string, std::string>>>& jsonData,
                            std::vector<std::string> enabledEngineIds,
                                const std::map<std::string, std::string>& additionalParams = {});
}

#endif /* PARSER_H */
