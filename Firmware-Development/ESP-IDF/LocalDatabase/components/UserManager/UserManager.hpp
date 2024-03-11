#ifndef USER_CREDENTIALS_MANAGER_HPP
#define USER_CREDENTIALS_MANAGER_HPP

#include <string>
#include "esp_err.h"

class UserCredentialsManager {
public:
    static esp_err_t createUser(const std::string& username, const std::string& password);
    static esp_err_t removeUser(const std::string& username);
    static esp_err_t updatePassword(const std::string& username, const std::string& newPassword);

private:
    static int getUserID(const std::string& username);
};

#endif // USER_CREDENTIALS_MANAGER_HPP
