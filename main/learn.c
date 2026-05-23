
/* 
string.h function

memset(&buf, value, size)           use for clear value in memory
memcpy(&dest, &source, size)        use for copy a block of memory *** have to assige the null terminator at the end '\0'
memmove(&dest, &source, size)       use for move memory can do same as memcpy

malloc(void *ptr);                  allocate momory block (byte) -> if fail return NULL
calloc(void *ptr);                  allocate momory block (byte) with Zero-Initialized Memory -> if fail return NULL
realloc(void *ptr, size_t newSize)  change the size of memnory block without losing the old data -> if fail return NULL
                                    this function moves the contents of the old block to a new block and the data of the old block is not lost.


strcpy(&dest, &source)              copy the string untill null terminator '\0'
strncpy(&dest, &source, size);
strrchr(req->uri, '.');             strrchr() look the string from the back
strcmp(ext, ".css")         
strtok(buf, "\r\n");                replace \r\n with NULL terminator \0



*/



    // printf("hello world!\n");
    // esp_log_level_set("LOG", ESP_LOG_INFO);     // setting tag "LOG" to display = error + warning + info log
    // ESP_LOGE("LOG", "This is an error");
    // ESP_LOGW("LOG", "This is a warning");
    // ESP_LOGI("LOG", "This is an info");
    // ESP_LOGD("LOG", "This is a debug");
    // ESP_LOGV("LOG", "This is verbose");

    // int number = 0;
    // ESP_LOGE("TAG 2", "This is an error %d", number++);
    // ESP_LOGW("TAG 2", "This is a warning %d", number++);
    // ESP_LOGI("TAG 2", "This is an info %d", number++);
    // ESP_LOGD("TAG 2", "This is a debug %d", number++);
    // ESP_LOGV("TAG 2", "This is verbose %d", number++);




//  LED brink
    // gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    // uint32_t isOn = 0;
    // while (true)
    // {
    //     vTaskDelay(1000/portTICK_PERIOD_MS);
    //     isOn = !isOn;
    //     gpio_set_level(LED, isOn);
        
    //     if(isOn){
    //         ESP_LOGI(TAG, "LED ON");
    //     }else{
    //         ESP_LOGI(TAG, "LED OFF");
    //     }
    // }




// keyboard input    
    // char c = 0;
    // char str[100];
    // memset(str, 0, sizeof(str));  // clear str
    // while (c != '\n')
    // {
    //     c = getchar();
    //     if(c != 0xFF){
    //         str[strlen(str)] = c;
    //         printf("%c", c);
    //     }
    //     vTaskDelay(100/portTICK_PERIOD_MS);
    // }
    // printf("you typed: %s\n", str);

// printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
// esp_restart();




/***** Structure Pointer  ******/
/*
    struct Person{
        char firstName[20];
        char lastName[20];
        int age;
    };

    void app_main(void)
    {
        struct Person person;
        strcpy(person.firstName, "Bob");
        strcpy(person.lastName, "Fisher");
        person.age = 35;
        printf("person: %s %s is %d years old\n", person.firstName, person.lastName, person.age);
    }




    typedef struct Person_struct{
        char firstName[20];
        char lastName[20];
        int age;
    } Person;

    void app_main(void)
    {
        Person person;
        strcpy(person.firstName, "Bob");
        strcpy(person.lastName, "Fisher");
        person.age = 35;
        printf("person: %s %s is %d years old\n", person.firstName, person.lastName, person.age);
    }




    typedef struct Person_struct{
        char firstName[20];
        char lastName[20];
        int age;
    } Person;

    void updatePerson(Person *person){
        strcpy(person->firstName, "Bob");
        strcpy(person->lastName, "Fisher");
        person->age = 35;
    }

    void app_main(void)
    {
        Person person;
        updatePerson(&person);
        printf("person: %s %s is %d years old\n", person.firstName, person.lastName, person.age);
    }

    void updatePerson(Person *person){
        strcpy(person->firstName, "Bob");
        strcpy(person->lastName, "Fisher");
        person->age = 35;
    }

    void exclamIt(char *phrase){
        strcat(phrase, "!");
        // sprintf(phrase, "%s!", phrase);
    }

    void app_main(void)
    {
        Person person;
        char phrase[20] = {"Hello world"};
        exclamIt(phrase);
        printf("function output: %s\n", phrase);
        updatePerson(&person);
        printf("person: %s %s is %d years old\n", person.firstName, person.lastName, person.age);
    }
*/





/***** Function Pointer  ******/
/*
    typedef struct Person_struct{
        char firstName[20];
        char lastName[20];
        int age;
        void (*DoWork)(char *dataRetrieved)     // function pointer 
    } Person;

    void updatePerson(Person *person, char *dataRetrieved){
        strcpy(person->firstName, "data from dataRetrieved");
        strcpy(person->lastName, "data from dataRetrieved");
        person->DoWork = DoWorkForPerson;
        person->DoWork("some params");
        person->age = 35;
    }

    void connectAndGetInfo(char *url, void (*DoWork)(char *dataRetrieved)){   // function pointer = void (*DoWork)(char *dataRetrieved)
        //connect to wireless
        // connect to endpoint
        char *dataRetrieved = (char *) malloc(1024);        //  allocate memory and casting to char *
        // fill buffer with data: getRequest(&dataRetrieved);
        // do work and get a person object
        DoWork(dataRetrieved);
        free((void *) dataRetrieved);                       // clean up memory and other resources
    }

    void DoWorkForPerson(char *dataRetrieved){              // signature
        Person person;
        updatePerson(&person, dataRetrieved);
        printf("person: %s %s is %d years old\n", person.firstName, person.lastName, person.age);
    }

    void app_main(void){
        connectAndGetInfo("http://getperson.com", DoWorkForPerson);
    }
*/




/***** Task  ******/
/*
    xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask)
    xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID)
    pvTaskCode      = function name
    pcName          = funciton description for debug
    usStackDepth    = stack size => 100 = 100*4 byte = 400 byte      configMINIMAL_STACK_SIZE = 1536
    pvParameters    = pass parameter by any type ex. struct, *char 
    uxPriority      = 0-24     configMAX_PRIORITIES = 25-1
    pxCreatedTask   = function handler
                       // Use the handle to delete the task.
                        if( xHandle != NULL ) {  vTaskDelete( xHandle ); }
    xCoreID         = The core to which the task is pinned to, or tskNO_AFFINITY if the task has no core affinity.
        return pdPASS, pdFAIL
            
            
    Dual core Xtensa 32 bit LX6
        core 0 = PRO_CPU     handling Wi-Fi or Bluetooth are pinned to Core 0
        core 1 = APP_CPU     handling the remainder of the application are pinned to Core 1


    FreeRTOS interaction between 2 or more task - Decision tree
        1) Task notificaiton 	-> use when -> I want to communicate directly to another task. I care about counting or sending simple data
        2) Semaphone 		    -> use when -> Someone needs to block or unblock a task. I don't care who
        3) Mutex		        -> use when -> Only the task that blocks a another task can unblock it
        4) Queue		        -> use when -> I need to pass data from one task to another
        5) Event group		    -> use when -> Multiple things need to happen before I Unblock a task

    xTaskCreate(); => If configNUMBER_OF_CORES > 1, this function will create an tskNO_AFFINITY => run to core kernel manage which task to run
*/




