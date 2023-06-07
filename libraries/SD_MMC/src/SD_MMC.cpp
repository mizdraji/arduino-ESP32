// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pins_arduino.h"
#include "SD_MMC.h"
#ifdef SOC_SDMMC_HOST_SUPPORTED
#include "vfs_api.h"

#include <dirent.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "soc/sdmmc_pins.h"
#include "ff.h"

using namespace fs;


SDMMCFS::SDMMCFS(FSImplPtr impl)
    : FS(impl), _card(nullptr)
{
#if defined(SOC_SDMMC_USE_GPIO_MATRIX) && defined(BOARD_HAS_SDMMC)
    _pin_clk = SDMMC_CLK;
    _pin_cmd = SDMMC_CMD;
    _pin_d0 = SDMMC_D0;
#ifndef BOARD_HAS_1BIT_SDMMC
    _pin_d1 = SDMMC_D1;
    _pin_d2 = SDMMC_D2;
    _pin_d3 = SDMMC_D3;
#endif // BOARD_HAS_1BIT_SDMMC
#endif // defined(SOC_SDMMC_USE_GPIO_MATRIX) && defined(BOARD_HAS_SDMMC)
}

bool SDMMCFS::setPins(int clk, int cmd, int d0)
{
    return setPins(clk, cmd, d0, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC);
}

bool SDMMCFS::setPins(int clk, int cmd, int d0, int d1, int d2, int d3)
{
    if (_card != nullptr) {
        log_e("SD_MMC.setPins must be called before SD_MMC.begin");
        return false;
    }
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    // SoC supports SDMMC pin configuration via GPIO matrix. Save the pins for later use in SDMMCFS::begin.
    _pin_clk = (int8_t) clk;
    _pin_cmd = (int8_t) cmd;
    _pin_d0 = (int8_t) d0;
    _pin_d1 = (int8_t) d1;
    _pin_d2 = (int8_t) d2;
    _pin_d3 = (int8_t) d3;
    return true;
#elif CONFIG_IDF_TARGET_ESP32
    // ESP32 doesn't support SDMMC pin configuration via GPIO matrix.
    // Since SDMMCFS::begin hardcodes the usage of slot 1, only check if
    // the pins match slot 1 pins.
    bool pins_ok = (clk == (int)SDMMC_SLOT1_IOMUX_PIN_NUM_CLK) &&
        (cmd == (int)SDMMC_SLOT1_IOMUX_PIN_NUM_CMD) &&
        (d0 == (int)SDMMC_SLOT1_IOMUX_PIN_NUM_D0) &&
        (((d1 == -1) && (d2 == -1) && (d3 == -1)) ||
         ((d1 == (int)SDMMC_SLOT1_IOMUX_PIN_NUM_D1) &&
         (d2 == (int)SDMMC_SLOT1_IOMUX_PIN_NUM_D2) &&
         (d3 == (int)SDMMC_SLOT1_IOMUX_PIN_NUM_D3)));
    if (!pins_ok) {
        log_e("SDMMCFS: specified pins are not supported by this chip.");
        return false;
    }
    return true;
#else
#error SoC not supported
#endif
}

int SDMMCFS::getClkPin()
{
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    return _pin_clk;
#elif CONFIG_IDF_TARGET_ESP32
    return SDMMC_SLOT1_IOMUX_PIN_NUM_CLK;
#endif
}

int SDMMCFS::getCmdPin()
{
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    return _pin_cmd;
#elif CONFIG_IDF_TARGET_ESP32
    return SDMMC_SLOT1_IOMUX_PIN_NUM_CMD;
#endif
}

int SDMMCFS::getD0Pin()
{
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    return _pin_d0;
#elif CONFIG_IDF_TARGET_ESP32
    return SDMMC_SLOT1_IOMUX_PIN_NUM_D0;
#endif
}

int SDMMCFS::getD1Pin()
{
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    return _pin_d1;
#elif CONFIG_IDF_TARGET_ESP32
    return SDMMC_SLOT1_IOMUX_PIN_NUM_D1;
#endif
}

