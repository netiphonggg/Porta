#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"  // l
#include "freertos/task.h"      // l
#include "freertos/semphr.h"    // l
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "esp_system.h"         // l
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_timer.h"          // l
#include "esp_log.h"
#include "esp_types.h"
#include "esp_heap_caps.h"                  // memory size
#include "driver/gpio.h"
// #include "driver/dac.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/i2c_master.h"
#include "driver/touch_pad.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_sntp.h"

#include "esp_http_server.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "math.h" 

#include "mdns.h"

#include "i_modbus/i_modbus.h"
#include "i_server/i_server.h"
#include "i_wifi_connect/i_wifi_connect.h"
#include "i_mqtt_client/i_mqtt_client.h"
#include "i_https_client/i_https_client.h"
#include "i_sd_mmc/i_sd_mmc.h"
#include "i_i2c/i_i2c.h"

#include "parse_json.h"

#include "i_type_def.h"
#include "i_define.h"

#define SD_WRITE_QUEUE_LEN   1024

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;

/*****   Define data  *****/
uint16_t reconnectWifiCounter;

void OnConnectedTask(void *para);
char *create_body();
void on_timer100ms(TimerHandle_t xTimer);        // init timer function
void on_timer1s(TimerHandle_t xTimer);        // init timer function
char *mac_to_str(char *buffer, uint8_t *mac);
void test_send_messages(void *param);


#endif