/***** dual core example ******/
/*  
    static TaskHandle_t receiverHandler = NULL;

    void task1(void *params){
        while (true){
            printf("reading temperature from %s \n", (char *) params);
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }   
    }

    void task2(void *params){
        while (true){
            printf("reading humidity %s \n", (char *) params);
            vTaskDelay(2000/portTICK_PERIOD_MS);
        }   
    }

    void app_main(void){
        xTaskCreate(&task1, "temperature reading", 2048, "task 1", 2, NULL);
        xTaskCreatePinnedToCore(&task2, "humidity reading", 2048, "task 1", 2, NULL, 1);   // Core 0 = default core, Core 1 = second core
    }
*/





/***** Task Notifination 1 ******/
/*
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"

    static TaskHandle_t receiverHandler = NULL;

    void sender(void *params){
        while (true){
            xTaskNotifyGive(receiverHandler);
            xTaskNotifyGive(receiverHandler);
            xTaskNotifyGive(receiverHandler);
            xTaskNotifyGive(receiverHandler);
            vTaskDelay(5000/portTICK_PERIOD_MS);        
        }   
    }

    void receiver(void *params){
        while (true){
            int count = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);       // this task block program until received notification  -> notifiy 4 times
            // int count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     // this task block program until received notification  -> notifiy only 1 time
            printf("received notification %d times\n", count);
        }   
    }

    void app_main(void){
        xTaskCreate(&receiver, "receiver", 2048, NULL, 2, &receiverHandler);
        xTaskCreate(&sender, "sender", 2048, NULL, 2, NULL);   // Core 0 = default core, Core 1 = second core

    }



/***** Task Notifination 2 *****
    static TaskHandle_t receiverHandler = NULL;

    void sender(void *params){
        while (true){
            xTaskNotify(receiverHandler, (1<<0), eSetValueWithoutOverwrite);
            vTaskDelay(1000/portTICK_PERIOD_MS);        
            xTaskNotify(receiverHandler, (1<<1), eSetValueWithoutOverwrite);
            vTaskDelay(1000/portTICK_PERIOD_MS);   
            xTaskNotify(receiverHandler, (1<<2), eSetValueWithoutOverwrite);
            vTaskDelay(1000/portTICK_PERIOD_MS);   
            xTaskNotify(receiverHandler, (1<<3), eSetValueWithoutOverwrite);
            vTaskDelay(1000/portTICK_PERIOD_MS);   
        }   
    }

    void receiver(void *params){
        unsigned int state;
        while (true){
            xTaskNotifyWait(0, 0, &state, portMAX_DELAY);   
            printf("received state %d times\n", state);
        }   
    }

    void app_main(void){
        xTaskCreate(&receiver, "sender", 2048, NULL, 2, &receiverHandler);
        xTaskCreate(&sender, "receiver", 2048, NULL, 2, NULL);   // Core 0 = default core, Core 1 = second core

    }




    static TaskHandle_t receiverHandler = NULL;

    void sender(void *params){
        while (true){
            xTaskNotify(receiverHandler, (1<<0), eSetBits);  //  eSetBites not clear previous value
            xTaskNotify(receiverHandler, (1<<1), eSetBits);
            vTaskDelay(1000/portTICK_PERIOD_MS);   
            xTaskNotify(receiverHandler, (1<<2), eSetBits);
            xTaskNotify(receiverHandler, (1<<3), eSetBits);
            vTaskDelay(1000/portTICK_PERIOD_MS);   
        }   
    }

    void receiver(void *params){
        unsigned int state;
        while (true){
            xTaskNotifyWait(0xFFFFFFFF, 0, &state, portMAX_DELAY);    // 0xFFFFFFFF clear all bit
            printf("received state %d times\n", state);
        }   
    }

    void app_main(void){
        xTaskCreate(&receiver, "sender", 2048, NULL, 2, &receiverHandler);
        xTaskCreate(&sender, "receiver", 2048, NULL, 2, NULL);   // Core 0 = default core, Core 1 = second core

    }
*/



/***** mutex semaphore => (flag, signal) ******/
/*
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"

    SemaphoreHandle_t mutexBus;     // the flag that occupy the bus

    void writeToBus(char *message){  // communication write to the bus like I2C, serial
        printf(message);
    }

    void task1(void *params){
        while (true){
            printf("reading temperature \n");
            if(xSemaphoreTake(mutexBus, 1000/portTICK_PERIOD_MS)){      // occupy the bus to write the data 
                writeToBus("temperature is 25c\n");                     // write data to bus
                xSemaphoreGive(mutexBus);                               // give the bus to another task use 
            }else{
                printf("writing temperature timeout \n");
            }
            vTaskDelay(1000/portTICK_PERIOD_MS);   
        }   
    }

    void task2(void *params){
        while (true){
            printf("reading humidity \n");
            if(xSemaphoreTake(mutexBus, 1000/portTICK_PERIOD_MS)){      // occupy the bus to write the data 
                writeToBus("humidity is 50\%\n");                       // write data to bus
                xSemaphoreGive(mutexBus);                               // give the bus to another task use 
            }else{
                printf("writing humidity timeout \n");
            }
            vTaskDelay(2000/portTICK_PERIOD_MS); 
        }   
    }

    void app_main(void){
        mutexBus = xSemaphoreCreateMutex();
        xTaskCreate(&task1, "temperature writing", 2048, NULL, 2, NULL);
        xTaskCreate(&task2, "humidity writing", 2048, NULL, 2, NULL);   

    }
*/




/***** binary semaphore ******/  
// quite similar to mutex semaphore occupy resouce from 1 task to another task
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"

// SemaphoreHandle_t binSemaphore;     // the flag that occupy the bus

// void listenForHTTP(void *params){
//     while (true){
//         printf("received http message \n");             // 1
//         xSemaphoreGive(binSemaphore);
//         printf("processed http message \n");            // 3
//         vTaskDelay(5000/portTICK_PERIOD_MS); 
//     }   
// }

// void task1(void *params){
//     while (true){
//         xSemaphoreTake(binSemaphore, portMAX_DELAY);   // wait until semaphore give happened
//         printf("doing something with http \n");         // 2
        
//     }   
// }

// void app_main(void){
//     // xTaskCreate(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler);
//     // xTaskCreatePinnedToCore(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler, core number);
//     binSemaphore = xSemaphoreCreateBinary();
//     xTaskCreate(&listenForHTTP, "get http", 2048, NULL, 1, NULL);
//     xTaskCreate(&task1, "do something with http ", 2048, NULL, 1, NULL);   // Core 0 = default core, Core 1 = second core

// }

// SemaphoreHandle_t binSemaphore;     // the flag that occupy the bus

// void listenForHTTP(void *params){
//     while (true){
//         printf("received http message \n");             // 1
//         xSemaphoreGive(binSemaphore);
//         printf("processed http message \n");            // 2
//         vTaskDelay(5000/portTICK_PERIOD_MS); 
//     }   
// }

// void task1(void *params){
//     while (true){
//         xSemaphoreTake(binSemaphore, portMAX_DELAY);   // wait until semaphore give happened
//         printf("doing something with http \n");         // 3
        
//     }   
// }

// void app_main(void){
//     // xTaskCreate(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler);
//     // xTaskCreatePinnedToCore(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler, core number);
//     binSemaphore = xSemaphoreCreateBinary();
//     xTaskCreate(&listenForHTTP, "get http", 2048, NULL, 2, NULL);
//     xTaskCreate(&task1, "do something with http ", 2048, NULL, 1, NULL);   // Core 0 = default core, Core 1 = second core

