#pragma once

#ifdef JC_RESPONSES_DEFINE
#define JC_RESPONSE(varName, string) \
    const char* varName = string
#else
#define JC_RESPONSE(varName, string) \
    extern const char* varName
#endif

#define JSON_MESSAGE(success, message) "{\n\t\"success\": " #success ",\n\t\"message\": \"" message "\"\n}"
#define JSON_MESSAGE_WITH_TOKEN(success, message, token) \
    "{\n\t\"success\": " #success ",\n\t\"message\": \"" message "\",\n\t\"token\": \"" + token + "\"\n}"

JC_RESPONSE(emptyRequestBodyResponse, JSON_MESSAGE(false, "Empty request body"));
JC_RESPONSE(memoryAllocationFailedResponse, JSON_MESSAGE(false, "Failed to allocate memory"));
JC_RESPONSE(receivingRequestBodyResponse, JSON_MESSAGE(false, "Error receiving request body"));
JC_RESPONSE(missingCredentialsResponse, JSON_MESSAGE(false, "Missing username or password"));
JC_RESPONSE(authenticationFailedResponse, JSON_MESSAGE(false, "Authentication failed"));
JC_RESPONSE(queryParseErrorResponse, JSON_MESSAGE(false, "Error parsing query parameters"));
JC_RESPONSE(missingConfigurationsResponse, JSON_MESSAGE(false, "missing configurations parameters"));
JC_RESPONSE(missingAuthorizationHeaderResponse, JSON_MESSAGE(false, "Authorization header missing"));
JC_RESPONSE(invalidHeaderFormatResponse, JSON_MESSAGE(false, "Invalid authorization header format"));
JC_RESPONSE(invalidTokenResponse, JSON_MESSAGE(false, "Invalid token"));
JC_RESPONSE(expiredTokenResponse, JSON_MESSAGE(false, "Expired token"));

JC_RESPONSE(configurationsUpdatedResponse, JSON_MESSAGE(true, "Configurations updated successfully"));
JC_RESPONSE(helloWorldResponse, JSON_MESSAGE(true, "Hello World !!!!!"));
// JC_RESPONSE(loginSuccessResponse, JSON_MESSAGE(true, "User login successful"));