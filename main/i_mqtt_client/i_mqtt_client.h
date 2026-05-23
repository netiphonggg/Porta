#ifndef _I_MQTT_CLIENT_H_      //prevent call more than one
#define _I_MQTT_CLIENT_H_

#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <unistd.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mqtt_client.h"


#include "i_i2c/i_i2c.h"
#include "i_type_def.h"
#include "i_define.h"

/* data point cache for mqtt*/
#define MAX_DEV    5       // maximum device 
#define MAX_POINT  50      // maximum point 
typedef struct {
    time_t   epoch_s;      // 0 until SNTP synced
    float    value;        // engineering units (or store scaled int if you prefer)
    uint8_t  valid;           
} point_state_t;            
typedef struct {
    point_state_t points[MAX_POINT];        
} dev_state_t;              
static dev_state_t devices[MAX_DEV];

static QueueHandle_t s_reply_q;

// const char* deviceStatusTP = "{\"device\":\"esp32\",\"status\":5,\"mac\":\"%s\",\"startup\":\"%lld\"}";

int mqtt_send(const char *topic, const char *payload, bool retain);
esp_err_t init_mqtt_client(void);
void stop_mqtt_client(void);
char *mac_to_str(char *buffer, uint8_t *mac);

#endif