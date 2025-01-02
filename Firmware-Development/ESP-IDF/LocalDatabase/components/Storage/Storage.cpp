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
sdmmc_host_t Storage::spiHost;

void Storage::init() 
{
    if (mountNAND() != ESP_OK) 
    {
        ESP_LOGE(TAG, "FAILED TO MOUNT NAND FLASH, SWITCHING TO SPIFFS.");
        if (mountSpiffs() != ESP_OK) 
        {
            ESP_LOGE(TAG, "FAILED TO MOUNT SPIFFS.");
            return;
        }
    }

    ESP_LOGW(TAG, "mount point is (%s)", MOUNT_POINT);
    storageStats();
}

void Storage::deinit()
{
    if(strcmp(MOUNT_POINT, MOUNT_POINT_NAND) == 0)
    {
#ifndef USE_LITTLEFS
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT_NAND, card);
#else
        esp_vfs_littlefs_unregister_sdmmc(card);
#endif
        card = NULL;
        spi_bus_free((spi_host_device_t)spiHost.slot);
        ESP_LOGI(TAG, "NAND unmounted and SPI bus deinitialized.");
    }
    else if(strcmp(MOUNT_POINT, MOUNT_POINT_SPIFFS) == 0)
    {
#ifndef USE_LITTLEFS
        esp_vfs_spiffs_unregister(nullptr);
#else
        esp_vfs_littlefs_unregister(nullptr);
#endif
        ESP_LOGI(TAG, "SPIFFS unmounted.");
    }
    MOUNT_POINT = NULL;
}

void printCardData(sdmmc_card_t* sdcard)
{
    ESP_LOGW("NAND_REGISTERS", "sdcard->is_mmc = %d, sdcard->is_mem = %d, sdcard->is_sdio = %d", sdcard->is_mmc, sdcard->is_mem, sdcard->is_sdio);
    ESP_LOGW("NAND_REGISTERS", "sdcard->ocr = %d", sdcard->ocr);
    // ESP_LOGW("NAND_REGISTERS", "sdcard->ocr = %lu", sdcard->ocr);
    ESP_LOGW("NAND_REGISTERS", "sdcard->cid.date = %d", sdcard->cid.date);
    ESP_LOGW("NAND_REGISTERS", "sdcard->cid.mfg_id = %d", sdcard->cid.mfg_id);
    ESP_LOGW("NAND_REGISTERS", "sdcard->cid.name = %s", sdcard->cid.name);
    ESP_LOGW("NAND_REGISTERS", "sdcard->cid.oem_id = %d", sdcard->cid.oem_id);
    ESP_LOGW("NAND_REGISTERS", "sdcard->cid.revision = %d", sdcard->cid.revision);
    ESP_LOGW("NAND_REGISTERS", "sdcard->cid.serial = %d", sdcard->cid.serial);
    ESP_LOGW("NAND_REGISTERS", "=========================================");
    ESP_LOGW("NAND_REGISTERS", "sdcard->scr.bus_width = %d", sdcard->scr.bus_width);
    // ESP_LOGW("NAND_REGISTERS", "sdcard->scr.erase_mem_state = %d", sdcard->scr.erase_mem_state);
    // ESP_LOGW("NAND_REGISTERS", "sdcard->scr.rsvd_mnf = %lu", sdcard->scr.rsvd_mnf);
    ESP_LOGW("NAND_REGISTERS", "sdcard->scr.sd_spec = %d", sdcard->scr.sd_spec);
    ESP_LOGW("NAND_REGISTERS", "=========================================");
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.capacity = %d", sdcard->csd.capacity);
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.card_command_class = %d", sdcard->csd.card_command_class);
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.csd_ver = %d", sdcard->csd.csd_ver);
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.mmc_ver = %d", sdcard->csd.mmc_ver);
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.read_block_len = %d", sdcard->csd.read_block_len);
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.sector_size = %d", sdcard->csd.sector_size);
    ESP_LOGW("NAND_REGISTERS", "sdcard->csd.tr_speed = %d", sdcard->csd.tr_speed);
}

