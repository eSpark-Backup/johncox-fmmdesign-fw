/**
 * @file littlefs_sdmmc.c
 * @brief Maps the HAL of sdmmc driver <-> littlefs
 * @author Jackson Ming Hu <jackson@smartguide.com.au>
 */

#include <sdmmc_cmd.h>
#include <sys/param.h>
#include "littlefs_api.h"

#if CONFIG_LITTLEFS_SDMMC_SUPPORT

// ////////////////////////////////////////////////////////////////////////////////////

// #include "../sdmmc_common.h"

// #define SDMMC_SD_DISCARD_TIMEOUT  250    // SD erase (discard) timeout

// /* supported arguments for erase command 38 */
// #define SDMMC_SD_ERASE_ARG      0
// #define SDMMC_SD_DISCARD_ARG    1
// #define SDMMC_MMC_TRIM_ARG      1
// #define SDMMC_MMC_DISCARD_ARG   3

// #define SD_OCR_SDHC_CAP                 (1<<30)             /* HCS bit */
// #define MMC_ERASE_GROUP_START           35      /* R1 */
// #define MMC_ERASE_GROUP_END             36      /* R1 */
// #define MMC_ERASE                       38      /* R1B */
// #define SD_ERASE_GROUP_START            32      /* R1 */
// #define SD_ERASE_GROUP_END              33      /* R1 */

// #define SDSPI_CMD_FLAG_RSP_R1B  BIT(3)  //!< Response format R1 (1 byte), with busy polling

// static const char *TAG = "demo_esp_littlefs";

// typedef enum {
//     SDMMC_ERASE_ARG = 0,      /*!< Erase operation on SD, Trim operation on MMC */
//     SDMMC_DISCARD_ARG = 1,    /*!< Discard operation for SD/MMC */
// } sdmmc_erase_arg_t;

// // static inline bool host_is_spi(const sdmmc_card_t* card)
// // {
// //     return (card->host.flags & SDMMC_HOST_FLAG_SPI) != 0;
// // }

// esp_err_t sdmmc_can_discard(sdmmc_card_t* card)
// {
//     // if ((!card->is_mmc) && !host_is_spi(card) && (card->ssr.discard_support == 1)) {
//     if ((!card->is_mmc) && !host_is_spi(card)) {
//         return ESP_OK;
//     }
//     return ESP_FAIL;
// }

// uint32_t sdmmc_sd_get_erase_timeout_ms(const sdmmc_card_t* card, int arg, size_t erase_size_kb)
// {
//     if (arg == SDMMC_SD_DISCARD_ARG) {
//         return SDMMC_SD_DISCARD_TIMEOUT;
//     } else if (arg == SDMMC_SD_ERASE_ARG) {
//         // if (card->ssr.alloc_unit_kb != 0 &&
//         //         card->ssr.erase_size_au != 0 &&
//         //         card->ssr.erase_timeout != 0 &&
//         //         card->ssr.erase_offset != 0) {
//         //     /* Card supports erase timeout estimation. See the erase timeout equation in SD spec. */
//         //     uint32_t timeout_sec = card->ssr.erase_offset +
//         //         card->ssr.erase_timeout * (erase_size_kb + card->ssr.alloc_unit_kb - 1) /
//         //             (card->ssr.erase_size_au * card->ssr.alloc_unit_kb);
//         //     ESP_LOGD(TAG, "%s: erase timeout %" PRIu32 " s (erasing %" PRIu32 " kB, ES=%" PRIu32 ", ET=%" PRIu32 ", EO=%" PRIu32 ", AU=%" PRIu32 " kB)",
//         //              __func__, timeout_sec, (uint32_t) erase_size_kb, (uint32_t) card->ssr.erase_size_au,
//         //              (uint32_t) card->ssr.erase_timeout, (uint32_t) card->ssr.erase_offset, (uint32_t) card->ssr.alloc_unit_kb);
//         //     return timeout_sec * 1000;
//         // } else {
//             uint32_t timeout_ms = SDMMC_SD_DISCARD_TIMEOUT * erase_size_kb / card->csd.sector_size;
//             timeout_ms = MAX(1000, timeout_ms);
//             ESP_LOGD(TAG, "%s: erase timeout %" PRIu32 " s (erasing %" PRIu32 " kB, %" PRIu32 " ms per sector)",
//                      __func__, (uint32_t) (timeout_ms / 1000), (uint32_t) erase_size_kb, (uint32_t) SDMMC_SD_DISCARD_TIMEOUT);
//             return timeout_ms;
//         // }
//     } else {
//         assert(false && "unexpected SD erase argument");
//         return 0;
//     }
// }

