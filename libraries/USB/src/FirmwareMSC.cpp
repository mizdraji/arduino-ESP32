// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstring>
#include "FirmwareMSC.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp32-hal.h"
#include "firmware_msc_fat.h"

#if CONFIG_TINYUSB_MSC_ENABLED
ESP_EVENT_DEFINE_BASE(ARDUINO_FIRMWARE_MSC_EVENTS);
esp_err_t arduino_usb_event_post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait);
esp_err_t arduino_usb_event_handler_register_with(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg);

//General Variables
static uint8_t * msc_ram_disk = NULL;
static fat_boot_sector_t * msc_boot = NULL;
static uint8_t * msc_table = NULL;
static uint16_t msc_table_sectors = 0;
static uint16_t msc_total_sectors = 0;
static bool mcs_is_fat16 = false;

//Firmware Read
static const esp_partition_t* msc_run_partition = NULL;
static uint16_t fw_start_sector = 0;
static uint16_t fw_end_sector = 0;
static size_t fw_size = 0;
static fat_dir_entry_t * fw_entry = NULL;

//Firmware Write
typedef enum {
  MSC_UPDATE_IDLE,
  MSC_UPDATE_STARTING,
  MSC_UPDATE_RUNNING,
  MSC_UPDATE_END
} msc_update_state_t;

static const esp_partition_t* msc_ota_partition = NULL;
static msc_update_state_t msc_update_state = MSC_UPDATE_IDLE;
static uint16_t msc_update_start_sector = 0;
static uint32_t msc_update_bytes_written = 0;
static fat_dir_entry_t * msc_update_entry = NULL;

static uint32_t get_firmware_size(const esp_partition_t* partition){
  esp_image_metadata_t data;
  const esp_partition_pos_t running_pos  = {
      .offset = partition->address,
      .size = partition->size,
  };
  data.start_addr = running_pos.offset;
  esp_image_verify(ESP_IMAGE_VERIFY, &running_pos, &data);
  return data.image_len;
}

//Get number of sectors required based on the size of the firmware and OTA partition
static size_t msc_update_get_required_disk_sectors(){
  size_t data_sectors = 16;
  size_t total_sectors = 0;
  msc_run_partition = esp_ota_get_running_partition();
  msc_ota_partition = esp_ota_get_next_update_partition(NULL);
  if(msc_run_partition){
    fw_size = get_firmware_size(msc_run_partition);
    data_sectors += FAT_SIZE_TO_SECTORS(fw_size);
    log_d("running size: %u (%u:%u)", fw_size, fw_size / DISK_SECTOR_SIZE, fw_size % DISK_SECTOR_SIZE);
  }
  if(msc_ota_partition){
    data_sectors += FAT_SIZE_TO_SECTORS(msc_ota_partition->size);
    log_d("ota size: %u (%u:%u)", msc_ota_partition->size, msc_ota_partition->size / DISK_SECTOR_SIZE, msc_ota_partition->size % DISK_SECTOR_SIZE);
  }
  msc_table_sectors = fat_sectors_per_alloc_table(data_sectors, false);
  total_sectors = data_sectors + msc_table_sectors + 2;
  if(total_sectors > 0xFF4){
    log_d("USING FAT16");
    mcs_is_fat16 = true;
    total_sectors -= msc_table_sectors;
    msc_table_sectors = fat_sectors_per_alloc_table(data_sectors, true);
    total_sectors += msc_table_sectors;
  } else {
    log_d("USING FAT12");
    mcs_is_fat16 = false;
  }
  log_d("data sectors: %u", data_sectors);
  log_d("table sectors: %u", msc_table_sectors);
  return total_sectors;
}