// }




/***** queue ******/
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"

// QueueHandle_t queue;     // the flag that occupy the bus

// void listenForHTTP(void *params){
//     int count = 0;
//     while (true){
//         count++;
//         printf("received http message \n");                 // 1
//         long ok = xQueueSend(queue, &count, 1000/portTICK_PERIOD_MS);  //
//         if(ok){
//             printf("added message to queue\n");             // 2
//         }else{
//             printf("failed to add message to queue\n"); 
//         }
//         vTaskDelay(1000/portTICK_PERIOD_MS); 
//     }   
// }

// void task1(void *params){
//     while (true){
//         int rxInt;
//         if(xQueueReceive(queue, &rxInt, 5000/portTICK_PERIOD_MS)){
//             printf("doing something with http %d\n", rxInt);        // 3
//         }
//     }   
// }

// void app_main(void){
//     // xTaskCreate(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler);
//     // xTaskCreatePinnedToCore(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler, core number);
//     queue = xQueueCreate(3, sizeof(int));
//     xTaskCreate(&listenForHTTP, "get http", 2048, NULL, 2, NULL);
//     xTaskCreate(&task1, "do something with http ", 2048, NULL, 1, NULL);   // Core 0 = default core, Core 1 = second core

// }




/***** event groups like semaphore but can trigger multiple tasks ******/    
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"
// #include "freertos/event_groups.h"


// EventGroupHandle_t evtGrp;
// const int gotHttp = BIT0;    // (1 << 0)
// const int gotBLE = BIT1;     // (1 << 1)

// void listenForHTTP(void *params){
//     while (true){
//         xEventGroupSetBits(evtGrp, gotHttp);
//         printf("got http\n"); 
//         vTaskDelay(2000/portTICK_PERIOD_MS); 
//     }   
// }

// void listenForBluetooth(void *params){
//     while (true){
//         xEventGroupSetBits(evtGrp, gotBLE);
//         printf("got bluetooth\n"); 
//         vTaskDelay(5000/portTICK_PERIOD_MS); 
//     }   
// }

// void task1(void *params){
//     while (true){
//         xEventGroupWaitBits(evtGrp, gotHttp | gotBLE, true, true, portMAX_DELAY);
//         printf("received http and bluetooth\n");
//     }   
// }

// void app_main(void){
//     // xTaskCreate(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler);
//     // xTaskCreatePinnedToCore(function call address, task name, memory utilize, pass parameter, priority 1(low)-5(high), 2, task handler, core number);
//     evtGrp = xEventGroupCreate();
//     xTaskCreate(&listenForHTTP, "get http", 2048, NULL, 1, NULL);
//     xTaskCreate(&listenForBluetooth, "get BLE", 2048, NULL, 1, NULL);
//     xTaskCreate(&task1, "do something with http ", 2048, NULL, 1, NULL);   // Core 0 = default core, Core 1 = second core

// }




/***** FreeRTOS timer ******/  
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/timers.h"
// #include "esp_system.h"
// #include "esp_timer.h"  

// void on_timer(TimerHandle_t xTimer){
//     printf("time hit %lld\n", esp_timer_get_time()/1000);
// }


// void app_main(void){
//     printf("app started %lld", esp_timer_get_time()/1000);
//     TimerHandle_t xTimer = xTimerCreate("Timer1", pdMS_TO_TICKS(1000), true, NULL, on_timer);
//     xTimerStart(xTimer, 0);
// }



/***** high resolution timer ******/ 
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_system.h"
// #include "esp_timer.h"

// void timer_callback(void* arg){
//     static bool on;         // static -> like global variable
//     on = !on;
//     gpio_set_level(GPIO_NUM_4, on);
// }

// void app_main(void){
//     gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

//     const esp_timer_create_args_t esp_timer_create_args = {
//         .callback = timer_callback,
//         .name = "My timer"
//     };
//     esp_timer_handle_t esp_timer_handle;
//     esp_timer_create(&esp_timer_create_args, &esp_timer_handle);
//     // esp_timer_start_once(esp_timer_handle, 20);          // timeout have to more than 20us
//     esp_timer_start_periodic(esp_timer_handle, 50);         // periodic of 50us

//     int x = 0;

//     while(true){

//         esp_timer_dump(stdout);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         if(x++ == 5){
//             esp_timer_stop(esp_timer_handle);
//             esp_timer_delete(esp_timer_handle);
//         }
//     }
// }




/***** Memory ******/ 
/*
    int data = 5;
    char bss[20];

    void aTask(void *param){
        // use memnory from stack
        int stackmem = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "stack space task = %d", stackmem);
        char buffer[6000];
        memset(&buffer, 1, sizeof(buffer));
        while(true){
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }


    void app_main(void){

        printf("%d %s", data, bss);
        // ESP_LOGI(TAG, "xPortGetFreeHeapSize %dk = DRAM", xPortGetFreeHeapSize);

        int Ram = heap_caps_get_free_size(MALLOC_CAP_32BIT);
        int DRam = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        int IRam = Ram - DRam;

        ESP_LOGI(TAG, "RAM \t\t %d", Ram);
        ESP_LOGI(TAG, "DRAM \t\t %d", DRam);
        ESP_LOGI(TAG, "IRAM \t\t %d", IRam);

        
        // heap
        int free = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);   // byte
        ESP_LOGI(TAG, "heap free = %d", free);
        
          // use memnory from heap
            void *memoryPointer = malloc(free);    // malloc is take memory from heap
            if(memoryPointer == NULL){
                ESP_LOGI(TAG, "failed to allocate memory");
            }else{
                ESP_LOGI(TAG, "Allocated memory");
            }

        // stack
        int stackmem = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "stack space main = %d", stackmem);

        xTaskCreate(&aTask, "a Task", 8000 byte, NULL, 1, NULL);
    }
*/




/***** storage add file ******/ 
/*
    extern const unsigned char indexHtml[] asm("_binary_index_html_start");
    printf("html = %s\n", indexHtml);

    extern const unsigned char sample[] asm("_binary_sample_txt_start");
    printf("sample = %s\n", sample);

    extern const unsigned char imgStart[] asm("_binary_pinout_png_start");
    extern const unsigned char imgEnd[] asm("_binary_pinout_png_end");              // const unsigned char = pointer
    const unsigned int imgSize = imgEnd-imgStart;
    printf("img size is = %d\n", imgSize);
*/



/***** nvs ******/ 
/*
    ESP_LOGI(TAG_INFO, "\n *** start app ***\n");

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_ERROR_CHECK(nvs_flash_init());       
    nvs_handle handle;      
    ESP_ERROR_CHECK(nvs_open("store", NVS_READWRITE, &handle));      

    int32_t val = 0;
    esp_err_t result = nvs_get_i32(handle, "val", &val);
    switch(result){
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGE(TAG_NVS, "Value not set yet");
            break;
        case ESP_OK:
            ESP_LOGI(TAG_NVS, "Value is %ld", val);
            break;
        default:
            ESP_LOGE(TAG_NVS, "Error (%s) opening NVS handle!\n", esp_err_to_name(result));
            break;
    }
    val++;
    ESP_ERROR_CHECK(nvs_set_i32(handle, "val", val));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
*/



/***** nvs partition ******/ 
// ESP_LOGI(TAG_INFO, "\n *** start app ***\n");

