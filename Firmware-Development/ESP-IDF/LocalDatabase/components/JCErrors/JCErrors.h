#pragma once

#ifdef JC_ERRORS_DEFINE
#define JC_ERROR(varName, string) \
    const char* varName = string
#else
#define JC_ERROR(varName, string) \
    extern const char* varName
#endif

#define JSON_MESSAGE(success, message) "{\n\t\"success\": " #success ",\n\t\"message\": \"" message "\"\n}"
#define JSON_MESSAGE_WITH_TOKEN(success, message, token) \
    "{\n\t\"success\": " #success ",\n\t\"message\": \"" message "\",\n\t\"token\": \"" + token + "\"\n}"

JC_ERROR(emptyRequestBodyResponse, JSON_MESSAGE(false, "Empty request body"));
JC_ERROR(memoryAllocationFailedResponse, JSON_MESSAGE(false, "Failed to allocate memory"));
JC_ERROR(receivingRequestBodyResponse, JSON_MESSAGE(false, "Error receiving request body"));
JC_ERROR(missingCredentialsResponse, JSON_MESSAGE(false, "Missing username or password"));
JC_ERROR(authenticationFailedResponse, JSON_MESSAGE(false, "Authentication failed"));
JC_ERROR(queryParseErrorResponse, JSON_MESSAGE(false, "Error parsing query parameters"));
JC_ERROR(missingConfigurationsResponse, JSON_MESSAGE(false, "missing configurations parameters"));
JC_ERROR(missingAuthorizationHeaderResponse, JSON_MESSAGE(false, "Authorization header missing"));
JC_ERROR(invalidHeaderFormatResponse, JSON_MESSAGE(false, "Invalid authorization header format"));
JC_ERROR(invalidTokenResponse, JSON_MESSAGE(false, "Invalid token"));
JC_ERROR(expiredTokenResponse, JSON_MESSAGE(false, "Expired token"));

JC_ERROR(configurationsUpdatedResponse, JSON_MESSAGE(true, "Configurations updated successfully"));
JC_ERROR(helloWorldResponse, JSON_MESSAGE(true, "Hello World !!!!!"));
// JC_ERROR(loginSuccessResponse, JSON_MESSAGE(true, "User login successful"));