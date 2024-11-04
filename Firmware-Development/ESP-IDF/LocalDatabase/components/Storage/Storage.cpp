#include "Storage.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "esp_spiffs.h"

const char *Storage::TAG = "Storage";
const char *Storage::MOUNT_POINT = nullptr;
sdmmc_card_t *Storage::card = nullptr;

void Storage::init() 
{
        esp_err_t ret;

    ret = mountNAND();
    if (ret != ESP_OK) 
    {
        MOUNT_POINT = MOUNT_POINT_SPIFFS;
        ESP_LOGE(TAG, "FAILED TO MOUNT NAND FLASH, SWITCHING TO SPIFFS.");
        mountSpiffs();
    }
    else
    {
        MOUNT_POINT = MOUNT_POINT_NAND;
    }
    ESP_LOGW(TAG, "mount point is (%s)", MOUNT_POINT);
}

void Storage::deinit()
{
    esp_vfs_spiffs_unregister(nullptr);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}

esp_err_t Storage::mountNAND()
{
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.  
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = NAND_MOSI,
        .miso_io_num = NAND_MISO,
        .sclk_io_num = NAND_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = NAND_CS;
    slot_config.host_id = (spi_host_device_t)host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT_NAND, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).\n"
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        spi_bus_free((spi_host_device_t)host.slot);
        ESP_LOGW(TAG, "SPI bus deinitialized.");
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);

    return ret;
}

esp_err_t Storage::mountSpiffs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = 
    {
      .base_path = MOUNT_POINT_SPIFFS,
      .partition_label = NULL,
      .max_files = 5,
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
      .format_if_mount_failed = true,
#else
      .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) 
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } 
        else if (ret == ESP_ERR_NOT_FOUND) 
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } 
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

void Storage::storageStats()
{
    if(strcmp(MOUNT_POINT, MOUNT_POINT_NAND) == 0)
    {
        FATFS* fs;
        DWORD fre_clust, fre_sect, tot_sect;

        if (f_getfree(MOUNT_POINT, &fre_clust, &fs) != FR_OK) {
            printf("Failed to get free space information.\n");
            return;
        }

        // Calculating total and free space
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        fre_sect = fre_clust * fs->csize;

        uint64_t total_space = tot_sect * 512ULL;
        uint64_t free_space = fre_sect * 512ULL;
        uint64_t used_space = total_space - free_space;

        ESP_LOGW(TAG, "Partition size: total: %llu MB, used: %llu MB, free: %llu MB", (total_space / (1024 * 1024)), (used_space / (1024 * 1024)), (free_space / (1024 * 1024)));
    }
    else if(strcmp(MOUNT_POINT, MOUNT_POINT_SPIFFS) == 0)
    {
        size_t total = 0, used = 0;
        esp_err_t ret = esp_spiffs_info(NULL, &total, &used);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } 
        else 
        {
            ESP_LOGW(TAG, "Partition size: total: %d, used: %d", total, used);
        }
    }
    else if(MOUNT_POINT != NULL)
    {
        ESP_LOGE(TAG, "Unknown mount point \"%s\".", MOUNT_POINT);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid mount point.");
    }
}
