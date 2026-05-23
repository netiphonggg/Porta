#ifndef _I_WIFI_CONNECT_H_      //prevent call more than one
#define _I_WIFI_CONNECT_H_

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"      
#include "freertos/event_groups.h"
#include "freertos/semphr.h"    
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_sntp.h"
#include "esp_timer.h"

#include "i_define.h"

esp_err_t wifi_get_config(void);
esp_err_t wifi_connect_init(void);
esp_err_t wifi_connect_sta(char * ssid, char *pass, int timeout);
esp_err_t wifi_connect_ap(const char * ssid, const char *pass);
void wifi_disconnect(void);
bool wifi_status(void);
bool wifi_sta_status(void);
int wifi_sta_get_rssi(void);
esp_err_t set_time_to_nvs(time_t tv_sec);
esp_err_t set_sntp_from_nvs(void);
bool sntp_sync_status(void);
void print_current_time(void);
wifi_mode_t wifi_mode(void);
void wifiInitTask(void *params);


#endif