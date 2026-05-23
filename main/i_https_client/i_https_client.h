#ifndef _I_HTTPS_CLIENT_H_
#define _I_HTTPS_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" 
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "i_wifi_connect/i_wifi_connect.h"
#include "i_sd_mmc/i_sd_mmc.h"

#include "i_type_def.h"
#include "i_define.h"




typedef struct chunk_payload_t{
    uint8_t *buffer;
    int buffer_index;
} chunk_payload_t;

void fetch_quote();
esp_err_t on_client_handler(esp_http_client_event_t *evt);
extern const uint8_t cert[] asm("_binary_amazon_cer_start");



void init_esp_https(void);

#endif