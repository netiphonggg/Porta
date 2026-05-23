#ifndef _I_SERVER_H_
#define _I_SERVER_H_

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "mdns.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_http_server.h"

#include "i_define.h"

#include "i_sd_mmc/i_sd_mmc.h"
#include "../i_wifi_connect/i_wifi_connect.h"

esp_err_t start_mdns_service(void);
void mount_fat_ro(void);
esp_err_t init_esp_server(void);
void stop_esp_server(void);
void resetWifiTask(void *params);

#endif