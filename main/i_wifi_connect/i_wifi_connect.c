#include "i_wifi_connect.h"

static bool wifiConnected, sntpSynced = false;

static char *TAG = "* WIFI";
static char *TAG_SNTP = "* SNTP";

static esp_netif_t *esp_netif;
static EventGroupHandle_t wifi_events;
static int CONNECTED = BIT0;
static int DISCONNECTED = BIT1;
static bool attempt_reconnect = false;

int disconnection_err_count = 0;

char *get_wifi_disconnection_string(wifi_err_reason_t wifi_err_reason){
    switch (wifi_err_reason){
    case WIFI_REASON_UNSPECIFIED: return "WIFI_REASON_UNSPECIFIED";
    case WIFI_REASON_AUTH_EXPIRE: return "WIFI_REASON_AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE: return "WIFI_REASON_AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE: return "WIFI_REASON_ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY: return "WIFI_REASON_ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED: return "WIFI_REASON_NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED: return "WIFI_REASON_NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE: return "WIFI_REASON_ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED: return "WIFI_REASON_ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD: return "WIFI_REASON_DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD: return "WIFI_REASON_DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_BSS_TRANSITION_DISASSOC: return "WIFI_REASON_BSS_TRANSITION_DISASSOC";
    case WIFI_REASON_IE_INVALID: return "WIFI_REASON_IE_INVALID";
    case WIFI_REASON_MIC_FAILURE: return "WIFI_REASON_MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: return "WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS: return "WIFI_REASON_IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID: return "WIFI_REASON_GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID: return "WIFI_REASON_PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID: return "WIFI_REASON_AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION: return "WIFI_REASON_UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP: return "WIFI_REASON_INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED: return "WIFI_REASON_802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED: return "WIFI_REASON_CIPHER_SUITE_REJECTED";
    case WIFI_REASON_TDLS_PEER_UNREACHABLE: return "WIFI_REASON_TDLS_PEER_UNREACHABLE";
    case WIFI_REASON_TDLS_UNSPECIFIED: return "WIFI_REASON_TDLS_UNSPECIFIED";
    case WIFI_REASON_SSP_REQUESTED_DISASSOC: return "WIFI_REASON_SSP_REQUESTED_DISASSOC";
    case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT: return "WIFI_REASON_NO_SSP_ROAMING_AGREEMENT";
    case WIFI_REASON_BAD_CIPHER_OR_AKM: return "WIFI_REASON_BAD_CIPHER_OR_AKM";
    case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION: return "WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION";
    case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS: return "WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS";
    case WIFI_REASON_UNSPECIFIED_QOS: return "WIFI_REASON_UNSPECIFIED_QOS";
    case WIFI_REASON_NOT_ENOUGH_BANDWIDTH: return "WIFI_REASON_NOT_ENOUGH_BANDWIDTH";
    case WIFI_REASON_MISSING_ACKS: return "WIFI_REASON_MISSING_ACKS";
    case WIFI_REASON_EXCEEDED_TXOP: return "WIFI_REASON_EXCEEDED_TXOP";
    case WIFI_REASON_STA_LEAVING: return "WIFI_REASON_STA_LEAVING";
    case WIFI_REASON_END_BA: return "WIFI_REASON_END_BA";
    case WIFI_REASON_UNKNOWN_BA: return "WIFI_REASON_UNKNOWN_BA";
    case WIFI_REASON_TIMEOUT: return "WIFI_REASON_TIMEOUT";
    case WIFI_REASON_PEER_INITIATED: return "WIFI_REASON_PEER_INITIATED";
    case WIFI_REASON_AP_INITIATED: return "WIFI_REASON_AP_INITIATED";
    case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT: return "WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT";
    case WIFI_REASON_INVALID_PMKID: return "WIFI_REASON_INVALID_PMKID";
    case WIFI_REASON_INVALID_MDE: return "WIFI_REASON_INVALID_MDE";
    case WIFI_REASON_INVALID_FTE: return "WIFI_REASON_INVALID_FTE";
    case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED: return "WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED";
    case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED: return "WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED";
    case WIFI_REASON_BEACON_TIMEOUT: return "WIFI_REASON_BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND: return "WIFI_REASON_NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL: return "WIFI_REASON_AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL: return "WIFI_REASON_ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT: return "WIFI_REASON_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_CONNECTION_FAIL: return "WIFI_REASON_CONNECTION_FAIL";
    case WIFI_REASON_AP_TSF_RESET: return "WIFI_REASON_AP_TSF_RESET";
    case WIFI_REASON_ROAMING: return "WIFI_REASON_ROAMING";
    case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG: return "WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG";
    case WIFI_REASON_SA_QUERY_TIMEOUT: return "WIFI_REASON_SA_QUERY_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY: return "WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY";
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD: return "WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD";
    case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD: return "WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD";
    }
    return "UNKNOWN";
}
char *get_wifi_mode_string(wifi_mode_t wifi_mode){
    switch (wifi_mode){
    case WIFI_MODE_NULL: return "WIFI_MODE_NULL";
    case WIFI_MODE_STA: return "WIFI_MODE_STA";
    case WIFI_MODE_AP: return "WIFI_MODE_AP";
    case WIFI_MODE_APSTA: return "WIFI_MODE_APSTA";
    case WIFI_MODE_NAN: return "WIFI_MODE_NAN";
    case WIFI_MODE_MAX: return "WIFI_MODE_MAX";
    }
    return "UNKNOWN";
}
char *get_wifi_event(int32_t event_id){
    switch (event_id){
    case WIFI_EVENT_WIFI_READY: return "WIFI_EVENT_WIFI_READY";
    case WIFI_EVENT_SCAN_DONE: return "WIFI_EVENT_SCAN_DONE";
    case WIFI_EVENT_STA_STOP: return "WIFI_EVENT_STA_STOP";
    case WIFI_EVENT_STA_AUTHMODE_CHANGE: return "WIFI_EVENT_STA_AUTHMODE_CHANGE";
    case WIFI_EVENT_STA_WPS_ER_SUCCESS: return "WIFI_EVENT_STA_WPS_ER_SUCCESS";
    case WIFI_EVENT_STA_WPS_ER_FAILED: return "WIFI_EVENT_STA_WPS_ER_FAILED";
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT: return "WIFI_EVENT_STA_WPS_ER_TIMEOUT";
    case WIFI_EVENT_STA_WPS_ER_PIN: return "WIFI_EVENT_STA_WPS_ER_PIN";
    case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP: return "WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP";
    case WIFI_EVENT_AP_START: return "WIFI_EVENT_AP_START";
    case WIFI_EVENT_AP_STOP: return "WIFI_EVENT_AP_STOP";
    case WIFI_EVENT_AP_STACONNECTED: return "WIFI_EVENT_AP_STACONNECTED";
    case WIFI_EVENT_AP_STADISCONNECTED: return "WIFI_EVENT_AP_STADISCONNECTED";
    case WIFI_EVENT_AP_PROBEREQRECVED: return "WIFI_EVENT_AP_PROBEREQRECVED";
    case WIFI_EVENT_FTM_REPORT: return "WIFI_EVENT_FTM_REPORT";
    case WIFI_EVENT_STA_BSS_RSSI_LOW: return "WIFI_EVENT_STA_BSS_RSSI_LOW";
    case WIFI_EVENT_ACTION_TX_STATUS: return "WIFI_EVENT_ACTION_TX_STATUS";
    case WIFI_EVENT_ROC_DONE: return "WIFI_EVENT_ROC_DONE";
    case WIFI_EVENT_STA_BEACON_TIMEOUT: return "WIFI_EVENT_STA_BEACON_TIMEOUT";
    case WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START: return "WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START";
    case WIFI_EVENT_AP_WPS_RG_SUCCESS: return "WIFI_EVENT_AP_WPS_RG_SUCCESS";
    case WIFI_EVENT_AP_WPS_RG_FAILED: return "WIFI_EVENT_AP_WPS_RG_FAILED";
    case WIFI_EVENT_AP_WPS_RG_TIMEOUT: return "WIFI_EVENT_AP_WPS_RG_TIMEOUT";
    case WIFI_EVENT_AP_WPS_RG_PIN: return "WIFI_EVENT_AP_WPS_RG_PIN";
    case WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP: return "WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP";
    case WIFI_EVENT_ITWT_SETUP: return "WIFI_EVENT_ITWT_SETUP";
    case WIFI_EVENT_ITWT_TEARDOWN: return "WIFI_EVENT_ITWT_TEARDOWN";
    case WIFI_EVENT_ITWT_PROBE: return "WIFI_EVENT_ITWT_PROBE";
    case WIFI_EVENT_ITWT_SUSPEND: return "WIFI_EVENT_ITWT_SUSPEND";
    case WIFI_EVENT_TWT_WAKEUP: return "WIFI_EVENT_TWT_WAKEUP";
    case WIFI_EVENT_BTWT_SETUP: return "WIFI_EVENT_BTWT_SETUP";
    case WIFI_EVENT_BTWT_TEARDOWN: return "WIFI_EVENT_BTWT_TEARDOWN";
    case WIFI_EVENT_NAN_STARTED: return "WIFI_EVENT_NAN_STARTED";
    case WIFI_EVENT_NAN_STOPPED: return "WIFI_EVENT_NAN_STOPPED";
    case WIFI_EVENT_NAN_SVC_MATCH: return "WIFI_EVENT_NAN_SVC_MATCH";
    case WIFI_EVENT_NAN_REPLIED: return "WIFI_EVENT_NAN_REPLIED";
    case WIFI_EVENT_NAN_RECEIVE: return "WIFI_EVENT_NAN_RECEIVE";
    case WIFI_EVENT_NDP_INDICATION: return "WIFI_EVENT_NDP_INDICATION";
    case WIFI_EVENT_NDP_CONFIRM: return "WIFI_EVENT_NDP_CONFIRM";
    case WIFI_EVENT_NDP_TERMINATED: return "WIFI_EVENT_NDP_TERMINATED";
    case WIFI_EVENT_HOME_CHANNEL_CHANGE: return "WIFI_EVENT_HOME_CHANNEL_CHANGE";
    case WIFI_EVENT_STA_NEIGHBOR_REP: return "WIFI_EVENT_STA_NEIGHBOR_REP";
    case WIFI_EVENT_MAX: return "WIFI_EVENT_MAX";
    }
    return "UNKNOWN";
}

