#include "main.h"

#define TAG_INFO  "* MAIN"
#define TAG_ADC  "* ADC"

// #define BTN0 0
#define PIN_SW1         GPIO_NUM_35
#define PIN_SW2         GPIO_NUM_34
#define PIN_PLOSS       GPIO_NUM_22
#define PIN_SD_DETECT   GPIO_NUM_27
#define PIN_RELEASE  0x0001
#define PIN_CHECK    0x0007

/***   initialize global variable   ***/
   SemaphoreHandle_t binSem_connectWifi;
   SemaphoreHandle_t binSem_initWifi;
   bool changeToAP = false;
   
   SemaphoreHandle_t sdmmcBus;

   QueueHandle_t q_sd_write;
   QueueHandle_t q_sd_cmd;
   
   device_cfg_t *cfg_list = NULL;
   size_t cfg_count = 0;
   modbus_param_type_t *g_my_param_type = NULL;     // for parsing modbus parameter type (modbus library not support)
   mb_parameter_descriptor_t *g_params = NULL;
   size_t g_params_count = 0;

   uint8_t *g_pool_holding  = NULL;
   size_t   g_pool_holding_size = 0;
   uint8_t *g_pool_input    = NULL;
   size_t   g_pool_input_size   = 0;
   uint8_t *g_pool_coil     = NULL;
   size_t   g_pool_coil_size    = 0;
   uint8_t *g_pool_discrete = NULL;
   size_t   g_pool_discrete_size= 0;
   
   time_t esp_startup_time = 0;
   uint8_t my_mac[6] = {0};

   esp_mqtt_client_handle_t mqttClient;


/***   initialize variable   ***/
uint16_t sw2Push3s, sw1Check, sw2Check;
bool sw1Push, sw2Push;
time_t currentTime = 0;

uint16_t checkInterval;


TaskHandle_t ploss_receiverHandler = NULL;
TaskHandle_t sddetect_receiverHandler = NULL;

/*  Power loss handler task to disable to Wifi, MQTT, HTTPS */
void powerLossTask(void * params){  
   while(true){
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // pdfTRUE use for set when notify many time to notify only last notify
      printf("power loss isr\n");
   }
}
static void IRAM_ATTR power_loss_isr_handler(void *args){  // IRAM_ATTR = tell complier to use DRAM for interrupt
   // int pinNumber = (int) args;
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(ploss_receiverHandler, &xHigherPriorityTaskWoken);  // Notify the task that the button was pressed
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);   // If the ISR woke up a higher-priority task, yield immediately
}

/*  sd detech handler task to disable to Wifi, MQTT, HTTPS */
void sdDetectTask(void * params){  
   while(true){
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // pdfTRUE use for set when notify many time to notify only last notify
      printf("sd detect isr\n");
   }
}
static void IRAM_ATTR sd_detech_isr_handler(void *args){  // IRAM_ATTR = tell complier to use DRAM for interrupt
   // int pinNumber = (int) args;
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(sddetect_receiverHandler, &xHigherPriorityTaskWoken);  // Notify the task that the button was pressed
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);   // If the ISR woke up a higher-priority task, yield immediately
}

static bool init_adc_calibration(adc_unit_t unit){
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_ADC, "ADC calibration enabled");
        return true;
    } else {
        ESP_LOGW(TAG_ADC, "ADC calibration NOT available");
        return false;
    }
}

// init GPIO 
static void init_gpio(void){

   /* Digital */
   /* Digital input  */
   gpio_config_t inputOnlyConfig = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_INPUT,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE,    // for input only pin not have internall pull-up
      .pin_bit_mask = (1ULL<<PIN_SW2) | (1ULL<<PIN_SW1)  //  1ULL = 0x00000001
   };  gpio_config(&inputOnlyConfig);

   /* Digital input interrupt */
   gpio_config_t inputOnlyNegISRConfig = {
      .intr_type = GPIO_INTR_NEGEDGE,
      .mode = GPIO_MODE_INPUT,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE,    // for input only pin not have internall pull-up
      .pin_bit_mask = (1ULL<<PIN_PLOSS)  //  1ULL = 0x00000001
   };  gpio_config(&inputOnlyNegISRConfig);
   
   gpio_config_t inputOnlyPosISRConfig = {
      .intr_type = GPIO_INTR_POSEDGE,
      .mode = GPIO_MODE_INPUT,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE,    // for input only pin not have internall pull-up
      .pin_bit_mask = (1ULL<<PIN_SD_DETECT)  //  1ULL = 0x00000001
   };  gpio_config(&inputOnlyPosISRConfig);

   gpio_install_isr_service(0);
   gpio_isr_handler_add(PIN_PLOSS, power_loss_isr_handler, NULL);
   gpio_isr_handler_add(PIN_SD_DETECT, sd_detech_isr_handler, NULL);

   /* Analog input */
   /*  1) Create ADC One-Shot unit -----------------------------*/
   adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
   };
   adc_oneshot_new_unit(&init_config1, &adc1_handle);
   
   /*  2) Configure channel -----------------------------*/
   adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_12,      // 0–4095
      .atten    = ADC_ATTEN_DB_12,      // up to 3.3–3.6V
   };
   adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config);
   adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_5, &config);
   /*  3) Try enable calibration -----------------------------*/
   bool cali_enabled = init_adc_calibration(ADC_UNIT_1);
   /*  4) read adc channel 4, 5 -----------------------------*/
   int raw4 = 0, raw5 = 0;
   int mv4 = 0, mv5 = 0;
   adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw4);
   adc_oneshot_read(adc1_handle, ADC_CHANNEL_5, &raw5);
   if (cali_enabled) {
      adc_cali_raw_to_voltage(adc1_cali_handle, raw4, &mv4);
      adc_cali_raw_to_voltage(adc1_cali_handle, raw5, &mv5);
      ESP_LOGI(TAG_ADC, "ADC4_Temp: %d, Volt: %d mV | ADC5_Vin: %d, Volt: %d mV", raw4, mv4, raw5, mv5);
   }
}

