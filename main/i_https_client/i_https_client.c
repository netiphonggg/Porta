#include "i_https_client.h"

#define TAG "* HTTPS"

void httpsTask(void *params){
   data_log_t data_log[5];
   esp_err_t err;
   int64_t start_read, end_read;
   uint8_t id = 0, idMax = 4;
   uint8_t wFile, rFile;
   int16_t indexDiff, indexDiffMax;
   uint8_t readSize = 5;

   while (true){
      if(xSemaphoreTake(sdmmcBus, 1000/portTICK_PERIOD_MS)){
         if(wifi_status()){
         //    wFile = indexData[id].file & 0x0F;
         //    rFile = indexData[id].file >> 4;
         //    indexDiff = indexData[id].wIndex-indexData[id].rIndex;
         //    indexDiffMax = MAX_INDEX_SIZE-indexData[id].rIndex;
         //    if(((wFile != rFile) && indexDiffMax >= 5) || ((wFile == rFile) && indexDiff >= 5)){
         //       // read data log
               
               
         //       char path[PATH_NAME_SIZE];
         //       snprintf(path, PATH_NAME_SIZE, "/sd/%s/%u.bin", i2cSchema[id].key, rFile);
         //       start_read = esp_timer_get_time();
         //       err = read_data_log_str(path, data_log, id, readSize);
         //       if(err == ESP_OK){
         //          //  do task send data to HTTP or MQTT
         //          //   fetch_quote();
                     
         //          //  if success than update rIndex
         //          indexData[id].rIndex += readSize;

         //          if(indexData[id].rIndex >= MAX_INDEX_SIZE){
         //             indexData[id].rIndex = 0;
         //             char path1[PATH_NAME_SIZE];
         //             snprintf(path1, PATH_NAME_SIZE, "/sd/%s/%u.bin", i2cSchema[id].key, rFile);
         //             remove_file(path1);
         //             rFile++; if(rFile >= MAX_FILE_INDEX) rFile = 0;
         //             indexData[id].file = (rFile << 4) | wFile;
         //          }
         //          // update_rIndex_str(I2C_INDEX_PATH, &_index, 0);
                  
         //          end_read = esp_timer_get_time();
         //          printf("data0 time %lldus, value: %3.2f, ts: %lld\n", end_read-start_read, data_log[0].value, data_log[0].ts);
         //          printf("data1 time %lldus, value: %3.2f, ts: %lld\n", end_read-start_read, data_log[1].value, data_log[1].ts);
         //          printf("data2 time %lldus, value: %3.2f, ts: %lld\n", end_read-start_read, data_log[2].value, data_log[2].ts);
         //          printf("data3 time %lldus, value: %3.2f, ts: %lld\n", end_read-start_read, data_log[3].value, data_log[3].ts);
         //          printf("data4 time %lldus, value: %3.2f, ts: %lld\n", end_read-start_read, data_log[4].value, data_log[4].ts);
         //       }
         //    }
         //    else{
         //       // ESP_LOGI(TAG, "no data to read");  
         //    }
         }

         if(++id >= idMax) id = 0;

         // if(write_allIndex_str(I2C_INDEX_PATH, indexData, 4) == ESP_OK){
         //     ESP_LOGI(TAG, "test write index");  
         // }

         //  int stackUnused = uxTaskGetStackHighWaterMark(NULL);
         //  ESP_LOGI(TAG, "stack unused = %d", stackUnused);  

         xSemaphoreGive(sdmmcBus);                               // give the bus to another task use 
      }else{
         ESP_LOGW(TAG, "sdmmcBus timeout ");
      }
      vTaskDelay(pdMS_TO_TICKS(20000)); 
   } 

}

void init_esp_https(void){
    xTaskCreate(httpsTask, "https", 3072, NULL, 4, NULL);
}


/* https */
char *create_body(){
   cJSON *json_payload = cJSON_CreateObject();
   cJSON_AddStringToObject(json_payload, "data", "test data");
   
   char * payload_body = cJSON_Print(json_payload);
   printf("body: %s\n", payload_body);
   cJSON_Delete(json_payload);
   return payload_body;
}

void fetch_quote(){
   chunk_payload_t chunk_payload = {0};         // {0} => short hand for memset to clear data
   esp_http_client_config_t esp_http_client_config = {
      .url = "https://weatherapi-com.p.rapidapi.com/forecast.json?q=Bangkok&days=1",
      .method = HTTP_METHOD_GET,
      .event_handler = on_client_handler,
      .user_data = &chunk_payload,
      .cert_pem = (char *)cert,
   };
   esp_http_client_handle_t client = esp_http_client_init(&esp_http_client_config);
   esp_http_client_set_header(client, "Content-Type", "application/json");
   esp_http_client_set_header(client, "x-rapidapi-host", "weatherapi-com.p.rapidapi.com");
   esp_http_client_set_header(client, "x-rapidapi-key", "807b07efbdmsh8cdbf11c6e8646ap129481jsncaddf2cf1435");
   char *payload_body = create_body();
   esp_http_client_set_post_field(client, payload_body, strlen(payload_body));

   esp_err_t err = esp_http_client_perform(client);
   if(err == ESP_OK){
      ESP_LOGI(TAG, "HTTP GET status = %d", esp_http_client_get_status_code(client));
      // parse_json((char *)chunk_payload.buffer);
   }else{
      ESP_LOGE(TAG, "HTTP GET request failed = %s", esp_err_to_name(err));
   }
   if(chunk_payload.buffer != NULL){   // clear memory
      free(chunk_payload.buffer);
   }

   if(payload_body != NULL){
      free(payload_body);
   }
   esp_http_client_cleanup(client);
}

esp_err_t on_client_handler(esp_http_client_event_t *evt){
   switch (evt->event_id){
   case HTTP_EVENT_ON_DATA:
   {
      // ESP_LOGI(TAG_WIFI, "Length = %d", evt->data_len);
      // printf("%.*s\n", evt->data_len , (char *)evt->data);
      chunk_payload_t *chunk_payload = evt->user_data;                  // point to user data
      chunk_payload->buffer = realloc(chunk_payload->buffer, chunk_payload->buffer_index + evt->data_len + 1);    // +1 => terminator = 1 byte
      memcpy(&chunk_payload->buffer[chunk_payload->buffer_index], (char *)evt->data, evt->data_len);
      chunk_payload->buffer_index = chunk_payload->buffer_index + evt->data_len;
      chunk_payload->buffer[chunk_payload->buffer_index] = '\0';                             // set terminator to null 
      // printf("****buffer***** %s\n", chunk_payload->buffer);
      break;
   }
   default:
      break;
   }
   return ESP_OK;
}