int SDMMCFS::getD2Pin()
{
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    return _pin_d2;
#elif CONFIG_IDF_TARGET_ESP32
    return SDMMC_SLOT1_IOMUX_PIN_NUM_D2;
#endif
}

int SDMMCFS::getD3Pin()
{
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    return _pin_d3;
#elif CONFIG_IDF_TARGET_ESP32
    return SDMMC_SLOT1_IOMUX_PIN_NUM_D3;
#endif
}

bool SDMMCFS::begin(const char * mountpoint, bool mode1bit, bool format_if_mount_failed, int sdmmc_frequency, uint8_t maxOpenFiles)
{
    if(_card) {
        return true;
    }
    //mount
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    // SoC supports SDMMC pin configuration via GPIO matrix.
    // Chech that the pins have been set either in the constructor or setPins function.
    if (_pin_cmd == -1 || _pin_clk == -1 || _pin_d0 == -1
        || (!mode1bit && (_pin_d1 == -1 || _pin_d2 == -1 || _pin_d3 == -1))) {
        log_e("SDMMCFS: some SD pins are not set");
        return false;
    }

    slot_config.clk = (gpio_num_t) _pin_clk;
    slot_config.cmd = (gpio_num_t) _pin_cmd;
    slot_config.d0 = (gpio_num_t) _pin_d0;
    slot_config.d1 = (gpio_num_t) _pin_d1;
    slot_config.d2 = (gpio_num_t) _pin_d2;
    slot_config.d3 = (gpio_num_t) _pin_d3;
    slot_config.width = 4;
#endif // SOC_SDMMC_USE_GPIO_MATRIX
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_4BIT;
    host.slot = SDMMC_HOST_SLOT_1;
    host.max_freq_khz = sdmmc_frequency;
#ifdef BOARD_HAS_1BIT_SDMMC
    mode1bit = true;
#endif
    if(mode1bit) {
        host.flags = SDMMC_HOST_FLAG_1BIT; //use 1-line SD mode
        slot_config.width = 1;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_mount_failed,
        .max_files = maxOpenFiles,
        .allocation_unit_size = 0
    };

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mountpoint, &host, &slot_config, &mount_config, &_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            log_e("Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else if (ret == ESP_ERR_INVALID_STATE) {
            _impl->mountpoint(mountpoint);
            log_w("SD Already mounted");
            return true;
        } else {
            log_e("Failed to initialize the card (0x%x). Make sure SD card lines have pull-up resistors in place.", ret);
        }
        _card = NULL;
        return false;
    }
    _impl->mountpoint(mountpoint);
    return true;
}

void SDMMCFS::end()
{
    if(_card) {
        esp_vfs_fat_sdmmc_unmount();
        _impl->mountpoint(NULL);
        _card = NULL;
    }
}

sdcard_type_t SDMMCFS::cardType()
{
    if(!_card) {
        return CARD_NONE;
    }
    return (_card->ocr & SD_OCR_SDHC_CAP)?CARD_SDHC:CARD_SD;
}

uint64_t SDMMCFS::cardSize()
{
    if(!_card) {
        return 0;
    }
    return (uint64_t)_card->csd.capacity * _card->csd.sector_size;
}

uint64_t SDMMCFS::totalBytes()
{
	FATFS* fsinfo;
	DWORD fre_clust;
	if(f_getfree("0:",&fre_clust,&fsinfo)!= 0) return 0;
    uint64_t size = ((uint64_t)(fsinfo->csize))*(fsinfo->n_fatent - 2)
#if _MAX_SS != 512
        *(fsinfo->ssize);
#else
        *512;
#endif
	return size;
}

uint64_t SDMMCFS::usedBytes()
{
	FATFS* fsinfo;
	DWORD fre_clust;
	if(f_getfree("0:",&fre_clust,&fsinfo)!= 0) return 0;
	uint64_t size = ((uint64_t)(fsinfo->csize))*((fsinfo->n_fatent - 2) - (fsinfo->free_clst))
#if _MAX_SS != 512
        *(fsinfo->ssize);
#else
        *512;
#endif
	return size;
}

SDMMCFS SD_MMC = SDMMCFS(FSImplPtr(new VFSImpl()));
#endif /* SOC_SDMMC_HOST_SUPPORTED */
