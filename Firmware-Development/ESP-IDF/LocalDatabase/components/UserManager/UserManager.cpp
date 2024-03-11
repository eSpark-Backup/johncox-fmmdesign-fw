#include "UserManager.hpp"
#include "LocalDB.hpp" // Assuming you have a LocalDB class for interacting with the database
#include <iostream>

esp_err_t UserCredentialsManager::createUser(const std::string& username, const std::string& password) 
{
    // Check if a user with the given username already exists
    if (getUserID(username) != -1) {
        std::cout << "User '" << username << "' already exists." << std::endl;
        return ESP_FAIL;
    }

    // Insert the new user into the UserCredentials table
    LocalDB& db = LocalDB::getInstance();
    std::map<std::string, std::string> userData = {
        {"device_id", "1"}, // Assuming only one device
        {"username", username},
        {"password", password}
    };
    db.insert("UserCredentials", userData);
    std::cout << "User '" << username << "' created successfully." << std::endl;
    return ESP_OK;
}

esp_err_t UserCredentialsManager::removeUser(const std::string& username) 
{
    // Delete the user from the UserCredentials table
    LocalDB& db = LocalDB::getInstance();
    int userID = getUserID(username);
    if (userID == -1) {
        std::cout << "User '" << username << "' not found." << std::endl;
        return ESP_FAIL;
    }
    db.remove("UserCredentials", std::make_pair("device_id", "1"), std::make_pair("username", username));
    std::cout << "User '" << username << "' removed successfully." << std::endl;
    return ESP_OK;
}

esp_err_t UserCredentialsManager::updatePassword(const std::string& username, const std::string& newPassword) 
{
    // Update the password for the given user
    LocalDB& db = LocalDB::getInstance();
    int userID = getUserID(username);
    if (userID == -1) {
        std::cout << "User '" << username << "' not found." << std::endl;
        return ESP_FAIL;
    }
    std::map<std::string, std::string> updateValues = {
        {"password", newPassword}
    };
    db.update("UserCredentials", updateValues, std::make_pair("device_id", "1"), std::make_pair("username", username));
    std::cout << "Password updated successfully for user '" << username << "'." << std::endl;
    return ESP_OK;
}

int UserCredentialsManager::getUserID(const std::string& username) 
{
    // Get the user ID for the given username
    LocalDB& db = LocalDB::getInstance();
    auto searchResults = db.search("UserCredentials", {"device_id"}, std::make_pair("device_id", "1"), std::make_pair("username", username));
    if (searchResults.empty()) {
        return -1; // User not found
    }
    return std::stoi(searchResults[0]["device_id"]);
}