//setup the ramdisk and add the firmware download file
static void msc_update_setup_disk(const char * volume_label, uint32_t serial_number){
  msc_total_sectors = msc_update_get_required_disk_sectors();
  uint8_t ram_sectors = msc_table_sectors + 2;
  msc_ram_disk = (uint8_t*)calloc(ram_sectors, DISK_SECTOR_SIZE);
  fw_start_sector = ram_sectors;
  fw_end_sector = fw_start_sector;
  msc_boot = fat_add_boot_sector(msc_ram_disk, msc_total_sectors, msc_table_sectors, fat_file_system_type(mcs_is_fat16), volume_label, serial_number);
  msc_table = fat_add_table(msc_ram_disk, msc_boot, mcs_is_fat16);
  //fat_dir_entry_t * label = fat_add_label(msc_ram_disk, volume_label);
  if(msc_run_partition){
    fw_entry = fat_add_root_file(msc_ram_disk, 0, "ACTIVEFW", "BIN", fw_size, 2, mcs_is_fat16);
    fw_end_sector = FAT_SIZE_TO_SECTORS(fw_size) + fw_start_sector;
  }
}

//filter out entries to only include BINs in the root folder
static fat_dir_entry_t * msc_update_get_root_bin_entry(uint8_t index){
  fat_dir_entry_t * entry = (fat_dir_entry_t *)(msc_ram_disk + ((msc_boot->sectors_per_alloc_table+1) * DISK_SECTOR_SIZE) + (index * sizeof(fat_dir_entry_t)));
  fat_lfn_entry_t * lfn = (fat_lfn_entry_t*)entry;

  //fw read entry
  if(entry == fw_entry){
    return NULL;
  }
  //empty entry
  if(entry->file_magic == 0){
    return NULL;
  }
  //long file name
  if(lfn->attr == 0x0F && lfn->type == 0x00 && lfn->first_cluster == 0x0000){
    return NULL;
  }
  //only files marked as archives
  if(entry->file_attr != FAT_FILE_ATTR_ARCHIVE){
    return NULL;
  }
  //deleted
  if(entry->file_magic == 0xE5 || entry->file_magic == 0x05){
    return NULL;
  }
  //not bins
  if(memcmp("BIN", entry->file_extension, 3)){
    return NULL;
  }
  return entry;
}

//get an empty bin (the host will add an entry for file about to be written with size of zero)
static fat_dir_entry_t * msc_update_find_new_bin(){
  for(uint8_t i=15; i; i--){
    fat_dir_entry_t * entry = msc_update_get_root_bin_entry(i);
    if(entry && entry->file_size == 0){
      return entry;
    }
  }
  return NULL;
}

//write the new data and erase the flash blocks when necessary
static esp_err_t msc_update_write(const esp_partition_t *partition, uint32_t offset, void *data, size_t size){
  esp_err_t err = ESP_OK;
  if((offset & (SPI_FLASH_SEC_SIZE-1)) == 0){
    err = esp_partition_erase_range(partition, offset, SPI_FLASH_SEC_SIZE);
    //log_i("ERASE[0x%08X]: %s", offset, (err != ESP_OK)?"FAIL":"OK");
    if(err != ESP_OK){
      return err;
    }
  }
  return esp_partition_write(partition, offset, data, size);
}

//called when error was encountered while updating
static void msc_update_error(){
  //log_e("UPDATE_ERROR: %u", msc_update_bytes_written);
  arduino_firmware_msc_event_data_t p = {0};
  p.error.size = msc_update_bytes_written;
  arduino_usb_event_post(ARDUINO_FIRMWARE_MSC_EVENTS, ARDUINO_FIRMWARE_MSC_ERROR_EVENT, &p, sizeof(arduino_firmware_msc_event_data_t), portMAX_DELAY);
  msc_update_state = MSC_UPDATE_IDLE;
  msc_update_entry = NULL;
  msc_update_bytes_written = 0;
  msc_update_start_sector = 0;
}