void app_main(void){

   /* Semaphore */   
      binSem_connectWifi = xSemaphoreCreateBinary();
      binSem_initWifi = xSemaphoreCreateBinary();
      
      sdmmcBus = xSemaphoreCreateMutex();
   
   /* create queue */ 
      q_sd_write = xQueueCreate(SD_WRITE_QUEUE_LEN, sizeof(data1_log_t));
      q_sd_cmd   = xQueueCreate(16, sizeof(sd_cmd_msg_t));

   /* esp32 mac address */
      esp_read_mac(my_mac, ESP_MAC_EFUSE_FACTORY);
      char my_mac_str[13];
      ESP_LOGI(TAG_INFO, "my mac %s", mac_to_str(my_mac_str, my_mac));

   /*****  GPIO *****/
   //  ESP_ERROR_CHECK(nvs_flash_init());  // use for wifi connection
      init_gpio();

      /*****  GPIO isr tesk *****/
      xTaskCreate(powerLossTask, "PowerLossTask", 2048, NULL, 10, &ploss_receiverHandler);
      xTaskCreate(sdDetectTask, "SDDetechTask", 2048, NULL, 10, &sddetect_receiverHandler);

   /***** i2c ******/  
      init_esp_i2c();
      
   /***** RTC SNTP *****/

   /*****  sd mmc  *****/
      init_esp_sd_mmc();
    
      // for(uint8_t i = 0; i < 4; i++){
      //    clear_index_str(I2C_INDEX_PATH, i);
      //    char buffer[30];
      //    snprintf(buffer, 30, "%s/0.bin", keyName[i]);
      //    remove_file(buffer);
      // }

      // if(read_allIndex_str(I2C_INDEX_PATH, indexData, 4) == ESP_OK){
      //    for (uint8_t i=0; i<4; i++){
      //       printf("Index ID: %u, wI: %u, wF: %u, rI: %u, wF: %u\n", i, indexData[i].wIndex, indexData[i].file & 0x0F, indexData[i].rIndex, indexData[i].file >> 4);
      //    }
      // }

      // if(write_i2c_schema_str(I2C_SCHEMA_PATH, i2cSchema, 4) == ESP_OK){
      //    ESP_LOGI("write i2c schema done");
      // }
      //    read_i2c_schema_str(const char *path, i2c_schema_t *_schema, uint8_t readSize);


      // DMA_ATTR static char long_text[1024];
      // memset(&long_text, 'c', 1023);
      // int64_t start_write = esp_timer_get_time();             // start timer
      // write_file("/log/text.txt", long_text);
      // int64_t start_read = esp_timer_get_time(); 
      // read_file("/log/text.txt");
      // int64_t end_read = esp_timer_get_time(); 
      // printf("write time %lldus, read time %lldus\n", start_read - start_write, end_read - start_read);

   /*****  mount fat storage  *****/
      mount_fat_ro();         /* mount fat file read only */
      
   /* connect wifi */
      xTaskCreate(wifiInitTask, "init comms", 3072, NULL, 10, NULL);
      xTaskCreate(OnConnectedTask, "handle comms", 5120, NULL, 5, NULL);
      xSemaphoreGive(binSem_initWifi);


   /***** modbus ******/ 
      init_esp_modbus();

   /*****  REST API post request *****/
      // init_esp_https();

   
   /***** FreeRTOS timer 100ms ******/  
      printf("...start timer\n");
      TimerHandle_t t100ms = xTimerCreate("t100ms", pdMS_TO_TICKS(100), true, NULL, on_timer100ms);
      xTimerStart(t100ms, 0);
      TimerHandle_t t1s = xTimerCreate("t1s", pdMS_TO_TICKS(1000), true, NULL, on_timer1s);
      xTimerStart(t1s, 0);

      vTaskDelay(pdMS_TO_TICKS(10000));
      int Ram = heap_caps_get_free_size(MALLOC_CAP_32BIT);
      int DRam = heap_caps_get_free_size(MALLOC_CAP_8BIT);
      int IRam = Ram - DRam;

      ESP_LOGI(TAG_INFO, "RAM \t\t %d", Ram);
      ESP_LOGI(TAG_INFO, "DRAM \t\t %d", DRam);
      ESP_LOGI(TAG_INFO, "IRAM \t\t %d", IRam);

      // heap
      int free = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);   // byte
      ESP_LOGI(TAG_INFO, "heap free = %d", free);

      int stackUnused = uxTaskGetStackHighWaterMark(NULL);
      ESP_LOGI(TAG_INFO, "Main stack unused = %d", stackUnused);

      // esp_restart();       // soft restart, deep sleep will not time
}



