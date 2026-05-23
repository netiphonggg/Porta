
#include "i_mqtt_client.h"

#define TAG  "* MQTT"

/* get u32 from nvs  */
esp_err_t nvsGet_u32(const char* namespace_name, uint32_t *nvs_value){
    nvs_flash_init();           // no harm to call it again
    nvs_handle_t nvs;
    if (nvs_open(namespace_name, NVS_READWRITE, &nvs) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs open fail: %s", namespace_name); 
        return ESP_FAIL;
    }
    if (nvs_get_u32(nvs, namespace_name, nvs_value) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs get fail: %s", namespace_name);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "nvs get %s: %lu", namespace_name, *nvs_value);
    nvs_close(nvs);
    return ESP_OK;
}

 /* set u32 to nvs  */
esp_err_t nvsSet_u32(const char* namespace_name, uint32_t nvs_value){
    nvs_flash_init();       // no harm to call it again
    nvs_handle_t nvs;
    if (nvs_open(namespace_name, NVS_READWRITE, &nvs) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs open fail: %s", namespace_name); 
        return ESP_FAIL;
    }
    if (nvs_set_u32(nvs, namespace_name, nvs_value) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs set fail: %s", namespace_name);
        return ESP_FAIL;
    }
    nvs_commit(nvs);  // After setting any values, nvs_commit() must be called to ensure changes are written to flash storage
    nvs_close(nvs);
    ESP_LOGI(TAG, "nvs set %s: %lu", namespace_name, nvs_value);
    return ESP_OK;
}

// Returns next calendar day as YYYYMMDD using libc time math
static uint32_t next_day(uint32_t ymd) {
    struct tm t = {0};
    t.tm_year = (ymd / 10000) - 1900;
    t.tm_mon  = ((ymd / 100) % 100) - 1;
    t.tm_mday = (ymd % 100);
    // normalize and add 1 day
    time_t tt = mktime(&t);
    tt += 24*60*60;
    struct tm t2; localtime_r(&tt, &t2);
    return (uint32_t)((t2.tm_year + 1900) * 10000 + (t2.tm_mon + 1) * 100 + t2.tm_mday);
}

void update_cache_from_log(const data1_log_t *data_log){
   if(data_log->device_id >= MAX_DEV) return;
   if(data_log->point_id>= MAX_POINT) return;

   point_state_t *point = &devices[data_log->device_id].points[data_log->point_id];
   point->value = data_log->value;
   point->epoch_s = data_log->epoch_s;
   point->valid = 1;
}

void mqttTask(void *params){
   static data1_log_t buf[128];
   int64_t start, end;
   uint16_t nvsCount = 0, nvsCountMax = 10;
   uint16_t loopInv = 30000;
   s_reply_q = xQueueCreate(1, sizeof(sd_cmd_reply_t));
   uint32_t nvs_readfile = 20260323, nvs_offset = 0;

   vTaskDelay(pdMS_TO_TICKS(60000));  // wait for everything ready
   nvsGet_u32(NVS_SD_READFILE_KEY, &nvs_readfile);
   nvsGet_u32(NVS_SD_INDEX_KEY, &nvs_offset);
   // nvsSet_u32(NVS_SD_READFILE_KEY, nvs_readfile);
   // nvsSet_u32(NVS_SD_INDEX_KEY, nvs_offset);
   sd_cmd_msg_t msg = {.op = SD_CMD_READ_RANGE, .read_file = (uint32_t)nvs_readfile, .reply_q = s_reply_q};

   bool test_CH4 = false;
   uint8_t rotate_CH = 4;

   while (true){

      // wait for WIFI/MQTT up (EventGroup)
      msg.from_offset = nvs_offset;
      msg.max_len = sizeof(buf);
      msg.dst = (uint8_t *)buf;

      // send request to SD task
      if (xQueueSend(q_sd_cmd, &msg, pdMS_TO_TICKS(200)) != pdPASS) {
         // SD task busy/full → backoff
         ESP_LOGW(TAG, "cmd queue full"); 
         vTaskDelay(pdMS_TO_TICKS(100));
         continue;
      }
      
      // wait for reply
      sd_cmd_reply_t rep;
      if (xQueueReceive(s_reply_q, &rep, pdMS_TO_TICKS(10000)) != pdPASS) {
         // SD task didn’t reply in time → handle/telemetry
         ESP_LOGW(TAG, "cmd timeout"); 
         continue;
      }
      if(rep.err == ESP_ERR_OFFSET) {
         ESP_LOGW(TAG, "offset > filesize. Reset");
      }
      nvs_offset = rep.next_offset;     // persist to NVS
      if(rep.end_of_file == 1 || rep.err == ESP_ERR_FILE_NOT_EXIST){
         msg.read_file = next_day(msg.read_file);        // next day
         nvsSet_u32(NVS_SD_READFILE_KEY, msg.read_file);
         nvs_offset = 0;         // reset offset
         ESP_LOGI(TAG, "next day: %lu", msg.read_file);
      }

      if(nvsCount >= nvsCountMax){
         // start = esp_timer_get_time();
         nvsSet_u32(NVS_SD_INDEX_KEY, nvs_offset);
         // end = esp_timer_get_time();
         // printf("set nvs time %.2f ms\n", (end - start) / 1000.0);
         
         pcf8574_write_do_bit(4, test_CH4);
         ESP_LOGI(TAG, "set channel 4: %u", test_CH4);

         rotate_CH++;
         if(rotate_CH > 7) {
            rotate_CH = 4;
            test_CH4 = !test_CH4;
         }

         nvsCount = 0;
      }nvsCount++;

      if(rep.err == ESP_OK){
         ESP_LOGI(TAG, "got reply data, offset: %lu", nvs_offset);

         char buffer[64];
         snprintf(buffer, sizeof(buffer), "%lld", buf[0].epoch_s);
         // start = esp_timer_get_time();
         mqtt_send("egat_hq/t100/dev/msg", buffer, false);    // send mqtt time = <5 ms 
         // end = esp_timer_get_time();
         // printf("sent mqtt time %.2f ms\n", (end - start) / 1000.0);
         for (size_t i = 0; i < 128; i=i+32) {
            printf("mqtt: dev: %u, point %u, %.2f, %llu\n", buf[i].device_id, buf[i].point_id, buf[i].value, buf[i].epoch_s);
         }
      }

      loopInv = rep.is_old_file == 1 ? 5000 : 30000;
      // int stackUnused = uxTaskGetStackHighWaterMark(NULL);
      // ESP_LOGI(TAG, "stack unused = %d", stackUnused);  
      vTaskDelay(pdMS_TO_TICKS(loopInv));   
   } 
}


/* MQTT handler */
/******    event         */
static void mqtt_event_handler(void* event_handler_arg,esp_event_base_t event_base, int32_t event_id, void* event_data){
   esp_mqtt_event_handle_t event = event_data;
   switch((esp_mqtt_event_id_t)event_id){
      case MQTT_EVENT_ANY:
         ESP_LOGI(TAG, "MQTT_EVENT_ANY");
         break;
      case MQTT_EVENT_ERROR:
         ESP_LOGE(TAG, "MQTT_EVENT_ERROR %s", strerror(event->error_handle->esp_transport_sock_errno));
         break;
      case MQTT_EVENT_CONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
         esp_mqtt_client_subscribe(mqttClient, "egat_hq/t100/esp32/cmd/#", 1);     // qos 1
         esp_mqtt_client_subscribe(mqttClient, "egat_hq/t101/esp32/cfg", 1);
         // esp_mqtt_client_subscribe(mqttClient, "egat_hq/t102/esp32", 1);
         // esp_mqtt_client_subscribe(mqttClient, "egat_hq/t103/esp32", 1);
         
         break;
      case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
         break;
      case MQTT_EVENT_SUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
         break;
      case MQTT_EVENT_UNSUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
         break;
      case MQTT_EVENT_PUBLISHED:
         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
         break;
      case MQTT_EVENT_DATA:
         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
         printf("topic: %.*s\n", event->topic_len, event->topic);
         printf("message: %.*s\n", event->data_len, event->data);

         if(strncmp(event->topic, "egat_hq/t100/esp32/cfg", strlen("egat_hq/t100/esp32/cfg")) == 0){  // compare topic 
            printf("got config data %.*s\n", event->data_len, event->data);
         }

         break;
      case MQTT_EVENT_BEFORE_CONNECT:
         ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
         break;
      case MQTT_EVENT_DELETED:
         ESP_LOGI(TAG, "MQTT_EVENT_DELETED");
         break;
      case MQTT_USER_EVENT:
         ESP_LOGI(TAG, "MQTT_USER_EVENT");
         break;
      default:
         break;
   }
}


/******    mqtt send         */
int mqtt_send(const char *topic, const char *payload, bool retain){
   return esp_mqtt_client_publish(mqttClient, topic, payload, strlen(payload), 1, retain);
}


void stop_mqtt_client(void) {
   if (mqttClient == NULL) {
      ESP_LOGW(TAG, "MQTT client is already NULL.");
      return;
   }

    // 1. Optional: Send DISCONNECT packet (Graceful exit)
   //  esp_mqtt_client_disconnect(mqttClient);

    // 2. Stop the internal task and close transport
    esp_err_t stop_err = esp_mqtt_client_stop(mqttClient);
    if (stop_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s", esp_err_to_name(stop_err));
        // Proceed to destroy even if stop failed, to clean resources
    }

    // 3. Destroy the client and free resources
   //  esp_err_t destroy_err = esp_mqtt_client_destroy(mqttClient);
   //  if (destroy_err != ESP_OK) {
   //      ESP_LOGE(TAG, "Failed to destroy MQTT client: %s", esp_err_to_name(destroy_err));
   //  } else {
   //      // Must set the pointer to NULL to prevent crashes on subsequent calls
   //      mqttClient = NULL; 
   //      ESP_LOGI(TAG, "MQTT Client successfully stopped and destroyed.");
   //  }
}

// How to call this function in your code:
// esp_mqtt_client_handle_t my_client = ...; // Your client handle
// safe_mqtt_shutdown(&my_client);


/******    init mqtt client         */
esp_err_t init_mqtt_client(void){
   esp_err_t err;
   esp_mqtt_client_config_t esp_mqtt_client_config = {
   .broker.address.uri = "mqtt://iot-mqtt.egat.co.th:1883",
   .session.last_will = {
         .topic = "egat_hq/t100/dev/lwt",
         .msg = "Disconnected",
         .msg_len = strlen("Disconnected"),
         .qos = 1
   }
   };
   mqttClient = esp_mqtt_client_init(&esp_mqtt_client_config);
   err = esp_mqtt_client_register_event(mqttClient, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register event fail"); 
      return ESP_FAIL;
   }
   err = esp_mqtt_client_start(mqttClient);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Start fail"); 
      return ESP_FAIL;
   }

   char buffer[150];
   char my_mac_str[13];
   snprintf(buffer, sizeof(buffer), "{\"device\":\"esp32\",\"status\":5,\"mac\":\"%s\",\"startup\":\"%lld\"}", mac_to_str(my_mac_str, my_mac), esp_startup_time);
   mqtt_send("egat_hq/t100/dev", buffer, false);    // send mqtt time = <5 ms 

   xTaskCreate(mqttTask, "test read data from queue", 8192, NULL, 6, NULL);

   return ESP_OK;
}

/* ESP print mac address */
char *mac_to_str(char *buffer, uint8_t *mac){
      sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1] ,mac[2], mac[3], mac[4], mac[5]);
      return buffer;
}