// vTaskDelay(pdMS_TO_TICKS(1000));
// ESP_ERROR_CHECK(nvs_flash_init_partition("nvs_name"));       
// nvs_handle handle;      
// ESP_ERROR_CHECK(nvs_open_from_partition("nvs_name", "store", NVS_READWRITE, &handle));      

// nvs_stats_t nvsStats;
// nvs_get_stats("nvs_name", &nvsStats);
// ESP_LOGI(TAG_INFO, "used: %d, free: %d, total: %d, namespace count: %d", nvsStats.used_entries, nvsStats.free_entries, nvsStats.total_entries, nvsStats.namespace_count);

// int32_t val = 0;
// esp_err_t result = nvs_get_i32(handle, "val", &val);
// switch(result){
//     case ESP_ERR_NVS_NOT_FOUND:
//         ESP_LOGE(TAG_NVS, "Value not set yet");
//         break;
//     case ESP_OK:
//         ESP_LOGI(TAG_NVS, "Value is %ld", val);
//         break;
//     default:
//         ESP_LOGE(TAG_NVS, "Error (%s) opening NVS handle!\n", esp_err_to_name(result));
//         break;
// }
// val++;
// ESP_ERROR_CHECK(nvs_set_i32(handle, "val", val));
// ESP_ERROR_CHECK(nvs_commit(handle));
// nvs_close(handle);





/***** nvs partition struct ******/ 
// typedef struct nvs_struct{
//     char name[20];
//     int age;
//     int id;
//     int resetCount;
// } Nvs_str;

// void app_main(void){
//     ESP_LOGI(TAG_INFO, "\n *** start app ***\n");

//     vTaskDelay(pdMS_TO_TICKS(1000));
//     ESP_ERROR_CHECK(nvs_flash_init_partition("mynvs"));       
//     nvs_handle handle;      
//     ESP_ERROR_CHECK(nvs_open_from_partition("mynvs", "store", NVS_READWRITE, &handle));      

//     nvs_stats_t nvsStats;
//     nvs_get_stats("mynvs", &nvsStats);
//     ESP_LOGI(TAG_INFO, "used: %d, free: %d, total: %d, namespace count: %d", nvsStats.used_entries, nvsStats.free_entries, nvsStats.total_entries, nvsStats.namespace_count);

//     char nvsKey[15];        // max 15 bytes
//     Nvs_str nvs_Str;
//     size_t nvsSize = sizeof(Nvs_str);

//     for(int i=0; i<5; i++){

//         sprintf(nvsKey, "nvs_%d", i);

//         esp_err_t result = nvs_get_blob(handle, nvsKey, (void *) &nvs_Str, &nvsSize);
//         switch(result){
//             case ESP_ERR_NVS_NOT_FOUND:
//                 ESP_LOGE(TAG_NVS, "Value not set yet");
//                 break;
//             case ESP_OK:
//                 ESP_LOGI(TAG_NVS, "nvs name: %s, age %d, id %d", nvs_Str.name, nvs_Str.age, nvs_Str.id);
//                 break;
//             default:
//                 ESP_LOGE(TAG_NVS, "Error (%s) opening NVS handle!\n", esp_err_to_name(result));
//                 break;
//         }
//     }

//     for(int i=0; i<5; i++){
//         sprintf(nvsKey, "nvs_%d", i);
//         Nvs_str new_nvs_Str;
//         sprintf(new_nvs_Str.name, "nvs_%d", i);
//         new_nvs_Str.age = i + 2;
//         new_nvs_Str.id = i;

//         ESP_ERROR_CHECK(nvs_set_blob(handle, nvsKey, (void *) &new_nvs_Str, nvsSize));
//         ESP_ERROR_CHECK(nvs_commit(handle));
//     }
//     nvs_close(handle);
// }





/***** spiffs ****/
// #include <stdlib.h>
// #include <dirent.h>
// #include <sys/unistd.h>
// #include <sys/stat.h>
// void app_main(void){
//     esp_vfs_spiffs_conf_t config = {
//         .base_path = "/spiffs",
//         .partition_label = NULL,
//         .max_files = 5,
//         .format_if_mount_failed = true,
//     };
//     esp_vfs_spiffs_register(&config);

//     DIR *dir = opendir("/spiffs");
//     struct dirent *entry;
//     while ((entry = readdir(dir)) != NULL){
//         char fullPath[300];
//         sprintf(fullPath, "/spiffs/%s", entry->d_name); 
//         struct stat entryState;
//         if(stat(fullPath, &entryState) == -1){
//             ESP_LOGE(TAG_SPIFFS, "error getting stats for %s", fullPath);
//         }else{
//             ESP_LOGI(TAG_SPIFFS, "full path = %s, file size = %ld", fullPath, entryState.st_size);
//         }
//     }
//     size_t total = 0, used = 0;
//     esp_spiffs_info(NULL, &total, &used);
//     ESP_LOGI(TAG_SPIFFS, "total = %d, used = %d", total, used);
    

//     FILE *file = fopen("/spiffs/sub/data.txt", "r");
//     if(file == NULL){
//         ESP_LOGE(TAG_SPIFFS, "could not open file");
//     }else{
//         char line[256];
//         while (fgets(line, sizeof(line), file) != NULL)
//         {
//             printf(line);
//         }
//         fclose(file);
//     }
//     esp_vfs_spiffs_unregister(NULL);
    
// }




/***** SD SPI ****/
/*

    #include "sdmmc_cmd.h"
    #include "driver/sdmmc_host.h"
    #define PIN_CS      4
    #define PIN_MOSI    5
    #define PIN_CLK     18  
    #define PIN_MISO    19

    void read_file(char *path);
    void write_file(char *path, char *content);
    

    static const char *BASE_PATH = "/store";

    void app_main(void){
        esp_err_t spiInit, spiMount;

    /***** init spi 
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024       // 1 sector = 16 KB 
        };

        spi_bus_config_t spi_bus_config = {         // config pin
            .mosi_io_num = PIN_MOSI, 
            .miso_io_num = PIN_MISO,
            .sclk_io_num = PIN_CLK,
            .quadhd_io_num = -1,
            .quadwp_io_num = -1
        };

        sdmmc_host_t host = SDSPI_HOST_DEFAULT();       // setting host
        spiInit = spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA);
        if (spiInit != ESP_OK) {
            if (spiInit == ESP_FAIL) {
                ESP_LOGE(TAG_SD, "Failed to initialize spi bus");
            }
            return;
        }
        ESP_LOGI(TAG_SD, "initialize spi bus");

        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = PIN_CS;           // redefine cs pin
        slot_config.host_id = host.slot;        // make sure to use same slot with host

        sdmmc_card_t *card;

        spiMount = esp_vfs_fat_sdspi_mount(BASE_PATH, &host, &slot_config, &mount_config, &card);
        if (spiMount != ESP_OK) {
            if (spiMount == ESP_FAIL) {
                ESP_LOGE(TAG_SD, "Failed to mount filesystem");
            } else {
                ESP_LOGE(TAG_SD, "Failed to initialize the card (%s). "
                        "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(spiMount));
            }
            return;
        }
        ESP_LOGI(TAG_SD, "spi mounted");
        sdmmc_card_print_info(stdout, card);

    /***  
        DMA_ATTR static char long_text[1024];
        memset(&long_text, 'c', 1023);
        
        int64_t start_write = esp_timer_get_time();             // start timer

        FILE *file = fopen("/store/text.txt", "w");
        fputs(long_text, file);
        fclose(file);

        int64_t start_read = esp_timer_get_time(); 

        file = fopen("/store/text.txt", "r");
        fgets(long_text, 1023, file);
        fclose(file);
        int64_t end_read = esp_timer_get_time(); 

        printf("write time %lldus, read time %lldus\n", start_read - start_write, end_read - start_read);
        
        // write_file("/store/text.txt", "Hello world!");
        // read_file("/store/text.txt");

        esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
        spi_bus_free(host.slot);
    }


    void read_file(char *path){
        ESP_LOGI(TAG_SD, "reading file %s", path);
        FILE *file = fopen(path, "r");
        char buffer[100];
        fgets(buffer, 99, file);
        fclose(file);
        ESP_LOGI(TAG_SD, "file contains: %s", buffer);
    }

    void write_file(char *path, char *content){
        ESP_LOGI(TAG_SD, "writing \"%s\" to %s", content, path);
        FILE *file = fopen(path, "w");
        fputs(content, file);
        fclose(file);
    }
    
    I (343) SD: initialize spi bus
    I (343) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
    I (393) sdspi_transaction: cmd=52, R1 response: command not supported
    I (433) sdspi_transaction: cmd=5, R1 response: command not supported
    I (453) SD: spi mounted
    Name: SD16G
    Type: SDHC/SDXC
    Speed: 20.00 MHz (limit: 20.00 MHz)
    Size: 3830MB
    CSD: ver=2, sector_size=512, capacity=7843840 read_bl_len=9
    SSR: bus_width=1
    write time 40715us, read time 6710us
    I (503) gpio: GPIO[4]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
    I (503) gpio: GPIO[5]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
    I (513) gpio: GPIO[19]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
    I (523) gpio: GPIO[18]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
    I (533) main_task: Returned from app_main()
*/






