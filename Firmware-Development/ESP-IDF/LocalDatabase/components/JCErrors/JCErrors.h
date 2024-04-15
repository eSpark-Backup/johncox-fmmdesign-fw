#pragma once

#include "esp_http_server.h" // Include the HTTP server library for the enum definition

typedef enum{
    HTTPD_200_OK = 0xff
} httpd_success_code_t;

#ifdef JC_RESPONSES_DEFINE
#define JC_RESPONSE(varName, errorCode, errorMessage) \
    const ErrorResponse varName = {errorCode, #errorCode, JSON_MESSAGE(errorCode, errorMessage)}
#else
#define JC_RESPONSE(varName, errorCode, errorMessage) \
    extern const ErrorResponse varName
#endif

typedef struct {
    httpd_err_code_t errorCode;
    const char* errorString;
    const char* errorMessage;
} ErrorResponse;

#define JSON_MESSAGE(errorCode, errorMessage) \
    "{\n\t\t\"status\": " #errorCode ",\n\t\t\"message\": \"" errorMessage "\"\n\t}"

#define JSON_MESSAGE_WITH_TOKEN(errorCode, message, token) \
    "{\n\t\"status\": " #errorCode ",\n\t\"message\": \"" message "\",\n\t\"token\": \"" + token + "\"\n}"

JC_RESPONSE(emptyRequestBody, HTTPD_400_BAD_REQUEST, "Empty request body"); 
JC_RESPONSE(memoryAllocationFailed, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate memory"); 
JC_RESPONSE(receivingRequestBody, HTTPD_500_INTERNAL_SERVER_ERROR, "Error receiving request body"); 
JC_RESPONSE(missingCredentials, HTTPD_400_BAD_REQUEST, "Missing username or password"); 
JC_RESPONSE(authenticationFailed, HTTPD_401_UNAUTHORIZED, "Authentication failed");  
JC_RESPONSE(queryParseError, HTTPD_400_BAD_REQUEST, "Error parsing query parameters"); 
JC_RESPONSE(missingConfigurationParams, HTTPD_400_BAD_REQUEST, "missing configurations parameters"); 
JC_RESPONSE(missingAuthorizationHeader, HTTPD_401_UNAUTHORIZED, "Authorization header missing"); 
JC_RESPONSE(invalidHeaderFormat, HTTPD_401_UNAUTHORIZED, "Invalid authorization header format"); 
JC_RESPONSE(invalidToken, HTTPD_401_UNAUTHORIZED, "Invalid token"); 
JC_RESPONSE(expiredToken, HTTPD_401_UNAUTHORIZED, "Expired token"); 
JC_RESPONSE(engineIdNotFound, HTTPD_404_NOT_FOUND, "engine_id not found in the database"); 
JC_RESPONSE(noEnabledEngines, HTTPD_403_FORBIDDEN, "No enabled engines found"); 
 
JC_RESPONSE(configurationsUpdated, HTTPD_200_OK, "Configurations updated successfully");
JC_RESPONSE(helloWorld, HTTPD_200_OK, "Hello World !!!!!");
// JC_RESPONSE(loginSuccessResponse, JSON_MESSAGE(true, "User login successful"));