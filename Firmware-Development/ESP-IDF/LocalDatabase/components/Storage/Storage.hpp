#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"
#include "esp_log.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT_SPIFFS "/spiffs"
#define MOUNT_POINT_NAND   "/nand"

#define CONFIG_FORMAT_IF_MOUNT_FAILED

// Onboard NAND memory interface
#define NAND_MISO  GPIO_NUM_12
#define NAND_MOSI  GPIO_NUM_13
#define NAND_CLK   GPIO_NUM_14
#define NAND_CS    GPIO_NUM_15

class Storage 
{
    public:
        static void init();
        static void deinit();
        static esp_err_t mountNAND();
        static esp_err_t mountSpiffs();
        static void storageStats();

        static const char *MOUNT_POINT;

    private:
        static const char *TAG;
        static sdmmc_card_t *card;
};

#endif /* STORAGE_H */