/* sntp */
void print_current_time(){
    time_t now = 0;
    time(&now);
    
    struct tm * time_info = localtime(&now);        // convert time_t to tm
    char time_buffer[50];
    strftime(time_buffer, sizeof(time_buffer), "%c", time_info);  // %c = Date and time representation  "Sun Aug 19 02:56:02 2012"
    ESP_LOGI(TAG_SNTP, "epoch: %lld, GMT+7: %s ", now, time_buffer);
}


// When connect to SNTP succeeds, it updates system time (via settimeofday() internally).
void on_got_time(struct timeval *tv){   // call back function
    printf("sync to sntp: %lld\n", tv->tv_sec);
    sntpSynced = true;

    int64_t start, end;
    start = esp_timer_get_time();
    set_time_to_nvs(tv->tv_sec);
    end = esp_timer_get_time();
    printf("set nvs sntp time %.2f ms\n", (end - start) / 1000.0);
    print_current_time();
}

esp_err_t set_time_to_nvs(time_t tv_sec){
    nvs_flash_init();       // no harm to call it again
    nvs_handle_t nvs;
    if (nvs_open(NVS_SNTP_KEY, NVS_READWRITE, &nvs) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs open fail"); 
        return ESP_FAIL;
    }
    if (nvs_set_i64(nvs, "time", tv_sec) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs set tv fail"); 
        return ESP_FAIL;
    }
    nvs_commit(nvs);  // After setting any values, nvs_commit() must be called to ensure changes are written to flash storage
    nvs_close(nvs);
    return ESP_OK;
}

