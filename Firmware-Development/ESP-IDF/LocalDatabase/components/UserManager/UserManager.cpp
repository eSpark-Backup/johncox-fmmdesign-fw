#include "UserManager.hpp"
#include "LocalDB.hpp" // Assuming you have a LocalDB class for interacting with the database
#include <iostream>

UserCredentialsManager UserCredentialsManager::instance; // Define the static member

UserCredentialsManager::UserCredentialsManager() {
    // Constructor implementation
}

UserCredentialsManager::~UserCredentialsManager() {
    // Destructor implementation
}

UserCredentialsManager& UserCredentialsManager::getInstance() {
    return instance;
}

esp_err_t UserCredentialsManager::createUser(const std::string& username, const std::string& password) 
{
    // Check if a user with the given username already exists
    if (getUserID(username) != -1) 
    {
        std::cout << "User '" << username << "' already exists." << std::endl;
        return ESP_FAIL;
    }

    // Insert the new user into the UserCredentials table
    LocalDB& db = LocalDB::getInstance();
    std::map<std::string, std::string> userData = 
    {
        {"id", "1"}, // Assuming only one device
        {"username", username},
        {"password", password}
    };

    if (SQLITE_OK != db.insert("UserCredentials", userData))
    {
        return ESP_FAIL;
    }

    std::cout << "User '" << username << "' created successfully." << std::endl;
    return ESP_OK;
}

esp_err_t UserCredentialsManager::removeUser(const std::string& username) 
{
    // Delete the user from the UserCredentials table
    LocalDB& db = LocalDB::getInstance();
    int userID = getUserID(username);
    if (userID == -1) 
    {
        std::cout << "User '" << username << "' not found." << std::endl;
        return ESP_FAIL;
    }

    std::map<std::string, std::string> filter = 
    {
        {"id", "1"},
        {"username", username.c_str()},
    };
    db.remove("UserCredentials", filter);
    std::cout << "User '" << username << "' removed successfully." << std::endl;
    return ESP_OK;
}

esp_err_t UserCredentialsManager::updatePassword(const std::string& username, const std::string& oldPassword, const std::string& newPassword) {
    // Authenticate the user
    if (ESP_OK != authenticateUser(username, oldPassword)) 
    {
        std::cout << "Authentication failed for user '" << username << "'." << std::endl;
        return ESP_FAIL;
    }

    // Update the password for the given user
    LocalDB& db = LocalDB::getInstance();
    int userID = getUserID(username);
    if (userID == -1) 
    {
        std::cout << "User '" << username << "' not found." << std::endl;
        return ESP_FAIL;
    }
    std::map<std::string, std::string> updateValues = 
    {
        {"password", newPassword}
    };
    std::map<std::string, std::string> filter = {{"id", "1"}};
    db.update("UserCredentials", updateValues, filter);
    std::cout << "Password updated successfully for user '" << username << "'." << std::endl;
    return ESP_OK;
}


int UserCredentialsManager::getUserID(const std::string& username) 
{
    // Get the user ID for the given username
    LocalDB& db = LocalDB::getInstance();
    
    std::map<std::string, std::string> filter;
    filter["username"] = username;

    auto searchResults = db.search("UserCredentials", {"id"}, filter);
    if (searchResults.empty()) 
    {
        return -1; // User not found
    }
    return std::stoi(searchResults[0]["id"]);
}

esp_err_t UserCredentialsManager::authenticateUser(const std::string& username, const std::string& password) 
{
    // Check if the provided username and password match the stored credentials
    LocalDB& db = LocalDB::getInstance();
    std::map<std::string, std::string> filter;
    filter["username"] = username;

    auto searchResults = db.search("UserCredentials", {"password"}, filter);

    if (!searchResults.empty()) 
    {
        const std::string& storedPassword = searchResults.front().at("password");
        if (storedPassword == password)
        {
            return ESP_OK;
        }
    } 
    else 
    {
        std::cout << "User '" << username << "' not found." << std::endl;
        
    }
    return ESP_FAIL;
}
