#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"
#include "esp_log.h"
#include "sdmmc_cmd.h"
#include <esp_littlefs.h>

#define USE_LITTLEFS

#define MOUNT_POINT_SPIFFS "/spiffs"
#define MOUNT_POINT_NAND   "/nand"

#define CONFIG_FORMAT_IF_MOUNT_FAILED

#define NAND_PAGE_SIZE      512
// #define NAND_PAGE_SIZE      SOC_SPI_MAXIMUM_BUFFER_SIZE
#define NAND_BLOCK_SIZE     (64 * NAND_PAGE_SIZE)

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
        static void storageStats();

        static const char *MOUNT_POINT;

    private:
        static esp_err_t mountNAND();
        static esp_err_t mountSpiffs();
        
        static const char *TAG;
        static sdmmc_card_t *card;
        static sdmmc_host_t  spiHost;
};

#endif /* STORAGE_H */