esp_err_t set_time_from_nvs(void){
    nvs_flash_init();       // no harm to call it again
    struct timeval tv;
    nvs_handle_t nvs;
    if (nvs_open(NVS_SNTP_KEY, NVS_READWRITE, &nvs) != ESP_OK) { 
        ESP_LOGW(TAG_SNTP, "nvs open fail");
        return ESP_FAIL; 
    }
    if (nvs_get_i64(nvs, "time", &tv.tv_sec) != ESP_OK) { 
        ESP_LOGW(TAG_SNTP, "nvs get tv fail"); 
        return ESP_FAIL; 
    }
    setenv("TZ", "<+07>-7", 1);        // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    tzset();
    tv.tv_usec = 0;      // set microsec to zero
    settimeofday(&tv, NULL);      // set to ESP32 system clock
    sntpSynced = true;
    printf("set tv %lld\n", tv.tv_sec);
            
    nvs_close(nvs);
    return ESP_OK; 
}

void sntp_got_time(void){
    ESP_LOGI(TAG_SNTP, "attempt connect to sntp server");
    // 1) Set TZ (optional, affects localtime formatting only)
    setenv("TZ", "<+07>-7", 1);        // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    tzset();
    
    // 2) Make sure networking is initialized & connected *before* SNTP
    // esp_netif_init(); esp_event_loop_create_default(); create_default_wifi_sta();
    // connect Wi-Fi and wait for IP (SYSTEM_EVENT_STA_GOT_IP / IP_EVENT_STA_GOT_IP)
  
    // 3) Configure SNTP BEFORE init()
    esp_sntp_stop(); // safe to call if not started
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);           // Sets client mode — SNTP_OPMODE_POLL means it will keep synchronizing periodically, not just once.
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);          // immediate set - When sync happens, set system time immediately (no gradual slew).
    esp_sntp_set_time_sync_notification_cb(on_got_time);   // call back - Register a callback (on_got_time) that’s called every time synchronization happens.
    // esp_sntp_set_sync_interval(60*60*1000);  // 1 hour min

    // Set multiple servers (by name) – requires DNS working
    // esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(0, "time.egat.co.th"); // use with EGAT-IoT
    esp_sntp_setservername(1, "time.google.com"); // use with EGAT-IoT
    // esp_sntp_setservername(2, "time.cloudflare.com");
    
    // 4) Start SNTP
    esp_sntp_init();        // init sntp - Starts the SNTP client task. It automatically polls the configured server(s) (default interval 1 hour).
}