/***** SDMMC ****/
/*
 esp_err_t ret;

    gpio_pullup_en(GPIO_NUM_12);

  /**** init sd mmc 
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024       // 1 sector = 16 KB 
    };
    sdmmc_card_t *card;
    ESP_LOGI(TAG_SD, "Initializing SD card");
    ESP_LOGI(TAG_SD, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();       // setting host use default 
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        slot_config.width = 4;                           // width of data 4 bit full speed 20MHz
        slot_config.cd = SDMMC_SLOT_NO_CD;
        slot_config.wp = SDMMC_SLOT_NO_WP;
        slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;   // Enable internal pullups on enabled pins
    ESP_LOGI(TAG_SD, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(BASE_PATH, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SD, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG_SD, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG_SD, "Filesystem mounted");

  /**** init sd mmc 
    DMA_ATTR static char long_text[1024];
    memset(&long_text, 'c', 1023);
    
    int64_t start_write = esp_timer_get_time();             // start timer

    FILE *file = fopen("/store/text.txt", "w");
    fputs(long_text, file);
    fclose(file);

    uint64_t start_read = esp_timer_get_time(); 

    file = fopen("/store/text.txt", "r");
    fgets(long_text, 1023, file);
    fclose(file);
    uint64_t end_read = esp_timer_get_time(); 

    printf("write time %lld, read time %lld\n", start_read - start_write, end_read - start_read);
    
    // write_file("/store/text.txt", "Hello world!");
    // read_file("/store/text.txt");

    esp_vfs_fat_sdcard_unmount(BASE_PATH, card); 
*/




// static const char *BASE_PATH = "/store";
  
//   /**** init sd mmc */
//     esp_vfs_fat_sdmmc_mount_config_t mount_config = {
//         .format_if_mount_failed = true,
//         .max_files = 5,
//         .allocation_unit_size = 16 * 1024       // 1 sector = 16 KB 
//     };
//     sdmmc_card_t *card;
//     sdmmc_host_t host = SDMMC_HOST_DEFAULT();       // setting host use default 
//     sdmmc_slot_config_t sdmmc_slot = SDMMC_SLOT_CONFIG_DEFAULT();
//         sdmmc_slot.width = 4;                           // width of data 4 bit full speed 20MHz
//         sdmmc_slot.cd = SDMMC_SLOT_NO_CD;
//         sdmmc_slot.wp = SDMMC_SLOT_NO_WP;
//         sdmmc_slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
//     ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount(BASE_PATH, &host, &sdmmc_slot, &mount_config, &card));
  
//   /**** init sd mmc */
//     DMA_ATTR static char long_text[1024];
//     memset(&long_text, 'c', 1023);
    
//     int64_t start_write = esp_timer_get_time();             // start timer

//     FILE *file = fopen("/store/text.txt", "w");
//     fputs(long_text, file);
//     fclose(file);

//     uint64_t start_read = esp_timer_get_time(); 

//     file = fopen("/store/text.txt", "r");
//     fgets(long_text, 1023, file);
//     fclose(file);
//     uint64_t end_read = esp_timer_get_time(); 

//     printf("write time %lld, read time %lld\n", start_read - start_write, end_read - start_read);
    
//     // write_file("/store/text.txt", "Hello world!");
//     // read_file("/store/text.txt");

//     esp_vfs_fat_sdcard_unmount(BASE_PATH, card); 






/**** GPIO *****/
// #define LED_BUILDIN 2
// #define PIN_SW1     34

// void app_main(void){
//     gpio_set_direction(LED_BUILDIN, GPIO_MODE_OUTPUT);

//     gpio_set_direction(PIN_SW1, GPIO_MODE_INPUT);
//     gpio_pulldown_en(PIN_SW1);
//     gpio_pullup_dis(PIN_SW1);               // if not use pullup or pulldown disable it 
//     // gpio_pullup_en(PIN_SW1);
//     // gpio_pulldown_dis(PIN_SW1);          // if not use pullup or pulldown disable it 

//     while (true){
//         int level = gpio_get_level(PIN_SW1);
//         gpio_set_level(LED_BUILDIN, level);
//         vTaskDelay(pdMS_TO_TICKS(1));
//     }
// }





/**** GPIO interrupt with task queue and De-bouncing *****/
// QueueHandle_t interputQueue;
//     gpio_install_isr_service(0);
//     gpio_isr_handler_add(PIN_SW1, gpio_isr_handler, (void *) PIN_SW1);

// void buttonPushedTask(void *params){
//     int pinNumber, count = 0;;
//     while(true){
//         if(xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY)){
//             /*** De-bouncing */ 
//             // disable the interrupt
//             gpio_isr_handler_remove(PIN_SW1);
//             // wait some time while we check for the button to be released
//             do{
//                 vTaskDelay(20 / portTICK_PERIOD_MS);
//             }while(gpio_get_level(pinNumber) == 1);

//             printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count++, gpio_get_level(PIN_SW1));
            
//             // re-enable the interrupt
//             gpio_isr_handler_add(PIN_SW1, gpio_isr_handler, (void *) PIN_SW1);

//         }
//     }
// }

// void app_main(void){
//     gpio_set_direction(LED_BUILDIN, GPIO_MODE_OUTPUT);

//     gpio_set_direction(PIN_SW1, GPIO_MODE_INPUT);
//     gpio_pulldown_en(PIN_SW1);
//     gpio_pullup_dis(PIN_SW1);               // if not use pullup or pulldown disable it 
//     // gpio_pullup_en(PIN_SW1);
//     // gpio_pulldown_dis(PIN_SW1);          // if not use pullup or pulldown disable it 
//     gpio_set_intr_type(PIN_SW1, GPIO_INTR_POSEDGE);

//     interputQueue = xQueueCreate(10, sizeof(int));
//     xTaskCreate(buttonPushedTask, "buttonPushedTask", 2048, NULL, 1, NULL);

