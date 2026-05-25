#include "i_server.h"



#define TAG  "* SERVER"


static const char *BASE_PATH = "/store";
static httpd_handle_t esp_server = NULL;
static httpd_config_t esp_server_config = HTTPD_DEFAULT_CONFIG();

/* url handler */


// static esp_err_t on_url_hit(httpd_req_t *req){
   //  esp_vfs_spiffs_conf_t config = {
   //      .base_path = "/spiffs",
   //      .partition_label = NULL,
   //      .max_files = 5,
   //      .format_if_mount_failed = true};
   //  esp_vfs_spiffs_register(&config);

   //  ESP_LOGI(TAG, "url %s was hit", req->uri);
   //  char path[600];
   //  sprintf(path, "/spiffs%s", req->uri);
   //  if (strcmp(path, "/spiffs/") == 0)
   //  {
   //      sprintf(path, "/spiffs/%s", "index.html");
   //  }
   //  //style.css
   //  char *ptr = strrchr(path, '.');
   //  if (strcmp(ptr, ".css") == 0)
   //  {
   //      ESP_LOGI(TAG, "setting mime type to css");
   //      httpd_resp_set_type(req, "text/css");
   //  }
   //  FILE *file = fopen(path, "r");
   //  if (file == NULL)
   //  {
   //      httpd_resp_send_404(req);
   //      return ESP_OK;
   //  }

   //  char lineRead[256];
   //  while (fgets(lineRead, sizeof(lineRead), file))
   //  {
   //      httpd_resp_sendstr_chunk(req, lineRead);
   //  }
   //  httpd_resp_sendstr_chunk(req, NULL);
   //  esp_vfs_spiffs_unregister(NULL);
//     return ESP_OK;
// }

void resetWifiTask(void *params){
   vTaskDelay(pdMS_TO_TICKS(1000));
   ESP_LOGI(TAG, "free mdns");  
   mdns_free();
   ESP_LOGI(TAG, "stop esp server"); 
   stop_esp_server();
   ESP_LOGI(TAG, "stop mqtt client");
   stop_mqtt_client();
   ESP_LOGI(TAG, "wifi disconnected");  
   wifi_disconnect();
   ESP_LOGI(TAG, "4"); 
   xSemaphoreGive(binSem_initWifi);       // reconnect wifi
   ESP_LOGI(TAG, "5"); 
   
   int stackUnused = uxTaskGetStackHighWaterMark(NULL);
   ESP_LOGI(TAG, "resetWifiTask stack unused = %d", stackUnused);  
   
   vTaskDelete(NULL);
}

/******    porta1-v01.local/         */
static esp_err_t on_default_url(httpd_req_t *req){
   ESP_LOGI(TAG, "URL1: %s", req->uri);    // req->uri → "/api/hello-world" if GET http://porta1-v01.local/api/hello-world
   
   // // If someone hits an /api path that has no handler, return 404 JSON (don’t try to serve a file)
   // if (strncmp(req->uri, "/api", 4) == 0) {
   //    httpd_resp_set_type(req, "application/json");
   //    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "{\"error\":\"no such API endpoint\"}");
   //    return ESP_OK;
   // }
   
   char *ext = strrchr(req->uri, '.');    //  strrchr() look the string from the back
   // define MIME type of file use strcmp() return zero when matched
   if(ext){
      if(strcmp(ext, ".css") == 0) httpd_resp_set_type(req, "text/css");
      else if(strcmp(ext, ".js") == 0) httpd_resp_set_type(req, "text/javascript");
      else if(strcmp(ext, ".svg") == 0) httpd_resp_set_type(req, "image/svg+xml");
      else if(strcmp(ext, ".png") == 0) httpd_resp_set_type(req, "image/png");
      else if(strcmp(ext, ".jpg") == 0) httpd_resp_set_type(req, "image/jpeg");
   }
   
   // Try open requested file
   char path[600];
   sprintf(path, "/store%s", req->uri);
   ESP_LOGI(TAG, "URL2: %s", path);
   FILE *file = fopen(path, "r");
   if(file == NULL){
      // Fallback: single-page app or root → /html/index.html
      file = fopen("/store/index.html", "r");
      ESP_LOGI(TAG, "URL3: %s", path);
      if(file == NULL){
         httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
         return ESP_OK;             // ← IMPORTANT: stop here
      }
      httpd_resp_set_type(req, "text/html"); // Ensure correct content-type for html fallback
   }

   // Stream file
   char buffer[1024];
   int bytes_read = 0;
   while((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0){
      // httpd_resp_send_chunk(req, buffer, bytes_read);
      if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
         fclose(file);
         return ESP_FAIL;
      }
   }
   fclose(file);
   httpd_resp_send_chunk(req, NULL, 0);
   return ESP_OK;
}