//called when all firmware bytes have been received
static void msc_update_end(){
  log_v("UPDATE_END: %u (%u)", msc_update_entry->file_size, msc_update_bytes_written - msc_update_entry->file_size);
  msc_update_state = MSC_UPDATE_END;
  size_t ota_size = get_firmware_size(msc_ota_partition);
  if(ota_size != msc_update_entry->file_size){
    log_e("OTA SIZE MISMATCH %u != %u", ota_size, msc_update_entry->file_size);
    msc_update_error();
    return;
  }
  if(!ota_size || esp_ota_set_boot_partition(msc_ota_partition) != ESP_OK){
    log_e("ENABLING OTA PARTITION FAILED");
    msc_update_error();
    return;
  }
  arduino_firmware_msc_event_data_t p = {0};
  p.end.size = msc_update_entry->file_size;
  arduino_usb_event_post(ARDUINO_FIRMWARE_MSC_EVENTS, ARDUINO_FIRMWARE_MSC_END_EVENT, &p, sizeof(arduino_firmware_msc_event_data_t), portMAX_DELAY);
}

static int32_t msc_write(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize){
  //log_d("lba: %u, offset: %u, bufsize: %u", lba, offset, bufsize);
  if(lba < fw_start_sector){
    //write to sectors that are in RAM
    memcpy(msc_ram_disk + (lba * DISK_SECTOR_SIZE) + offset, buffer, bufsize);
    if(lba == (fw_start_sector - 1)){
      //monitor the root folder table
      if(msc_update_state <= MSC_UPDATE_RUNNING){
        fat_dir_entry_t * update_entry = msc_update_find_new_bin();
        if(update_entry) {
          if(msc_update_entry) {
            log_v("REPLACING ENTRY");
          } else {
            log_v("ASSIGNING ENTRY");
          }
          if(msc_update_state <= MSC_UPDATE_STARTING){
            msc_update_state = MSC_UPDATE_STARTING;
            msc_update_bytes_written = 0;
            msc_update_start_sector = 0;
          }
          msc_update_entry = update_entry;
        } else if(msc_update_state == MSC_UPDATE_RUNNING){
          if(msc_update_entry && msc_update_bytes_written >= msc_update_entry->file_size){
            msc_update_end();
          }
        }
      }
    }
  } else if(lba >= fw_end_sector && lba >= msc_update_start_sector){
    //handle writes to the region where the new firmware will be uploaded
    arduino_firmware_msc_event_data_t p = {0};
    if(msc_update_state <= MSC_UPDATE_STARTING && buffer[0] == 0xE9){
      msc_update_state = MSC_UPDATE_RUNNING;
      msc_update_start_sector = lba;
      msc_update_bytes_written = 0;
      log_v("UPDATE_START: %u (0x%02X)", lba, lba - msc_boot->sectors_per_alloc_table);
      arduino_usb_event_post(ARDUINO_FIRMWARE_MSC_EVENTS, ARDUINO_FIRMWARE_MSC_START_EVENT, &p, sizeof(arduino_firmware_msc_event_data_t), portMAX_DELAY);
      if(msc_update_write(msc_ota_partition, ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset, buffer, bufsize) == ESP_OK){
        log_v("UPDATE_WRITE: %u %u", ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset, bufsize);
        msc_update_bytes_written = ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset + bufsize;
        p.write.offset = ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset;
        p.write.size = bufsize;
        arduino_usb_event_post(ARDUINO_FIRMWARE_MSC_EVENTS, ARDUINO_FIRMWARE_MSC_WRITE_EVENT, &p, sizeof(arduino_firmware_msc_event_data_t), portMAX_DELAY);
      } else {
        msc_update_error();
        return 0;
      }
    } else if(msc_update_state == MSC_UPDATE_RUNNING){
      if(msc_update_entry && msc_update_entry->file_size && msc_update_bytes_written < msc_update_entry->file_size && (msc_update_bytes_written + bufsize) >= msc_update_entry->file_size){
        bufsize = msc_update_entry->file_size - msc_update_bytes_written;
      }
      if(msc_update_write(msc_ota_partition, ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset, buffer, bufsize) == ESP_OK){
        log_v("UPDATE_WRITE: %u %u", ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset, bufsize);
        msc_update_bytes_written = ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset + bufsize;
        p.write.offset = ((lba - msc_update_start_sector) * DISK_SECTOR_SIZE) + offset;
        p.write.size = bufsize;
        arduino_usb_event_post(ARDUINO_FIRMWARE_MSC_EVENTS, ARDUINO_FIRMWARE_MSC_WRITE_EVENT, &p, sizeof(arduino_firmware_msc_event_data_t), portMAX_DELAY);
        if(msc_update_entry && msc_update_entry->file_size && msc_update_bytes_written >= msc_update_entry->file_size){
          msc_update_end();
        }
      } else {
        msc_update_error();
        return 0;
      }
    }
  }
  return bufsize;
}

