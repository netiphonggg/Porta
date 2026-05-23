#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include "esp_heap_caps.h"                  // memory size

#include "esp_log.h"
#include "parse_json.h"

static const char *TAG = "PARSE JSON";

esp_err_t parse_json(char *weatherStr){
    int dram = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);   // check heap size
    printf("dram %d\n", dram);
    
    cJSON *weather_json = cJSON_Parse(weatherStr);
    if(weather_json == NULL){
        const char *err = cJSON_GetErrorPtr();
        if(err){
            ESP_LOGE(TAG, "Error parsing json before %s", err);
            return -1;
        }
    }
    cJSON *location = cJSON_GetObjectItemCaseSensitive(weather_json, "location");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(location, "name");
    // printf("location %s\n", location->valuestring);
    printf("name %s\n", name->valuestring);
    
    return ESP_OK;
}