void OnConnectedTask(void *para){
   while (true){
      if (xSemaphoreTake(binSem_connectWifi, portMAX_DELAY)){
         printf("on connected\n"); 
         
         /* create mDNS */
         start_mdns_service();   
         printf("start mdns\n"); 
         
         /* create server */
         init_esp_server();      
         printf("init server\n"); 
         
         /*****  MQTT *****/
         vTaskDelay(pdMS_TO_TICKS(1000));
         if(wifi_status()) init_mqtt_client();
         printf("init mqtt\n");

         

         int stackUnused = uxTaskGetStackHighWaterMark(NULL);
         ESP_LOGI("OnConnectedTask", "stack unused = %d", stackUnused);
      }
   }
}


/***** FreeRTOS timer 100ms ******/  
void on_timer100ms(TimerHandle_t xTimer){
   //  printf("time hit %lld\n", esp_timer_get_time()/1000);
   //  btn0Check = (btn0Check << 1) | gpio_get_level(BTN0);
   //  btn0Check &= PIN_CHECK;

   //  if(!changeToAP && btn0Check == 0) btn0Push3s++; 
   //  else btn0Push3s = 0;
    
   //  if(btn0Push3s >= 30){   btn0Push3s = 0; 
   //    changeToAP = true; 
   //    xTaskCreate(resetWifiTask, "reset wifi", 1024 * 2, NULL, 15, NULL);
   //    // printf("Change to AP mode\n");
   //  }else if(btn0Check == PIN_RELEASE && !changeToAP){
   //    // printf("btn0 push\n");
   //  }

   /*   Switch 1   */
   sw1Check = (sw1Check << 1) | gpio_get_level(PIN_SW1);
   sw1Check &= PIN_CHECK;
   if(sw1Check == PIN_RELEASE){
      printf("sw1 push\n");
   }

   /*   Switch 2   */
   sw2Check = (sw2Check << 1) | gpio_get_level(PIN_SW2);
   sw2Check &= PIN_CHECK;
   if(!changeToAP && sw2Check == 0) sw2Push3s++; 
   else sw2Push3s = 0;
   if(sw2Push3s >= 30){   sw2Push3s = 0; 
      changeToAP = true; 
      xTaskCreate(resetWifiTask, "reset wifi", 1024 * 2, NULL, 15, NULL);
      // printf("Change to AP mode\n");
   }else if(sw2Check == PIN_RELEASE && !changeToAP){
      printf("sw2 push\n");
   }
}

/***** FreeRTOS timer 1s ******/ 
void on_timer1s(TimerHandle_t xTimer){
   if(wifi_mode() == WIFI_MODE_STA){
      if(!wifi_status()) reconnectWifiCounter++;
      if(reconnectWifiCounter > 120){ reconnectWifiCounter = 0;
         xTaskCreate(resetWifiTask, "reset wifi", 1024 * 2, NULL, 15, NULL);
      }
   }
   
   if(wifi_mode() == WIFI_MODE_STA){
      if(wifi_status()) checkInterval++;
      if(checkInterval > 10){ checkInterval = 0; 
         // printf("rssi: %d, sntp status: %d\n", wifi_sta_get_rssi(), sntp_sync_status());
         // print_current_time();
      }
   }

   // printf("time hit %lld, counter: %d\n", esp_timer_get_time()/1000, reconnectWifiCounter);
   // printf("aid %d\n", wifi_status());

}






/******    mqtt publish task         */
void test_send_messages(void *param){
    int count = 0;
    char message[50];
    while(true){
      sprintf(message, "hello from ESP32 count %d", count++);
      mqtt_send("animal/message/from/esp32", message, false);
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
}