bool sntp_sync_status(void){
    // if(esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) sntpSynced = true;

    return sntpSynced;
    /*
        SNTP_SYNC_STATUS_RESET,         // Reset status.                     
        SNTP_SYNC_STATUS_COMPLETED,     // Time is synchronized.             
        SNTP_SYNC_STATUS_IN_PROGRESS,   // Smooth time sync in progress.     
    */
}

void event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    switch (event_id){
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        esp_wifi_connect();         // connect wifi and wait until event occur
        break;
    case WIFI_EVENT_STA_CONNECTED:
        wifiConnected = true;
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
        disconnection_err_count = 0;

        break;
    case WIFI_EVENT_STA_DISCONNECTED:
    {
        wifiConnected = false; //sntpSynced = false;
        wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = event_data;
        ESP_LOGW(TAG, "DISCONNECTED %d, %s", wifi_event_sta_disconnected->reason, get_wifi_disconnection_string(wifi_event_sta_disconnected->reason));
        if(attempt_reconnect){
            // if(
            //     wifi_event_sta_disconnected->reason == WIFI_REASON_NO_AP_FOUND ||
            //     wifi_event_sta_disconnected->reason == WIFI_REASON_ASSOC_LEAVE ||
            //     wifi_event_sta_disconnected->reason == WIFI_REASON_UNSPECIFIED
            // ){
                if(disconnection_err_count++ < 5){
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    esp_wifi_connect();         // connect wifi and wait until event occur
                    break;
                }
            // }
        }
        xEventGroupSetBits(wifi_events, DISCONNECTED);
        break;
    }   
    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
        xEventGroupSetBits(wifi_events, CONNECTED);
        
        /*****  SNTP *****/
        vTaskDelay(pdMS_TO_TICKS(1000));
        sntp_got_time();
        


        break;
    default:
        ESP_LOGW(TAG, "%s", get_wifi_event(event_id));
        break;
    }
}


esp_err_t wifi_get_config(void){
    esp_err_t err;
    wifi_config_t wifi_config = {};
    err = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "get config fail"); 
        return ESP_FAIL;
    }
    //(char *) wifi_config.sta.ssid and (char *) wifi_config.sta.password will provide values of already store wifi ssid and password credentials
    ESP_LOGI(TAG, "WIFI SSID: %s", (char *) wifi_config.sta.ssid);
    ESP_LOGI(TAG, "WIFI Password: %s", (char *) wifi_config.sta.password);
    // if(wifi_config.sta.ssid != NULL && wifi_config.sta.password != NULL) 
    return ESP_OK;
}

int wifi_sta_get_rssi(void){
    esp_err_t err;
    int _rssi;
    err = esp_wifi_sta_get_rssi(&_rssi);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "get rssi fail"); 
        return 0;    // 0 is no signal
    }
    return _rssi;
}