// uint32_t sdmmc_get_erase_timeout_ms(const sdmmc_card_t* card, int arg, size_t erase_size_kb)
// {
//     // if (card->is_mmc) {
//     //     return sdmmc_mmc_get_erase_timeout_ms(card, arg, erase_size_kb);
//     // } else {
//         return sdmmc_sd_get_erase_timeout_ms(card, arg, erase_size_kb);
//     // }
// }

// esp_err_t sdmmc_erase_sectors(sdmmc_card_t* card, size_t start_sector,
//         size_t sector_count, sdmmc_erase_arg_t arg)
// {
//     if (sector_count == 0) {
//         return ESP_OK;
//     }

//     if (start_sector + sector_count > card->csd.capacity) {
//         return ESP_ERR_INVALID_SIZE;
//     }

//     uint32_t cmd38_arg;
//     // if (arg == SDMMC_ERASE_ARG) {
//         cmd38_arg = SDMMC_SD_ERASE_ARG;
//     // } else {
//     //     cmd38_arg = SDMMC_SD_DISCARD_ARG;
//     // }

//     // // /* validate the CMD38 argument against card supported features */
//     // // if (card->is_mmc) {
//     // //     if ((cmd38_arg == SDMMC_MMC_TRIM_ARG) && (sdmmc_can_trim(card) != ESP_OK)) {
//     // //         return ESP_ERR_NOT_SUPPORTED;
//     // //     }
//     // //     if ((cmd38_arg == SDMMC_MMC_DISCARD_ARG) && (sdmmc_can_discard(card) != ESP_OK)) {
//     // //         return ESP_ERR_NOT_SUPPORTED;
//     // //     }
//     // // } else { // SD card
//     //     if ((cmd38_arg == SDMMC_SD_DISCARD_ARG) && (sdmmc_can_discard(card) != ESP_OK)) {
//     //         return ESP_ERR_NOT_SUPPORTED;
//     //     }
//     // // }

//     /* default as block unit address */
//     size_t addr_unit_mult = 1;

//     if (!(card->ocr & SD_OCR_SDHC_CAP)) {
//         addr_unit_mult = card->csd.sector_size;
//     }

//     /* prepare command to set the start address */
//     sdmmc_command_t cmd = {
//             .flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_WAIT_BUSY,
//             .opcode = SD_ERASE_GROUP_START,
//             .arg = (start_sector * addr_unit_mult),
//     };

//     ESP_LOGW(TAG, "cmd.flags = %d, cmd.opcode = %u, cmd.arg = %u, cmd.timeout_ms = %d", cmd.flags, cmd.opcode, cmd.arg, cmd.timeout_ms);
//     esp_err_t err = sdmmc_send_cmd(card, &cmd);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "%s: sdmmc_send_cmd (ERASE_GROUP_START) returned 0x%x", __func__, err);
//         return err;
//     }

//     /* prepare command to set the end address */
//     cmd.opcode = SD_ERASE_GROUP_END;
//     cmd.arg = ((start_sector + (sector_count - 1)) * addr_unit_mult);

//     ESP_LOGW(TAG, "cmd.flags = %d, cmd.opcode = %u, cmd.arg = %u, cmd.timeout_ms = %d", cmd.flags, cmd.opcode, cmd.arg, cmd.timeout_ms);
//     err = sdmmc_send_cmd(card, &cmd);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "%s: sdmmc_send_cmd (ERASE_GROUP_END) returned 0x%x", __func__, err);
//         return err;
//     }

//     /* issue erase command */
//     memset((void *)&cmd, 0 , sizeof(sdmmc_command_t));
//     cmd.flags = SCF_CMD_AC | SCF_RSP_R1B | SCF_WAIT_BUSY | SDSPI_CMD_FLAG_RSP_R1B;
//     cmd.opcode = MMC_ERASE;
//     cmd.arg = cmd38_arg;
//     cmd.timeout_ms = sdmmc_get_erase_timeout_ms(card, cmd38_arg, sector_count * card->csd.sector_size / 1024);