/******    porta1-v01.local/api/toggle-led         */
static esp_err_t on_toggle_led_url(httpd_req_t *req){
   char buffer[100];
   memset(&buffer, 0, sizeof(buffer));                          // reset memory buffer to zero also set null terminal
   httpd_req_recv(req, buffer, req->content_len);               // copy http req to buffer

   cJSON *payload = cJSON_Parse(buffer);                        // parse json payload to json object
   cJSON *is_on_json = cJSON_GetObjectItem(payload, "is_on");   // parse json payload to json object
   bool is_on = cJSON_IsTrue(is_on_json);                  // get json bool
   // uint16_t is_on = is_on_json->valueint;                      // get json int
   cJSON_Delete(payload);                                       // delete parent json object -> will delete everything else in side
   printf("toggle_led %u\n", is_on);                              

   httpd_resp_set_status(req, "204 NO CONTENT");                // response http
   httpd_resp_send(req, "OK", 2);                               // send terminator string
   return ESP_OK;
}
   
/******    porta1-v01.local/api/hello-world         */
static esp_err_t on_hello_world_url(httpd_req_t *req){
   httpd_resp_sendstr(req, "hello-world");
   return ESP_OK;
}

/******    porta1-v01.local/api/toggle-led         */
static esp_err_t on_get_data_url(httpd_req_t *req){
   httpd_resp_sendstr(req, "get data");
   return ESP_OK;
}

/******    porta1-v01.local/api/toggle-led         */
static esp_err_t on_get_temp_url(httpd_req_t *req){
   httpd_resp_sendstr(req, "28");
   return ESP_OK;
}


static esp_err_t on_setwifi_set(httpd_req_t *req){
   ESP_LOGI(TAG, "URL: %s", req->uri);   
   char buf[150];
   memset(&buf, 0, sizeof(buf));          // set buffer to zero 
   httpd_req_recv(req, buf, req->content_len);
   
   cJSON *payload = cJSON_Parse(buf);                        // parse json payload to json object
   cJSON *ssid = cJSON_GetObjectItem(payload, "ssid");      // parse json payload to json object
   cJSON *pass = cJSON_GetObjectItem(payload, "pass");      // parse json payload to json object

   nvs_flash_init();       // no harm to call it again
   nvs_handle_t nvs;
   nvs_open(NVS_WIFI_KEY, NVS_READWRITE, &nvs);
   if(cJSON_IsString(ssid) && cJSON_IsString(pass)){
      nvs_set_str(nvs, "ssid", ssid->valuestring);
      nvs_set_str(nvs, "pass", pass->valuestring);
      printf("ssid: %s pass: %s\n", ssid->valuestring, pass->valuestring);
   }
   nvs_close(nvs);

   /* redirect to finish page */
   httpd_resp_set_status(req, "200");
   // httpd_resp_set_hdr(req, "Location", "/wifi-set.html");
   httpd_resp_send(req, NULL, 0);

   xTaskCreate(resetWifiTask, "reset wifi", 1024 * 2, NULL, 15, NULL);
   return ESP_OK;
}
static esp_err_t on_getwifi(httpd_req_t *req){
   nvs_flash_init();       // no harm to call it again
   nvs_handle_t nvs;
   nvs_open(NVS_WIFI_KEY, NVS_READWRITE, &nvs);
   size_t ssidLen, passLen;
   char *ssid = NULL, *pass = NULL;
   if (nvs_get_str(nvs, "ssid", NULL, &ssidLen) == ESP_OK){
      if (ssidLen > 0) {
         ssid = malloc(ssidLen);
         nvs_get_str(nvs, "ssid", ssid, &ssidLen);
      }
   }
   if (nvs_get_str(nvs, "pass", NULL, &passLen) == ESP_OK){
      if (passLen > 0) {
         pass = malloc(passLen);
         nvs_get_str(nvs, "pass", pass, &passLen);
      }
   }   
   nvs_close(nvs);
   ESP_LOGI(TAG, "Get ssid: %s, pass: %s", ssid ? ssid : "(null)", pass ? pass : "(null)");

   const size_t MAX_JSON_LEN = 128; 
   char *json_response = (char*) malloc(MAX_JSON_LEN);

   const char *final_ssid = ssid ? ssid : "";
   const char *final_pass = pass ? pass : "";

   snprintf(json_response, MAX_JSON_LEN, "{\"ssid\":\"%s\", \"pass\":\"%s\"}", final_ssid, final_pass);

   // --- 2. Set HTTP Response Headers ---
   httpd_resp_set_status(req, "200");
   httpd_resp_set_type(req, "application/json");
   httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);

   // --- 4. Cleanup Resources ---
   if (ssid) free(ssid);
   if (pass) free(pass);
   if (json_response) free(json_response);

   return ESP_OK;
}

