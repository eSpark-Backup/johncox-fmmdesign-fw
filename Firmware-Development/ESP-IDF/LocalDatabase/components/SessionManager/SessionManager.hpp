#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>

#define MINUTES_TO_SECONDS(x) ((x) * 60)
#define HOURS_TO_SECONDS(x) (MINUTES_TO_SECONDS((x)* 60))

#define TOKEN_EXPIRE_DURATION MINUTES_TO_SECONDS(10) //minutes

enum TokenStatus 
{
    TOKEN_VALID,
    TOKEN_INVALID,
    TOKEN_EXPIRED
};

class SessionManager 
{
    public:
        static std::string getBearerToken();
        static TokenStatus verifyToken(char* token);

    private:
        static std::string generateRandomKey(int length = 32);
        static std::pair<std::string, std::string> getCredentials();
        static long getCurrentTimeInSeconds();
        static long getTimeElapsedSinceTokenGeneration();
        static std::string parseUsername(const char* payload);

        static std::string token;
        static std::string key;
        static long previousTokenGenerationTime;
};

#endif // SESSION_MANAGER_H