static int32_t msc_read(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize){
  //log_d("lba: %u, offset: %u, bufsize: %u", lba, offset, bufsize);
  if(lba < fw_start_sector){
    memcpy(buffer, msc_ram_disk + (lba * DISK_SECTOR_SIZE) + offset, bufsize);
  } else if(lba < fw_end_sector){
    //read the currently running firmware
    if(!msc_run_partition || esp_partition_read(msc_run_partition, ((lba - fw_start_sector) * DISK_SECTOR_SIZE) + offset, buffer, bufsize) != ESP_OK){
      return 0;
    }
  } else {
    memset(buffer, 0, bufsize);
  }
  return bufsize;
}

static bool msc_start_stop(uint8_t power_condition, bool start, bool load_eject){
  //log_d("power: %u, start: %u, eject: %u", power_condition, start, load_eject);
  arduino_firmware_msc_event_data_t p = {0};
  p.power.power_condition = power_condition;
  p.power.start = start;
  p.power.load_eject = load_eject;
  arduino_usb_event_post(ARDUINO_FIRMWARE_MSC_EVENTS, ARDUINO_FIRMWARE_MSC_POWER_EVENT, &p, sizeof(arduino_firmware_msc_event_data_t), portMAX_DELAY);
  return true;
}

static volatile TaskHandle_t msc_task_handle = NULL;
static void msc_task(void *pvParameters){
    for (;;) {
      if(msc_update_state == MSC_UPDATE_END){
        delay(100);
        esp_restart();
      }
      delay(100);
    }
    msc_task_handle = NULL;
    vTaskDelete(NULL);
}

FirmwareMSC::FirmwareMSC():msc(){}

FirmwareMSC::~FirmwareMSC(){}

bool FirmwareMSC::begin(){
  msc_update_setup_disk("ESP32S2-FW", 0x0);
  msc.vendorID("ESP32S2");//max 8 chars
  msc.productID("Firmware MSC");//max 16 chars
  msc.productRevision("1.0");//max 4 chars
  msc.onStartStop(msc_start_stop);
  msc.onRead(msc_read);
  msc.onWrite(msc_write);
  msc.mediaPresent(true);
  msc.begin(msc_boot->fat12_sector_num, DISK_SECTOR_SIZE);

  if(!msc_task_handle){
      xTaskCreateUniversal(msc_task, "msc_disk", 1024, NULL, 2, (TaskHandle_t*)&msc_task_handle, 0);
      if(!msc_task_handle){
          return false;
      }
  }
  return true;
}

void FirmwareMSC::onEvent(esp_event_handler_t callback){
    onEvent(ARDUINO_FIRMWARE_MSC_ANY_EVENT, callback);
}
void FirmwareMSC::onEvent(arduino_firmware_msc_event_t event, esp_event_handler_t callback){
    arduino_usb_event_handler_register_with(ARDUINO_FIRMWARE_MSC_EVENTS, event, callback, this);
}

#endif /* CONFIG_USB_MSC_ENABLED */