//     ESP_LOGW(TAG, "cmd.flags = %d, cmd.opcode = %u, cmd.arg = %u, cmd.timeout_ms = %d", cmd.flags, cmd.opcode, cmd.arg, cmd.timeout_ms);
//     err = sdmmc_send_cmd(card, &cmd);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "%s: sdmmc_send_cmd (ERASE) returned 0x%x", __func__, err);
//         return err;
//     }
//     ESP_LOGW(TAG, "Erase command executed.");

//     if (host_is_spi(card)) {
//         uint32_t status;
//         err = sdmmc_send_cmd_send_status(card, &status);
//         if (err != ESP_OK) {
//             ESP_LOGE(TAG, "%s: sdmmc_send_cmd_send_status returned 0x%x", __func__, err);
//             return err;
//         }
//         if (status != 0) {
//             ESP_LOGE(TAG, "%s: card status indicates an error after erase operation: r2=0x%04" PRIx32,
//                      __func__, status);
//             return ESP_ERR_INVALID_RESPONSE;
//         }
//     }

//     return ESP_OK;
// }


// // esp_err_t sdmmc_full_erase(sdmmc_card_t* card)
// // {
// //     sdmmc_erase_arg_t arg = SDMMC_SD_ERASE_ARG; // erase by default for SD card
// //     esp_err_t err;
// //     // if (card->is_mmc) {
// //     //     arg = sdmmc_mmc_can_sanitize(card) == ESP_OK ? SDMMC_MMC_DISCARD_ARG: SDMMC_MMC_TRIM_ARG;
// //     // }
// //     err = sdmmc_erase_sectors(card, 0, card->csd.capacity, arg);
// //     if ((err == ESP_OK) && (arg == SDMMC_MMC_DISCARD_ARG)) {
// //         uint32_t timeout_ms = sdmmc_get_erase_timeout_ms(card, SDMMC_MMC_DISCARD_ARG, card->csd.capacity * ((uint64_t) card->csd.sector_size) / 1024);
// //         return sdmmc_mmc_sanitize(card, timeout_ms);
// //     }
// //     return err;
// // }

// ////////////////////////////////////////////////////////////////////////////////////

// #if ESP_IDF_VERSION_MAJOR < 5
// #error "SDMMC feature is only supported in ESP-IDF v5+, see: https://github.com/joltwallet/esp_littlefs/pull/170#issuecomment-1882484668"
// #else

int littlefs_sdmmc_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    esp_littlefs_t * efs = c->context;
    uint32_t part_off = (block * c->block_size) + off;

    esp_err_t ret = sdmmc_read_sectors(efs->sdcard, buffer, block, MIN(size / efs->cfg.read_size, 1));
    if (ret != ESP_OK) {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Failed to read addr 0x%08lx: off 0x%08lx, block 0x%08lx, size %lu, err=0x%x", (long unsigned int)part_off, (long unsigned int)off, (long unsigned int)block, (long unsigned int)size, ret);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int littlefs_sdmmc_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    esp_littlefs_t * efs = c->context;
    uint32_t part_off = (block * c->block_size) + off;

    esp_err_t ret = sdmmc_write_sectors(efs->sdcard, buffer, block, MIN(size / efs->cfg.prog_size, 1));
    if (ret != ESP_OK) {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Failed to write addr 0x%08lx: off 0x%08lx, block 0x%08lx, size %lu, err=0x%x", (long unsigned int)part_off, (long unsigned int)off, (long unsigned int)block, (long unsigned int)size, ret);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int littlefs_sdmmc_erase(const struct lfs_config *c, lfs_block_t block)
{
    esp_littlefs_t * efs = c->context;
    // ESP_LOGW(ESP_LITTLEFS_TAG, "\"%s\" was called.", __ASSERT_FUNC);
    // ESP_LOGW(ESP_LITTLEFS_TAG, "block = %lu", (uint32_t)block);
    // ESP_LOGW(ESP_LITTLEFS_TAG, "block = %d", (uint32_t)block);
    // esp_err_t ret = sdmmc_erase_sectors(efs->sdcard, block, 1, SDMMC_ERASE_ARG);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(ESP_LITTLEFS_TAG, "Failed to erase block %lu: ret=0x%x %s", (long unsigned int)block, ret, esp_err_to_name(ret));
    //     return LFS_ERR_IO;
    // }

    return LFS_ERR_OK;
}

int littlefs_sdmmc_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK; // Doesn't require & doesn't support sync
}
// #endif

#endif