//     gpio_install_isr_service(0);
//     gpio_isr_handler_add(PIN_SW1, gpio_isr_handler, (void *) PIN_SW1);

// }



/**** GPIO config interrupt with task queue and De-bouncing *****/
// QueueHandle_t interputQueue;
// static void IRAM_ATTR gpio_isr_handler(void *args){  // IRAM_ATTR = tell complier to use DRAM for interrupt
//     int pinNumber = (int) args;
//     xQueueSendFromISR(interputQueue, &pinNumber, NULL);

// }

// void blinkTask(void *params){
//     int isOn = 0;
//     while(true){
//         isOn = !isOn;
//         gpio_set_level(LED_BUILDIN, isOn);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

// void buttonPushedTask(void *params){
//     int pinNumber, count = 0;;
//     while(true){
//         if(xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY)){
//             /*** De-bouncing */ 
//             // disable the interrupt
//             gpio_isr_handler_remove(PIN_SW1);
//             // wait some time while we check for the button to be released
//             do{
//                 vTaskDelay(100 / portTICK_PERIOD_MS);
//             }while(gpio_get_level(pinNumber) == 1);

//             printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count++, gpio_get_level(PIN_SW1));
            
//             // re-enable the interrupt
//             gpio_isr_handler_add(PIN_SW1, gpio_isr_handler, (void *) PIN_SW1);
//         }
//     }
// }

// void app_main(void){
//     gpio_config_t outputConfig = {
//         .intr_type = GPIO_INTR_DISABLE,
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_down_en = 0,
//         .pull_up_en = 0,
//         .pin_bit_mask = (1ULL<<LED_BUILDIN)
//     };  gpio_config(&outputConfig);

//     gpio_config_t inputIsrConfig = {
//         .intr_type = GPIO_INTR_POSEDGE,
//         .mode = GPIO_MODE_INPUT,
//         .pull_down_en = 1,
//         .pull_up_en = 0,
//         .pin_bit_mask = (1ULL<<PIN_SW1) | (1ULL<<PIN_SW2)
//     };  gpio_config(&inputIsrConfig);

//     interputQueue = xQueueCreate(10, sizeof(int));
//     xTaskCreate(buttonPushedTask, "buttonPushedTask", 2048, NULL, 1, NULL);
//     xTaskCreate(blinkTask, "blinkTask", 2048, NULL, 1, NULL);

//     gpio_install_isr_service(0);
//     gpio_isr_handler_add(PIN_SW1, gpio_isr_handler, (void *) PIN_SW1);

// }




/*** DAC */
// #include "driver/dac.h"
//    dac_output_enable(DAC_CHAN_0);
//    dac_output_voltage(DAC_CHAN_0, 200);





/**** LEDC */
    // ledc_timer_config_t timer = {
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .duty_resolution = LEDC_TIMER_10_BIT,
    //     .timer_num = LEDC_TIMER_0,
    //     .freq_hz = 5000,
    //     .clk_cfg = LEDC_AUTO_CLK};
    // ledc_timer_config(&timer);

    // ledc_channel_config_t channel = {
    //     .gpio_num = 4,
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .channel = LEDC_CHANNEL_0,
    //     .timer_sel = LEDC_TIMER_0,
    //     .duty = 0,
    //     .hpoint = 0};
    // ledc_channel_config(&channel);

    // ledc_fade_func_install(0);
    // for (int i = 0; i < 1024; i++)
    // {
    //     ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i, 0);
    //     // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
    //     // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    //     vTaskDelay(10 / portTICK_PERIOD_MS);
    // }

    // while(true)
    // {
    //     ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0,1000,LEDC_FADE_WAIT_DONE);
    //     ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,1024,1000,LEDC_FADE_WAIT_DONE);
    // }




/***** touch sensor */

// #include "driver/touch_pad.h"
// #define TOUCH_PAD_GPIO13_CHANNEL TOUCH_PAD_NUM4

// void app_main(void){
//     touch_pad_init();
//     touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
//     touch_pad_config(TOUCH_PAD_GPIO13_CHANNEL, -1);
//     touch_pad_filter_start(10);

//     uint16_t val;
//     uint16_t touch_filter_value = 0;
//     uint16_t touch_rawValue = 0;

//     while (true)
//     {
//         touch_pad_read_raw_data(TOUCH_PAD_GPIO13_CHANNEL, &touch_rawValue);
//         touch_pad_read_filtered(TOUCH_PAD_GPIO13_CHANNEL, &touch_filter_value);
//         touch_pad_read(TOUCH_PAD_GPIO13_CHANNEL, &val);
//         printf("val = %d raw = %d filtered = %d\n", val, touch_rawValue, touch_filter_value);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }



/* UART*/
// #define TXD_PIN     4
// #define RXD_PIN     5
// #define RX_BUF_SIZE     1024


// void app_main(void){
//     uart_config_t uart_config_t = {
//         .baud_rate = 9600,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//     };  uart_param_config(UART_NUM_1, &uart_config_t);

//     uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//     uart_driver_install(UART_NUM_1, RX_BUF_SIZE, 0, 0, NULL, 0);
    
//     char message[] = "ping";
//     printf("sending: %s\n", message);
//     uart_write_bytes(UART_NUM_1, message, sizeof(message));

//     char incoming_message[RX_BUF_SIZE];
//     memset(incoming_message, 0, sizeof(incoming_message));
//     uart_read_bytes(UART_NUM_1, (uint8_t *) incoming_message, RX_BUF_SIZE, pdMS_TO_TICKS(500));
//     printf("recieved: %s\n", incoming_message);

// }



/* UART Blue tooth*/

// #include "driver/uart.h"
// #define TXD_PIN     4
// #define RXD_PIN     5
// #define RX_BUF_SIZE     1024


// void app_main(void){
//     uart_config_t uart_config_t = {
//         .baud_rate = 9600,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//     };  uart_param_config(UART_NUM_1, &uart_config_t);
//     uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//     uart_driver_install(UART_NUM_1, RX_BUF_SIZE, 0, 0, NULL, 0);
    
//     char message[] = "ping";
//     printf("sending: %s\n", message);
//     uart_write_bytes(UART_NUM_1, message, sizeof(message));

//     char incoming_message[RX_BUF_SIZE];
//     while (true)
//     {
//         memset(incoming_message, 0, sizeof(incoming_message));
//         uart_read_bytes(UART_NUM_1, (uint8_t *) incoming_message, RX_BUF_SIZE, pdMS_TO_TICKS(500));
//         printf("recieved: %s\n", incoming_message);
//         // uart_write_bytes(UART_NUM_1, incoming_message, sizeof(incoming_message));
//     }
// }






/****** UART queue */
// #define TXD_PIN     4
// #define RXD_PIN     5
// #define RX_BUF_SIZE     1024
// #define TX_BUF_SIZE     1024
// #define PATTERN_LEN     3

// QueueHandle_t uart_queue;

