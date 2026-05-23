#ifndef _I_DEFINE_H_
#define _I_DEFINE_H_

#include "freertos/FreeRTOS.h"  // l
#include "freertos/task.h"      // l
#include "freertos/semphr.h"    // l
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "i_type_def.h"
#include "esp_modbus_master.h"   // master helpers (descriptor API)
#include "mqtt_client.h"

#define ESP_NO_DATA     10
#define ESP_ERR_FILE_NOT_EXIST  0x63   /*!< Invalid argument */
#define ESP_ERR_OFFSET  0x64

#define I2C_SCHEMA_PATH "/sd/i2c_ischema.bin"
#define I2C_INDEX_PATH  "/sd/i2c_index.bin"
#define MB_CONF_PATH  "/sd/conf/mb.json"

#define NVS_SNTP_KEY  "sntp"
#define NVS_WIFI_KEY  "wifiCreds"
#define NVS_MQTT_KEY  "mqttCreds"
#define NVS_SD_INDEX_KEY  "sdindex"
#define NVS_SD_READFILE_KEY  "sdreadfile"

#define AP_WIFI_SSID  "PORTA1_AP"
#define AP_WIFI_PASS  "12345678"


#define PATH_NAME_SIZE  40
#define MAX_INDEX_SIZE  30       // maximum index (row) for 1 file
#define MAX_FILE_INDEX  5        // maximum file 

#define MAX_FILE_SIZE   1600     //  data_log_t   size = 16 byte -> max file size = 10080*16 = 161280 bytes
#define MAX_UNSEND_DATA 100      //  10080
#define COMBINE_SIZE    10

/***   global variable   ***/

    extern SemaphoreHandle_t binSem_connectWifi;
    extern SemaphoreHandle_t binSem_initWifi;
    extern bool changeToAP;
    
    extern SemaphoreHandle_t sdmmcBus;      // the flag that occupy the bus

    extern QueueHandle_t q_sd_write;    // xQueueCreate(SD_WRITE_QUEUE_LEN, sizeof(data_log_t));
    extern QueueHandle_t q_sd_cmd;      // xQueueCreate(16, sizeof(sd_cmd_msg_t));

    extern device_cfg_t *cfg_list;      // for parsing modbus device 
    extern size_t cfg_count;            
    extern modbus_param_type_t *g_my_param_type;     // for parsing modbus parameter type (modbus library not support)
    extern mb_parameter_descriptor_t *g_params;     // for parsing modbus parameter descriptor
    extern size_t g_params_count;   

    extern uint8_t *g_pool_holding;
    extern size_t   g_pool_holding_size;
    extern uint8_t *g_pool_input;
    extern size_t   g_pool_input_size;
    extern uint8_t *g_pool_coil;
    extern size_t   g_pool_coil_size;
    extern uint8_t *g_pool_discrete;
    extern size_t   g_pool_discrete_size;


    extern time_t esp_startup_time;
    extern uint8_t my_mac[6];

    extern esp_mqtt_client_handle_t mqttClient;

/***   global variable   ***/

#endif