esp_err_t Storage::mountNAND()
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spiHost = host;
    spiHost.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = NAND_MOSI,
        .miso_io_num = NAND_MISO,
        .sclk_io_num = NAND_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = NAND_PAGE_SIZE,
    };
    ret = spi_bus_initialize((spi_host_device_t)spiHost.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = NAND_CS;
    slot_config.host_id = (spi_host_device_t)spiHost.slot;


#ifndef USE_LITTLEFS
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 1024
        // .allocation_unit_size = 16 * 1024
    };
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT_NAND, &spiHost, &slot_config, &mount_config, &card);
#else
    ESP_LOGI(TAG, "Mounting LittleFS filesystem");

    int card_handle = -1;   //uninitialized
    // ret = init_sdspi_host(host.slot, &slot_config, &card_handle);
    ret = sdspi_host_init_device((const sdspi_device_config_t*)&slot_config, &card_handle);
    if(ESP_OK != ret)
    {
        ESP_LOGI(TAG, "slot init failed.");
    }
    else
    {
        ESP_LOGI(TAG, "slot init successfull.");
    }

    card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
    if (card == NULL) {
        ESP_LOGE(TAG, "could not locate new sdmmc_card_t");
    }

    ret = sdmmc_card_init(&host, card);
    if(ESP_OK != ret)
    {
        ESP_LOGI(TAG, "sdmmc_card_init failed.");
    }
    else
    {
        ESP_LOGI(TAG, "sdmmc_card_init successfull.");
    }

    ESP_LOGI(TAG, "Initializing LittelFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = MOUNT_POINT_NAND,
        .sdcard = card,
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .dont_mount = false,
        // .grow_on_mount = true,
    };
    ret = esp_vfs_littlefs_register(&conf);
#endif

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).\n"
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        // sdmmc_host_deinit();
        spi_bus_free((spi_host_device_t)spiHost.slot);
        ESP_LOGW(TAG, "SPI bus deinitialized.");
        MOUNT_POINT = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);
    printCardData(card);
    MOUNT_POINT = MOUNT_POINT_NAND;

    return ret;
}

esp_err_t Storage::mountSpiffs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

#ifndef USE_LITTLEFS
    ESP_LOGI(TAG, "Mounting FAT filesystem");

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
#else
    ESP_LOGI(TAG, "Mounting LittleFS filesystem");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = MOUNT_POINT_SPIFFS,
        .partition_label = NULL,
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
      .format_if_mount_failed = true,
#else
      .format_if_mount_failed = false,
#endif
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
#endif

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
        MOUNT_POINT = NULL;
        return ret;
    }
    MOUNT_POINT = MOUNT_POINT_SPIFFS;

    size_t total = 0, used = 0;
#ifndef USE_LITTLEFS
    ret = esp_spiffs_info(NULL, &total, &used);
#else
    ret = esp_littlefs_info(NULL, &total, &used);
#endif

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
    esp_err_t ret;

    if(MOUNT_POINT == NULL)
    {
        ESP_LOGE(TAG, "Storage not initialized.");
    }
    else if(strcmp(MOUNT_POINT, MOUNT_POINT_NAND) == 0)
    {
        uint64_t total_space, free_space, used_space;
#ifndef USE_LITTLEFS
        FATFS* fs;
        DWORD fre_clust, fre_sect, tot_sect;

        if (f_getfree(MOUNT_POINT, &fre_clust, &fs) != FR_OK) {
            printf("Failed to get free space information.\n");
            return;
        }

        // Calculating total and free space
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        fre_sect = fre_clust * fs->csize;

        total_space = tot_sect * 512ULL;
        free_space = fre_sect * 512ULL;
        used_space = total_space - free_space;
#else
        size_t total = 0, used = 0;
        ret = esp_littlefs_sdmmc_info(card, &total, &used);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
            return;
        }
        total_space = total;
        used_space = used;
        free_space = total_space - used_space;
#endif

        ESP_LOGI(TAG, "Partition size: total: %llu MB, used: %f MB, free: %f MB", (total_space / (1024 * 1024)), (used_space / (1024.0 * 1024.0)), (free_space / (1024.0 * 1024.0)));
    }
    else if(strcmp(MOUNT_POINT, MOUNT_POINT_SPIFFS) == 0)
    {
        size_t total = 0, used = 0;
#ifndef USE_LITTLEFS
        ret = esp_spiffs_info(NULL, &total, &used);
#else
        ret = esp_littlefs_info(NULL, &total, &used);
#endif
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

