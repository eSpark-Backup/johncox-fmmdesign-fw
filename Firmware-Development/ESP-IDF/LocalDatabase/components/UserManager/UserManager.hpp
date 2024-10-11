#ifndef USER_CREDENTIALS_MANAGER_HPP
#define USER_CREDENTIALS_MANAGER_HPP

#include <string>
#include "esp_err.h"

class UserCredentialsManager {
public:
    static UserCredentialsManager& getInstance(); // Static method to access the singleton instance

    esp_err_t createUser(const std::string& username, const std::string& password);
    esp_err_t removeUser(const std::string& username);
    esp_err_t updatePassword(const std::string& username, const std::string& oldPassword, const std::string& newPassword);
    esp_err_t authenticateUser(const std::string& username, const std::string& password);

private:
    UserCredentialsManager(); // Private constructor to prevent instantiation
    UserCredentialsManager(const UserCredentialsManager&) = delete; // Delete copy constructor
    UserCredentialsManager& operator=(const UserCredentialsManager&) = delete; // Delete assignment operator
    ~UserCredentialsManager(); // Private destructor

    int getUserID(const std::string& username);

    // Singleton instance
    static UserCredentialsManager instance;
};

#endif // USER_CREDENTIALS_MANAGER_HPP