// void uart_event_task(void *params){
//     uart_event_t uart_event;
//     uint8_t *received_buffer = malloc(RX_BUF_SIZE);
//     // size_t datalen;
//     while (true){
//         if(xQueueReceive(uart_queue, &uart_event, portMAX_DELAY)){
//             switch (uart_event.type){
//                 case UART_DATA:
//                     ESP_LOGI(TAG_UART1, "UART_DATA");
//                     uart_read_bytes(UART_NUM_1, received_buffer, uart_event.size, portMAX_DELAY);
//                     printf("receieved: %.*s\n", uart_event.size, received_buffer);
//                     break;
//                 case UART_BREAK:
//                     ESP_LOGI(TAG_UART1, "UART_BREAK");
//                     break;
//                 case UART_BUFFER_FULL:
//                     ESP_LOGI(TAG_UART1, "UART_BUFFER_FULL");
//                     break;
//                 case UART_FIFO_OVF:
//                     ESP_LOGI(TAG_UART1, "UART_FIFO_OVF");
//                     uart_flush_input(UART_NUM_1);           // flush buffer if full
//                     xQueueReset(uart_queue);                // reset queue buffer
//                     break;
//                 case UART_FRAME_ERR:
//                     ESP_LOGI(TAG_UART1, "UART_FRAME_ERR");
//                     break;
//                 case UART_PARITY_ERR:
//                     ESP_LOGI(TAG_UART1, "UART_PARITY_ERR");
//                     break;
//                 case UART_DATA_BREAK:
//                     ESP_LOGI(TAG_UART1, "UART_DATA_BREAK");
//                     break;
//                 // case UART_PATTERN_DET:
//                 //     ESP_LOGI(TAG_UART1, "UART_PATTERN_DET");
//                 //     uart_get_buffered_data_len(UART_NUM_1, &datalen);
//                 //     int pos = uart_pattern_pop_pos(UART_NUM_1);
//                 //     ESP_LOGI(TAG_UART1, "Detected %d pos %d", datalen, pos);
//                 //     uart_read_bytes(UART_NUM_1, received_buffer, datalen - PATTERN_LEN, pdMS_TO_TICKS(100));
//                 //     uint8_t pat[PATTERN_LEN + 1];
//                 //     memset(pat, 0, sizeof(pat));
//                 //     uart_read_bytes(UART_NUM_1, pat, PATTERN_LEN, pdMS_TO_TICKS(100));
//                 //     printf("data: %.*s === pattern: %s\n", datalen - PATTERN_LEN, received_buffer, pat);
//                 //     break;
//                 default:
//                     break;
//             }
//         }
//     }
// }

// void app_main(void){
//     uart_config_t uart_config_t = {
//         .baud_rate = 9600,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//     };  uart_param_config(UART_NUM_1, &uart_config_t);
//     uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//     uart_driver_install(UART_NUM_1, RX_BUF_SIZE, TX_BUF_SIZE, 20, &uart_queue, 0);

//     // uart_enable_pattern_det_baud_intr(UART_NUM_1, '+', PATTERN_LEN, 10000, 10, 10);
//     // uart_pattern_queue_reset(UART_NUM_1, 20);
//     xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 10, NULL);

// }





/**** i2c */
// #define SDA_GPIO 26
// #define SCL_GPIO 25
//     i2c_config_t i2c_config = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = SDA_GPIO,
//         .scl_io_num = SCL_GPIO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = 100000};
//     i2c_param_config(I2C_NUM_0, &i2c_config);
//     i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

//     uint8_t raw[2];
//     i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
//     i2c_master_start(cmd_handle);
//     i2c_master_write_byte(cmd_handle, (LM75A_ADDRESS << 1) | I2C_MASTER_READ, true);
//     i2c_master_read(cmd_handle, (uint8_t *)&raw, 2, I2C_MASTER_ACK);
//     i2c_master_stop(cmd_handle);
//     i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);
//     i2c_cmd_link_delete(cmd_handle);




// #define SDA_GPIO 26
// #define SCL_GPIO 25
// #define I2C_FREQ 100000
// #define ADS1015_ADDRESS 0x49
// #define ADS1015_CONVER_REG 0b00000000
// #define ADS1015_CONFIG_REG 0b00000001
// #define PCF8574_ADDRESS 0x20
// #define AHT20_ADDRESS   0x38
// #define AHT20_CMD_MEAS  0xAC
// #define AHT20_CMD_DATA0 0x33
// #define AHT20_CMD_DATA1 0x00

// void app_main(void){
//     esp_err_t i2c_err;

//   /*** init i2c bus */
//    i2c_master_bus_config_t i2c_mst_config = {
//       .clk_source = I2C_CLK_SRC_DEFAULT,
//       .i2c_port = I2C_NUM_0,
//       .sda_io_num = SDA_GPIO,
//       .scl_io_num = SCL_GPIO,
//       .glitch_ignore_cnt = 7,
//       .flags.enable_internal_pullup = true,
//    };
//    i2c_master_bus_handle_t bus_handle;
//    i2c_err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed1: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "i2c master bus created");
  
//   /*** init i2c device ads1115 */   
//    i2c_device_config_t i2c_ads1115_config = {
//       .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//       .device_address = ADS1015_ADDRESS,
//       .scl_speed_hz = I2C_FREQ,
//    };
//    i2c_master_dev_handle_t dev_handle_ADS1015;
//    i2c_err = i2c_master_bus_add_device(bus_handle, &i2c_ads1115_config, &dev_handle_ADS1015);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed2: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "i2c ADS1115 device added");

//   /*** init i2c device pcf8574 */ 
//    i2c_device_config_t i2c_pcf8574_config = {
//       .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//       .device_address = PCF8574_ADDRESS,
//       .scl_speed_hz = I2C_FREQ,
//    };
//    i2c_master_dev_handle_t dev_handle_PCF8574;
//    i2c_err = i2c_master_bus_add_device(bus_handle, &i2c_pcf8574_config, &dev_handle_PCF8574);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed2: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "i2c PCF8574 device added");

//   /*** init i2c device AHT20 */ 
//    i2c_device_config_t i2c_aht20_config = {
//       .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//       .device_address = AHT20_ADDRESS,
//       .scl_speed_hz = I2C_FREQ,
//    };
//    i2c_master_dev_handle_t dev_handle_aht20;
//    i2c_err = i2c_master_bus_add_device(bus_handle, &i2c_aht20_config, &dev_handle_aht20);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed2: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "i2c AHT20 device added");
    
//   /*** read i2c device ads1115 */  
//    uint8_t data_wr1[3];
//    data_wr1[0] = ADS1015_CONFIG_REG;
//    data_wr1[1] = 0b10000100;
//    data_wr1[2] = 0b01000011;
//    i2c_err = i2c_master_transmit(dev_handle_ADS1015, data_wr1, 3, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed3: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "write config register");
//    vTaskDelay(1000 / portTICK_PERIOD_MS);

//    uint8_t data_wr2[1];
//    data_wr2[0] = ADS1015_CONVER_REG;
//    uint8_t data_rd1[2];
//     // ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle_ADS1015, data_wr2, 1, data_rd, 2, 1000 / portTICK_PERIOD_MS));
//     // ESP_LOGI(TAG_I2C, "write read conversion register\n");

//    i2c_err = i2c_master_transmit(dev_handle_ADS1015, data_wr2, 1, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed4: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "write conversion register");

//    i2c_err = i2c_master_receive(dev_handle_ADS1015, data_rd1, 2, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed5: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "read conversion register from ads1115");

//    int16_t data = data_rd1[0] << 8 | data_rd1[1]; 
//    float read = 2.048/0x7fff * data;
//    printf("raw[0]: %d, raw[1]: %d => data: %d, read = %f\n", data_rd1[0], data_rd1[1], data, read);

//    vTaskDelay(1000 / portTICK_PERIOD_MS);


