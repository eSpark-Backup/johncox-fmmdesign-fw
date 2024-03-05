#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

class Storage 
{
    public:
        static void init();
        static void deinit();

    private:
        static const char *TAG;
};

#endif /* STORAGE_H */