// ===== api set mqtt config ======
static esp_err_t on_setmqtt_set(httpd_req_t *req){
   ESP_LOGI(TAG, "URL: %s", req->uri);
   int len = req->content_len;
   if(len <= 0 || len > 1024){
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
      return ESP_OK;
   }
   char *buf = calloc(1, len + 1);  
   httpd_req_recv(req, buf, len);

   cJSON *payload = cJSON_Parse(buf);                        // parse json payload to json object
   free(buf);
   if(!payload){
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
      return ESP_OK;
   }
   cJSON *host = cJSON_GetObjectItem(payload, "host");      // parse json payload to json object
   cJSON *port = cJSON_GetObjectItem(payload, "port");      // parse json payload to json object
   cJSON *client_id = cJSON_GetObjectItem(payload, "client_id");      // parse json payload to json object
   cJSON *username = cJSON_GetObjectItem(payload, "username");      // parse json payload to json object 
   cJSON *password = cJSON_GetObjectItem(payload, "password");      // parse json payload to json object
   
   nvs_flash_init();       // no harm to call it again
   nvs_handle_t nvs;
   if(nvs_open(NVS_MQTT_KEY, NVS_READWRITE, &nvs) == ESP_OK){
      if(cJSON_IsString(host)){
         nvs_set_str(nvs, "host", host->valuestring);
         printf("mqtt host: %s\n", host->valuestring);
      }
      if(cJSON_IsNumber(port)){
         nvs_set_u16(nvs, "port", (uint16_t)(port->valueint));
         printf("mqtt port: %d\n", port->valueint);
      }
      if(cJSON_IsString(client_id)){
         nvs_set_str(nvs, "client_id", client_id->valuestring);
         printf("mqtt client_id: %s\n", client_id->valuestring);
      }
      if(cJSON_IsString(username)){
         nvs_set_str(nvs, "username", username->valuestring);
         printf("mqtt username: %s\n", username->valuestring);
      }
      if(cJSON_IsString(password)){
         nvs_set_str(nvs, "password", password->valuestring);
         printf("mqtt password: %s\n", password->valuestring);
      }
      nvs_commit(nvs);
      nvs_close(nvs);
   }
   cJSON_Delete(payload);

   httpd_resp_set_status(req, "200");
   httpd_resp_send(req, NULL, 0);
   return ESP_OK;
}

// ===== api get mqtt config ======
static esp_err_t on_getmqtt(httpd_req_t *req){
   nvs_flash_init();
   nvs_handle_t nvs;
   nvs_open(NVS_MQTT_KEY, NVS_READONLY, &nvs);
   
   char host[64] = {0};
   char client_id[64] = {0};
   char username[64] = {0};
   char password[64] = {0};
   size_t l;
   l = sizeof(host);
   nvs_get_str(nvs, "host", host, &l);
   l = sizeof(client_id);
   nvs_get_str(nvs, "client_id", client_id, &l);
   l = sizeof(username);
   nvs_get_str(nvs, "username", username, &l);
   l = sizeof(password);
   nvs_get_str(nvs, "password", password, &l);
   uint16_t port=1883;
   nvs_get_u16(nvs, "port", &port);
   nvs_close(nvs);

   char json_response[512];
   snprintf (json_response, sizeof(json_response), "{\"host\":\"%s\", \"port\":%d, \"client_id\":\"%s\", \"username\":\"%s\", \"password\":\"%s\"}", 
      host, port, client_id, username, password);

   httpd_resp_set_type(req, "application/json");
   httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
   return ESP_OK;
}
/******    mDNS         */
esp_err_t start_mdns_service(void){
   esp_err_t err;
   err = mdns_init();
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "mdns init fail"); 
      return ESP_FAIL;
   }
   err = mdns_hostname_set("porta1-v01");
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "mdns set hostname fail"); 
      return ESP_FAIL;
   }
   err = mdns_instance_name_set("LEARN esp32 thing");
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "mdns set ins name fail"); 
      return ESP_FAIL;
   }
   // mdns_free();  // Stop and free mDNS server
   return ESP_OK;
}