//   /*** read/write i2c device pcf8574 */ 
//    uint8_t read_pcf8574[1];
//    uint8_t write_pcf8574[1];
//    i2c_err = i2c_master_receive(dev_handle_PCF8574, read_pcf8574, 1, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed5: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "read register from pcf8574 ");
//    printf("pcf8574 data: %x\n", read_pcf8574[0]);

//    write_pcf8574[0] = 0b10111111;            // for use as input port write to 1 
//    i2c_err = i2c_master_transmit(dev_handle_PCF8574, write_pcf8574, 1, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed4: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    ESP_LOGI(TAG_I2C, "write register to pcf8574");
//    vTaskDelay(1000 / portTICK_PERIOD_MS);

//   /*** read i2c device AHT20 */ 
//    uint8_t write_aht20[3];
//    uint8_t read_aht20[7];
//    write_aht20[0] = AHT20_CMD_MEAS;
//    write_aht20[1] = AHT20_CMD_DATA0;
//    write_aht20[2] = AHT20_CMD_DATA1;
//    i2c_err = i2c_master_transmit(dev_handle_aht20, write_aht20, 3, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed5: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    vTaskDelay(1000 / portTICK_PERIOD_MS);
//    i2c_err = i2c_master_receive(dev_handle_aht20, read_aht20, 7, 1000 / portTICK_PERIOD_MS);
//    if (i2c_err != ESP_OK){
//       ESP_LOGE(TAG_I2C, "Failed5: (%s).", esp_err_to_name(i2c_err));
//       return;
//    };
//    printf("aht[0]: %d, aht[1]: %d, aht[2]: %d, aht[3]: %d , aht[4]: %d , aht[5]: %d , aht[6]: %d\n", read_aht20[0], read_aht20[1], read_aht20[2], read_aht20[3], read_aht20[4], read_aht20[5], read_aht20[6]);
//    vTaskDelay(1000 / portTICK_PERIOD_MS);


//    i2c_master_bus_rm_device(dev_handle_ADS1015);
//    i2c_master_bus_rm_device(dev_handle_PCF8574);
//    i2c_del_master_bus(bus_handle);

// }



/**** i2c  from esp-idf lib */
// #define I2C_PORT 0
// #define SDA_GPIO 26
// #define SCL_GPIO 25

// #ifndef APP_CPU_NUM
// #define APP_CPU_NUM PRO_CPU_NUM
// #endif

// #define GAIN ADS111X_GAIN_4V096 // +-4.096V


// static const uint8_t addr = ADS111X_ADDR_VCC;   // I2C addresses
// static i2c_dev_t devices;     // Descriptors
// static float gain_val;           // Gain value

// static void measure(size_t n);
// void ads111x_test(void *pvParameters);    // Main task

// void app_main()
// {
    
//     ESP_ERROR_CHECK(i2cdev_init());    // Init library

//     memset(&devices, 0, sizeof(devices));  // Clear device descriptors

//     xTaskCreatePinnedToCore(ads111x_test, "ads111x_test", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);   // Start task
// }

// void ads111x_test(void *pvParameters){
//     gain_val = ads111x_gain_values[GAIN];

//     // Setup ICs
//     for (size_t i = 0; i < 1; i++){
//         ESP_ERROR_CHECK(ads111x_init_desc(&devices, addr, I2C_PORT, SDA_GPIO, SCL_GPIO));

//         ESP_ERROR_CHECK(ads111x_set_mode(&devices, ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
//         ESP_ERROR_CHECK(ads111x_set_data_rate(&devices, ADS111X_DATA_RATE_32));  // 32 samples per second
//         ESP_ERROR_CHECK(ads111x_set_input_mux(&devices, ADS111X_MUX_0_GND));     // positive = AIN0, negative = GND
//         ESP_ERROR_CHECK(ads111x_set_gain(&devices, GAIN));
//     }

//     while (1)
//     {
//         for (size_t i = 0; i < 1; i++){
//             measure(i);
//         }
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }
    
// static void measure(size_t n){
//     // wait for conversion end
//    //  bool busy;
//     // do
//     // {
//     //     ads111x_is_busy(&devices[n], &busy);
//     // }
//     // while (busy);

//     // Read result
//     int16_t raw = 0;
//     if (ads111x_get_value(&devices, &raw) == ESP_OK){
//         float voltage = gain_val / ADS111X_MAX_VALUE * raw;
//         printf("[%u] Raw ADC value: %d, voltage: %.04f volts\n", n, raw, voltage);
//     }
//     else
//         printf("[%u] Cannot read ADC value\n", n);
// }





/********
 *   wifi
 */

/* scan */
// char *getAuthModeName(wifi_auth_mode_t wifi_auth_mode);

// void app_main(void){
//    nvs_flash_init();

//    /** start wifi */
//    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
//    esp_wifi_init(&wifi_init_config);
//    esp_wifi_start();
//    /** scan wifi */
//    wifi_scan_config_t wifi_scan_config = {
//       .show_hidden = true,
//       .channel = 13,
//    };
//    esp_wifi_scan_start(&wifi_scan_config, true);
//    /** ap record */
//    wifi_ap_record_t wifi_records[MAX_APs];
//    uint16_t max_record = MAX_APs;
//    /** scan */
//    esp_wifi_scan_get_ap_records(&max_record, wifi_records);
//    /**  */

//    printf("Found %d access points:\n\n", max_record);
//    printf("               SSID              | Channel | RSSI |   Auth Mode \n");
//    printf("----------------------------------------------------------------\n");
//    for (int i = 0; i < max_record; i++){
//       printf("%32s | %7d | %4d | %12s\n", (char *)wifi_records[i].ssid, wifi_records[i].primary, wifi_records[i].rssi, getAuthModeName(wifi_records[i].authmode));
//    }
//    printf("----------------------------------------------------------------\n");
// }
// char *getAuthModeName(wifi_auth_mode_t wifi_auth_mode){
//    switch(wifi_auth_mode){
//    case WIFI_AUTH_OPEN: return "WIFI_AUTH_OPEN";
//    case WIFI_AUTH_WEP: return "WIFI_AUTH_WEP";
//    case WIFI_AUTH_WPA_PSK: return "WIFI_AUTH_WPA_PSK";
//    case WIFI_AUTH_WPA2_PSK: return "WIFI_AUTH_WPA2_PSK";
//    case WIFI_AUTH_WPA_WPA2_PSK: return "WIFI_AUTH_WPA_WPA2_PSK";
//    case WIFI_AUTH_ENTERPRISE: return "WIFI_AUTH_ENTERPRISE";
//    case WIFI_AUTH_WPA3_PSK: return "WIFI_AUTH_WPA3_PSK";
//    case WIFI_AUTH_WPA2_WPA3_PSK: return "WIFI_AUTH_WPA2_WPA3_PSK";
//    case WIFI_AUTH_WAPI_PSK: return "WIFI_AUTH_WAPI_PSK";
//    case WIFI_AUTH_OWE: return "WIFI_AUTH_OWE";
//    case WIFI_AUTH_WPA3_ENT_192: return "WIFI_AUTH_WPA3_ENT_192";
//    case WIFI_AUTH_WPA3_EXT_PSK: return "WIFI_AUTH_WPA3_EXT_PSK";
//    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE: return "WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE";
//    case WIFI_AUTH_DPP: return "WIFI_AUTH_DPP";
//    case WIFI_AUTH_MAX: return "WIFI_AUTH_MAX";
//    }
//    return "UNKNOW";
// }
