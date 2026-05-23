#ifndef _I_SD_MMC_H_
#define _I_SD_MMC_H_

#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <unistd.h>

#include "cJSON.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_modbus_master.h"   // master helpers (descriptor API)

#include "i_wifi_connect/i_wifi_connect.h"
#include "i_mqtt_client/i_mqtt_client.h"

#include "i_type_def.h"
#include "i_define.h"


#define BATCH_MAX_RECORDS   64            // record size = 24 Bytes, total buffer size = 24 Bytes * 128 record = 3072 Bytes
#define FLUSH_EVERY_MS      50            // flush every 50 ms  
#define FSYNC_EVERY_MS      30000         // sync data (commit) every 300 ms
#define SDMMC_SLOT_CD_PIN   GPIO_NUM_27   // set card detect pin - card detect, low = inserted

static uint32_t s_write_offset = 0;       // persist in NVS periodically

bool sdmmc_status(void);
esp_err_t init_esp_sd_mmc(void);
void read_file(const char *path);
void write_file(const char *path, char *content);

esp_err_t read_i2c_schema_str(const char *path, i2c_schema_t *_schema, uint8_t readSize);
esp_err_t write_i2c_schema_str(const char *path, i2c_schema_t *_schema, uint8_t updateSize);

esp_err_t read_data_log_str(const char *path, data_log_t *_data, uint8_t _id, uint8_t readSize);
esp_err_t write_data_log_str(const char *path, const data_log_t *_data, uint8_t _id);
esp_err_t append_data_log_str(const char *path, const data_log_t *_data);
esp_err_t read_combine_data_log_str(const char *path, data_log_t *_dataCombined, uint8_t _id);


esp_err_t read_allIndex_str(const char *path, index_t *_index, uint8_t readSize);
esp_err_t write_allIndex_str(const char *path, index_t *_index, uint8_t updateSize);
esp_err_t clear_index_str(const char *path, uint8_t _id);
// esp_err_t read_rIndex_str(const char *path, index_data_t *_index, uint8_t _id);
// esp_err_t update_rIndex_str(const char *path, index_data_t *_index, uint8_t _id);
// esp_err_t update_wIndex_str(const char *path, uint8_t _id);

esp_err_t logging_str(const char *path, const data_log_t *_data, uint8_t _id);

uint16_t getFileSize(const char *path);
esp_err_t remove_file(const char *path);
bool makeDir(const char *path);
esp_err_t isDirExist(const char *path);

#endif

/*

Opening Modes	Description
    r	        Searches file. If the file is opened successfully fopen( ) loads it into memory and sets up a pointer that points to the first character in it. If the file cannot be opened fopen( ) returns NULL.
    rb	        Open for reading in binary mode. If the file does not exist, fopen( ) returns NULL.
    w	        Open for writing in text mode. If the file exists, its contents are overwritten. If the file doesn’t exist, a new file is created. Returns NULL, if unable to open the file.
    wb	        Open for writing in binary mode. If the file exists, its contents are overwritten. If the file does not exist, it will be created.
    a	        Searches file. If the file is opened successfully fopen( ) loads it into memory and sets up a pointer that points to the last character in it. It opens only in the append mode. If the file doesn’t exist, a new file is created. Returns NULL, if unable to open the file.
    ab	        Open for append in binary mode. Data is added to the end of the file. If the file does not exist, it will be created.
    r+	        Searches file. It is opened successfully fopen( ) loads it into memory and sets up a pointer that points to the first character in it. Returns NULL, if unable to open the file.
    rb+	        Open for both reading and writing in binary mode. If the file does not exist, fopen( ) returns NULL.
    w+	        Searches file. If the file exists, its contents are overwritten. If the file doesn’t exist a new file is created. Returns NULL, if unable to open the file.
    wb+	        Open for both reading and writing in binary mode. If the file exists, its contents are overwritten. If the file does not exist, it will be created.
    a+	        Searches file. If the file is opened successfully fopen( ) loads it into memory and sets up a pointer that points to the last character in it. It opens the file in both reading and append mode. If the file doesn’t exist, a new file is created. Returns NULL, if unable to open the file.
    ab+	        Open for both reading and appending in binary mode. If the file does not exist, it will be created.
*/