esp_err_t  wifi_connect_init(void){
    esp_err_t err;
    err = esp_netif_init();                                  // init network interface
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Init TCP/IP fail"); 
        return ESP_FAIL;    
    }
    err = esp_event_loop_create_default();                     // create event loop
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Create event fail"); 
        return ESP_FAIL;    
    }
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_init_config);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Wifi init fail"); 
        return ESP_FAIL;    
    }
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL);      // looking all event in WIFI_EVENT
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Event handler fail"); 
        return ESP_FAIL;    
    }
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL);     // looking only IP_EVENT_STA_GOT_IP event
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Event handler fail"); 
        return ESP_FAIL;    
    }
    err = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "set storage fail"); 
        return ESP_FAIL;    
    }
    return ESP_OK;
}


esp_err_t wifi_connect_sta(char * ssid, char *pass, int timeout){
    esp_err_t err;
    attempt_reconnect = true;
    ESP_LOGI(TAG, "wifi connect as STA mode");
    esp_netif = esp_netif_create_default_wifi_sta();
    wifi_events = xEventGroupCreate();          // create event group 
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Set mode sta fail"); 
        return ESP_FAIL;    
    }
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)-1);                // set ssid
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password)-1);        // set password
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Set config sta fail"); 
        return ESP_FAIL;    
    }
    err = esp_wifi_start();       // start wifi and wait until event occur
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Start sta fail"); 
        return ESP_FAIL;    
    }
    EventBits_t result = xEventGroupWaitBits(wifi_events, CONNECTED | DISCONNECTED, true, false, pdMS_TO_TICKS(timeout));
    if(result == CONNECTED) return ESP_OK;
    return ESP_FAIL;
}


esp_err_t wifi_connect_ap(const char * ssid, const char *pass){
    esp_err_t err;
    attempt_reconnect = true;
    ESP_LOGI(TAG, "wifi connect as AP mode");
    esp_netif = esp_netif_create_default_wifi_ap();
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Set mode ap fail"); 
        return ESP_FAIL;    
    }
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid)-1);                // set ssid
    strncpy((char *)wifi_config.ap.password, pass, sizeof(wifi_config.ap.password)-1);        // set password
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;
    wifi_config.ap.channel = 1;                 // channel from 1-11 in Europe,  from 1-13 of other standard
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Set config ap fail"); 
        return ESP_FAIL;    
    }
    err = esp_wifi_start();       // start wifi and wait until event occur
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Start ap fail"); 
        return ESP_FAIL;    
    }
    return ESP_OK;
}


void wifi_disconnect(void){
    attempt_reconnect = false;
    if(esp_wifi_stop() != ESP_OK){
        ESP_LOGI(TAG, "wifi stop fail");
        // return;
    }
    esp_netif_destroy(esp_netif);
}


wifi_mode_t wifi_mode(void){
    wifi_mode_t esp_wifi_mode;
    esp_wifi_get_mode(&esp_wifi_mode);
    return esp_wifi_mode;
}

bool wifi_status(void){
    return wifiConnected;
}

bool wifi_sta_status(void){
    return wifiConnected && (wifi_mode() != WIFI_MODE_STA);
}

void wifiInitTask(void *params){
    esp_err_t err;

    err = nvs_flash_init();      // run only one time
    if (err != ESP_OK) { ESP_LOGW(TAG, "Init nvs fail"); }
    err = wifi_connect_init();
    if (err != ESP_OK) { ESP_LOGW(TAG, "Init wifi fail"); }
    while (true){
        if (xSemaphoreTake(binSem_initWifi, portMAX_DELAY)){
          /** check wifi ssid and password in nvs  */  
            nvs_handle_t nvs;
            err = nvs_open(NVS_WIFI_KEY, NVS_READWRITE, &nvs);
            if (err != ESP_OK) { ESP_LOGW(TAG, "nvs open fail"); }
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
            ESP_LOGI(TAG, "SSID: %s, PASS: %s", ssid ? ssid : "(null)", pass ? pass : "(null)");
            // printf("wifi ssid: %s pass: %s\n", ssid, pass);

          /** select wifi mode station or access point  */ 
            if(changeToAP || (ssid == NULL || pass == NULL)){ changeToAP = false;
                err = wifi_connect_ap(AP_WIFI_SSID, AP_WIFI_PASS);
                if (err != ESP_OK) { ESP_LOGW(TAG, "connect ap fail"); }
            }else{
                err = wifi_connect_sta(ssid, pass, 20000);
                if (err != ESP_OK) { ESP_LOGW(TAG, "connect sta fail"); }
            }
            xSemaphoreGive(binSem_connectWifi);
                       
          /** clear memory  */ 
            if (ssid != NULL) free(ssid);
            if (pass != NULL) free(pass);

        }

        int stackUnused = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "stack unused = %d", stackUnused);
    }  

}