/* fat read only */
void mount_fat_ro(void){
   esp_vfs_fat_mount_config_t esp_vfs_fat_mount_config = {
      .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
      .max_files = 5,
      .format_if_mount_failed = true,
   };

   esp_vfs_fat_spiflash_mount_ro(BASE_PATH, "storage", &esp_vfs_fat_mount_config);

   if(isDirExist(BASE_PATH) == ESP_OK){
      printf("dir exist %s\n", BASE_PATH);

    }else{
      ESP_LOGE(TAG, "opendir(/store) failed: %d (%s)", errno, strerror(errno)); return;
    }

}

/******    init server  station mode       */
esp_err_t init_esp_server(void){
   esp_err_t err;
   
   esp_server_config.uri_match_fn = httpd_uri_match_wildcard;
   esp_server_config.max_uri_handlers = 16;   // default is 8, increase it if you have more handlers to register

   err = httpd_start(&esp_server, &esp_server_config);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Server start fail"); 
      return ESP_FAIL;
   }

   httpd_uri_t toggle_led_url = {
      .uri = "/api/toggle-led",
      .method = HTTP_POST,
      .handler = on_toggle_led_url
   };
   err = httpd_register_uri_handler(esp_server, &toggle_led_url);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }

   httpd_uri_t get_temp_url = {
      .uri = "/api/temperature",
      .method = HTTP_POST,
      .handler = on_get_temp_url
   };
   err = httpd_register_uri_handler(esp_server, &get_temp_url);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }
   
   httpd_uri_t hello_world_url = {
      .uri = "/api/hello-world",
      .method = HTTP_GET,
      .handler = on_hello_world_url
   };
   err = httpd_register_uri_handler(esp_server, &hello_world_url);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }

   httpd_uri_t get_data_url = {
      .uri = "/api/get-data",
      .method = HTTP_GET,
      .handler = on_get_data_url
   };
   err = httpd_register_uri_handler(esp_server, &get_data_url);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }

   httpd_uri_t setwifi_end_point_config = {
      .uri = "/api/setwifi",
      .method = HTTP_POST,
      .handler = on_setwifi_set
   };
   err = httpd_register_uri_handler(esp_server, &setwifi_end_point_config);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }

   httpd_uri_t getwifi_end_point_config = {
      .uri = "/api/getwifi",
      .method = HTTP_GET,
      .handler = on_getwifi
   };
   err = httpd_register_uri_handler(esp_server, &getwifi_end_point_config);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }

   // ===== mqtt config api =====
   httpd_uri_t setmqtt_end_point_config = {
      .uri = "/api/setmqtt",
      .method = HTTP_POST,
      .handler = on_setmqtt_set
   };
   httpd_register_uri_handler(esp_server, &setmqtt_end_point_config);

   httpd_uri_t getmqtt_end_point_config = {
      .uri = "/api/getmqtt",
      .method = HTTP_GET,
      .handler = on_getmqtt
   };
   httpd_register_uri_handler(esp_server, &getmqtt_end_point_config);
   
   /***  allways move default url to the last */
   httpd_uri_t default_url = {    // move wildcard url /* to the last to let server check rout before first 
      .uri = "/*",
      .method = HTTP_GET,
      .handler = on_default_url
   };
   err = httpd_register_uri_handler(esp_server, &default_url);
   if (err != ESP_OK) { 
      ESP_LOGW(TAG, "Register uri fail"); 
      return ESP_FAIL;
   }

   return ESP_OK;
}




/******    init server access point mode       */
void init_esp_server_ap(void){
   // httpd_handle_t esp_server_ap = NULL;
   // httpd_config_t esp_server_ap_config = HTTPD_DEFAULT_CONFIG();

   // esp_server_ap_config.uri_match_fn = httpd_uri_match_wildcard;

   // ESP_ERROR_CHECK(httpd_start(&esp_server_ap, &esp_server_ap_config));

   // httpd_uri_t setwifi_end_point_config = {
   //    .uri = "/api/setwifi",
   //    .method = HTTP_POST,
   //    .handler = on_setwifi_set};
   // httpd_register_uri_handler(esp_server_ap, &setwifi_end_point_config);

   // /***  allways move default url to the last */
   // httpd_uri_t default_url = {    // move wildcard url /* to the last to let server_ap check rout before first 
   //    .uri = "/*",
   //    .method = HTTP_GET,
   //    .handler = on_default_url
   // };
   // httpd_register_uri_handler(esp_server_ap, &default_url); 
}


/******    stop server         */
void stop_esp_server(void){
   if(httpd_stop(esp_server) != ESP_OK){
      ESP_LOGI(TAG, "server stop fail");
        // return;
   }
}
