#include "i_sd_mmc.h"

#define TAG  "* SD"

/***   local variable   ***/
static bool sdmmcStatus = false;
static FILE *s_fd;
static char current_fname[64];
static int current_day = -1;
static const char *MOUNT_POINT = "/sd";



/* convert tm struct to YYYYMMDD */
uint32_t tmConvert(struct tm *t){
    uint32_t date;                          // YYYYMMDD
    date = (t->tm_year + 1900) * 10000;     
    date += (t->tm_mon + 1) * 100;          
    date += t->tm_mday; 
    if(date < 20250101)  date = 20250101;                  
    return date;
}


static void sd_write_task(void *arg) {
    static data1_log_t batch[BATCH_MAX_RECORDS];
    int64_t start, end;
    size_t n = 0;
    int64_t t_last_flush = esp_timer_get_time();
    int64_t t_last_sync  = t_last_flush;
    time_t now = 0;
    uint32_t currentDate_YYYYMMDD;
    bool isOldFile = 0;
    struct tm t;


    // // 1. open or rotate file here (append mode)      /sd/%s/%u.bin     FILE *file = fopen(path, "rb+");
    // s_fd = fopen("/sd/data.bin", "ab");  // Open for append in binary mode
    // // 2. Jump to the end of the file and remember where that is — so I can keep writing new data from there.
    // fseek(s_fd, 0, SEEK_END);               // Moves the file pointer to the end of the file — so new data will be appended.
    // s_write_offset = ftell(s_fd);           // Returns the current byte offset of that file pointer (from the beginning of the file). contains the current file size in bytes (how many bytes have already been written).
    // ESP_LOGI(TAG, "data.bin size: %u", s_write_offset); 

    while (true){

        if(sdmmc_status() && sntp_sync_status()){

            /* check file name */
            time(&now);
            localtime_r(&now, &t);
            currentDate_YYYYMMDD = tmConvert(&t);
            
            if (t.tm_mday != current_day) {
                // New day → close old file
                if (s_fd) fclose(s_fd);
                snprintf(current_fname, sizeof(current_fname), "/sd/logs/%08lu.bin", currentDate_YYYYMMDD);
                // 1. open or rotate file here (append mode)      /sd/%s/%u.bin     FILE *file = fopen(path, "rb+")
                s_fd = fopen(current_fname, "ab");
                if (!s_fd) {
                    ESP_LOGE(TAG, "Failed to open log file %s", current_fname);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                }
                current_day = t.tm_mday;
                
                // 2. Jump to the end of the file and remember where that is — so I can keep writing new data from there.
                fseek(s_fd, 0, SEEK_END);               // Moves the file pointer to the end of the file — so new data will be appended.
                s_write_offset = ftell(s_fd);           // Returns the current byte offset of that file pointer (from the beginning of the file). contains the current file size in bytes (how many bytes have already been written).
                ESP_LOGI(TAG, "Opened file: %s, size: %u", current_fname, s_write_offset);
            }

            /* 1 task - write to sd card */
            // 2. check queue size more than BATCH_MAX_RECORDS
            UBaseType_t pendingQueue = uxQueueMessagesWaiting(q_sd_write);
            // ESP_LOGI(TAG, "queue size = %u", pendingQueue); 

            if(pendingQueue > BATCH_MAX_RECORDS){
                int64_t now = esp_timer_get_time();
                for (size_t i = 0; i < BATCH_MAX_RECORDS; i++) {
                    // 3. read data from queue
                    xQueueReceive(q_sd_write, &batch[i], 0);  // 64 record 1024 bytes use 0.17ms
                }

                size_t bytes = BATCH_MAX_RECORDS * sizeof(data1_log_t);              // calculate byte to record
                start = esp_timer_get_time();
                // 4. write data to  C stdio buffer (FILE*)
                size_t wrote = fwrite(batch, 1, bytes, s_fd);       // write batch record to file - dump data from RAM buffer → stdio buffer (inside the C library).
                end = esp_timer_get_time();
                ESP_LOGI(TAG, "write data to file: %u byte, time = %.2f ms", wrote, (end - start) / 1000.0);
                if (wrote == bytes) {               
                    s_write_offset += bytes;        // advance write watermark - persist to NVS occasionally

                } else {
                    // handle I/O error: card full, etc.
                }

                // 5. flush and sync - fflush() -> FATFS internal cache, fsync() -> SD card flash
                if ((now - t_last_sync) >= (FSYNC_EVERY_MS*1000)) {     // check loop run over 300 ms
                    start = esp_timer_get_time();
                    fflush(s_fd);
                    fsync(fileno(s_fd));              // crash-safe
                    end = esp_timer_get_time();
                    ESP_LOGI(TAG, "flush & sync, time = %.2f ms", (end - start) / 1000.0); 
                    // persist s_write_offset to NVS (tiny write)
                    t_last_sync = now;
                }

            }else{
                // ESP_LOGI(TAG, "Queue less than batch size");
            }

            /* 2 task - Handle SD commands (reads / flush-now / rotate) */
            sd_cmd_msg_t cmd;
            if (xQueueReceive(q_sd_cmd, &cmd, 0) == pdPASS) {
                sd_cmd_reply_t reply = {.err = ESP_OK, .len = 0, .next_offset = 0, .end_of_file = 0, .is_old_file = 0};
                switch (cmd.op) {

                    // use when -> reads a contiguous byte range
                    case SD_CMD_READ_RANGE: {
                        fflush(s_fd);       // make sure file tail is visible
                       
                        reply.is_old_file = cmd.read_file < currentDate_YYYYMMDD ? true : false; // if old file can check end of file
                        /* open read file */
                        char fileName[64];
                        snprintf(fileName, sizeof(fileName), "/sd/logs/%08lu.bin", cmd.read_file);
                        FILE *readFileName = fopen(fileName, "rb");
                        if (!readFileName) { 
                            reply.err = ESP_ERR_FILE_NOT_EXIST; 
                            ESP_LOGW(TAG, "reply, fail open file: %s", fileName);
                            break; 
                        }
                        fseek(readFileName, 0, SEEK_END);               // Moves the file pointer to the end of the file — so new data will be appended.
                        uint32_t file_size = ftell(readFileName);       // Returns the current byte offset of that file pointer (from the beginning of the file). contains the current file size in bytes (how many bytes have already been written).
                        /* check end of file */
                        if(reply.is_old_file == 1){
                            reply.end_of_file = file_size <= (cmd.from_offset + cmd.max_len) ? 1 : 0; 
                        }
                        ESP_LOGI(TAG, "r file %s size: %u, offset: %u, len: %u, EOF: %u, OldFile: %u", fileName, file_size, cmd.from_offset, cmd.max_len, reply.end_of_file, reply.is_old_file);
                        /* check offset more than file size */
                        if(file_size < cmd.from_offset) {
                            reply.err = ESP_ERR_OFFSET; 
                            reply.len = 0;
                            reply.next_offset = file_size;
                            fclose(readFileName);
                            break; 
                        }
                        /* check read length  */
                        uint32_t read_len = file_size - cmd.from_offset;
                        if(read_len > cmd.max_len) read_len = cmd.max_len;
                        /* read data   */
                        fseek(readFileName, cmd.from_offset, SEEK_SET);
                        reply.len = fread(cmd.dst, 1, read_len, readFileName);
                        ESP_LOGI(TAG, "read size: %u", reply.len);
                        reply.next_offset = cmd.from_offset + reply.len;
                        fclose(readFileName);
                    } break;

                    // use when -> power-fail path, 2. Before rotation or before deep sleep / OTA.
                    case SD_CMD_FLUSH_NOW:   
                        // if (n) {
                        //     size_t bytes = n*sizeof(data_log_t);
                        //     if (fwrite(batch, 1, bytes, s_fd)==bytes){
                        //         s_write_offset += bytes; n=0;
                        //     }
                        // }
                        fflush(s_fd); 
                        fsync(fileno(s_fd));
                        break;

                    // use when -> Midnight (date change), File size cap reached (e.g., 128–512 MB), Manual rotation via maintenance API
                    case SD_CMD_ROTATE:
                        // close current, open new file, update offsets
                        break;

                    // use when -> Report the current file size
                    case SD_CMD_STAT:
                        // fill rep with file size, offsets, free space, etc.
                        break;
                }
                xQueueSend(cmd.reply_q, &reply, 0);
            }
        }
        // int stackUnused = uxTaskGetStackHighWaterMark(NULL);
        // ESP_LOGI(TAG, "stack unused = %d", stackUnused);  
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}

bool sdmmc_status(void){
    return sdmmcStatus;
}

/* Load whole file into RAM (null-terminated) */
static char *load_file(const char *path){
    FILE *file = fopen(path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "fail open file %s", path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);   // point to end of file
    long size = ftell(file);    // get file size
    fseek(file, 0, SEEK_SET);   // point to beginning 
    char *buf = malloc(size + 1);  // allocate memory
    if (!buf) {
        fclose(file);
        return NULL;
    }
    // read data to buf
    if (fread(buf, 1, size, file) != (size_t)size) {        
        fclose(file);
        free(buf);
        return NULL;
    }
    buf[size] = 0;  // set 0 to end of file (null-terminated)
    fclose(file);
    return buf;
}

/* Map "H"/"I"/"C"/"D" → MB_PARAM_* 
"reg_type": 
	"H" = Holding register
	"I" = Input register
    "C" = Coil register
    "D" = Discrete register
*/
static mb_param_type_t parse_reg_type(const char *s){
    if (!s) return MB_PARAM_HOLDING;
    if (!strcasecmp(s, "H"))    return MB_PARAM_HOLDING;
    if (!strcasecmp(s, "I"))    return MB_PARAM_INPUT;
    if (!strcasecmp(s, "C"))    return MB_PARAM_COIL;
    if (!strcasecmp(s, "D"))    return MB_PARAM_DISCRETE;
    return MB_PARAM_HOLDING;
}
/* Map "U16_BE", "U32_ABCD", "FLOAT_ABCD" → mb_descr_type_t 
    PARAM_TYPE_IB  = 0,      // Int_BigEndian  				ABCD
    PARAM_TYPE_IBS = 1,      // Int_BigEndian_ByteSwap			CDAB
    PARAM_TYPE_IL  = 2,      // Int_LittleEndian				DCBA
    PARAM_TYPE_ILS = 3,      // Int_LittleEndian_ByteSwap      BADC
    PARAM_TYPE_UB  = 4,      // Uint_BigEndian                  
    PARAM_TYPE_UL  = 5,      // Uint_LittleEndian
    PARAM_TYPE_UBS = 6,      // Uint_BigEndian_Byte Swap
    PARAM_TYPE_ULS = 7,      // Uint_LittleEndian_Byte Swap
    PARAM_TYPE_FB  = 8,      // Float_BigEndian
    PARAM_TYPE_FL  = 9,      // Float_LittleEndian
    PARAM_TYPE_FBS = 10,      // Float_BigEndian_Byte Swap
*/
static modbus_param_type_t parse_paramtype(const char *s){
    if (!s) return PARAM_TYPE_UB;

    // unsigned int
    if (!strcasecmp(s, "UB"))   return PARAM_TYPE_UB;
    if (!strcasecmp(s, "UBS"))  return PARAM_TYPE_UBS;
    if (!strcasecmp(s, "ULS"))  return PARAM_TYPE_ULS;
    if (!strcasecmp(s, "UL"))   return PARAM_TYPE_UL;
    
    //  int
    if (!strcasecmp(s, "IB"))   return PARAM_TYPE_IB;
    if (!strcasecmp(s, "IBS"))  return PARAM_TYPE_IBS;
    if (!strcasecmp(s, "ILS"))  return PARAM_TYPE_ILS;
    if (!strcasecmp(s, "IL"))   return PARAM_TYPE_IL;

    // float
    if (!strcasecmp(s, "FB"))   return PARAM_TYPE_FB;
    if (!strcasecmp(s, "FBS"))  return PARAM_TYPE_FBS;
    if (!strcasecmp(s, "FLS"))  return PARAM_TYPE_FLS;
    if (!strcasecmp(s, "FL"))   return PARAM_TYPE_FL;

    return PARAM_TYPE_UB;    // default
}
/* Map "U16_BE", "U32_ABCD", "FLOAT_ABCD" → mb_descr_type_t 
"datatype": 
	"IB"  = Int_BigEndian  					ABCD
    "IBS" = Int_BigEndian_ByteSwap			CDAB
	"IL"  = Int_LittleEndian				DCBA
	"ILS" = Int_LittleEndian_ByteSwap       BADC
    "UB"  = Uint_BigEndian                  
    "UBS" = Uint_BigEndian_Byte Swap
	"UL"  = Uint_LittleEndian
    "ULS" = Uint_LittleEndian_Byte Swap
	"FB"  = Float_BigEndian
    "FBS" = Float_BigEndian_Byte Swap
	"FL"  = Float_LittleEndian
	"FLS" = Float_LittleEndian_Byte Swap
*/
static mb_descr_type_t parse_datatype(const char *s, uint16_t mb_size){
    if (!s) return PARAM_TYPE_U16;
    /*
        PARAM_TYPE_U16 = 0x01,                  // !< Unsigned 16 
        PARAM_TYPE_ASCII = 0x04,                // !< ASCII type 
        PARAM_TYPE_BIN = 0x07,                  // !< BIN type 
        PARAM_TYPE_I16_AB = 0x0E,               // !< I16 signed integer, big endian 
 
        PARAM_TYPE_I32_ABCD = 0x12,             // !< I32 ABCD signed integer, big endian 
        PARAM_TYPE_I32_CDAB = 0x13,             // !< I32 CDAB signed integer, big endian, reversed register order 
        PARAM_TYPE_I32_BADC = 0x14,             // !< I32 BADC signed integer, little endian, reversed register order 
        PARAM_TYPE_I32_DCBA = 0x15,             // !< I32 DCBA signed integer, little endian 
        PARAM_TYPE_U32_ABCD = 0x16,             // !< U32 ABCD unsigned integer, big endian 
        PARAM_TYPE_U32_CDAB = 0x17,             // !< U32 CDAB unsigned integer, big endian, reversed register order 
        PARAM_TYPE_U32_BADC = 0x18,             // !< U32 BADC unsigned integer, little endian, reversed register order 
        PARAM_TYPE_U32_DCBA = 0x19,             // !< U32 DCBA unsigned integer, little endian 
        PARAM_TYPE_FLOAT_ABCD = 0x1A,           // !< Float ABCD floating point, big endian 
        PARAM_TYPE_FLOAT_CDAB = 0x1B,           // !< Float CDAB floating point big endian, reversed register order 
        PARAM_TYPE_FLOAT_BADC = 0x1C,           // !< Float BADC floating point, little endian, reversed register order 
        PARAM_TYPE_FLOAT_DCBA = 0x1D,           // !< Float DCBA floating point, little endian 
    */
    if(mb_size == 1){
        if (!strcasecmp(s, "UB"))   return PARAM_TYPE_U16;
        if (!strcasecmp(s, "IB"))   return PARAM_TYPE_U16;
        if (!strcasecmp(s, "IL"))   return PARAM_TYPE_U16;
    }else if(mb_size == 2){    
        // unsigned int
        if (!strcasecmp(s, "UB"))   return PARAM_TYPE_U32;
        if (!strcasecmp(s, "UBS"))  return PARAM_TYPE_U32;
        if (!strcasecmp(s, "ULS"))  return PARAM_TYPE_U32;
        if (!strcasecmp(s, "UL"))   return PARAM_TYPE_U32;
        
        //  int
        if (!strcasecmp(s, "IB"))   return PARAM_TYPE_U32;
        if (!strcasecmp(s, "IBS"))  return PARAM_TYPE_U32;
        if (!strcasecmp(s, "ILS"))  return PARAM_TYPE_U32;
        if (!strcasecmp(s, "IL"))   return PARAM_TYPE_U32;

        // float
        if (!strcasecmp(s, "FB"))   return PARAM_TYPE_U32;
        if (!strcasecmp(s, "FBS"))  return PARAM_TYPE_U32;
        if (!strcasecmp(s, "FLS"))  return PARAM_TYPE_U32;
        if (!strcasecmp(s, "FL"))   return PARAM_TYPE_U32;
    }

    return PARAM_TYPE_U16;    // default
}
/* Map "R"/"W"/"RW" → MB_PARAM_* 
"access": 
    "R" = Read
    "W" = Write
    "RW" = Read/Write
*/
static mb_param_perms_t parse_access(const char *s){
    if (!s) return PAR_PERMS_READ;
    if (!strcasecmp(s, "R"))    return PAR_PERMS_READ;
    if (!strcasecmp(s, "W"))    return PAR_PERMS_WRITE;
    if (!strcasecmp(s, "RW"))   return PAR_PERMS_READ_WRITE;
    return PAR_PERMS_READ;
}
/* Size in bytes for given mb_descr_type_t (simplified) */
static uint8_t descr_type_size(mb_descr_type_t t){
    switch (t) {
    case PARAM_TYPE_U8:
    case PARAM_TYPE_I8_A:
    case PARAM_TYPE_I8_B:
    case PARAM_TYPE_U8_A:
    case PARAM_TYPE_U8_B:
        return 1;

    case PARAM_TYPE_U16:
    case PARAM_TYPE_I16_AB:
    case PARAM_TYPE_I16_BA:
    case PARAM_TYPE_U16_AB:
    case PARAM_TYPE_U16_BA:
        return 2;

    case PARAM_TYPE_FLOAT:
    case PARAM_TYPE_U32:
    case PARAM_TYPE_I32_ABCD:
    case PARAM_TYPE_I32_CDAB:
    case PARAM_TYPE_I32_BADC:
    case PARAM_TYPE_I32_DCBA:
    case PARAM_TYPE_U32_ABCD:
    case PARAM_TYPE_U32_CDAB:
    case PARAM_TYPE_U32_BADC:
    case PARAM_TYPE_U32_DCBA:
    case PARAM_TYPE_FLOAT_ABCD:
    case PARAM_TYPE_FLOAT_CDAB:
    case PARAM_TYPE_FLOAT_BADC:
    case PARAM_TYPE_FLOAT_DCBA:
        return 4;

    default:
        return 2;
    }
}
/* Get int or default */
static int get_int_or_default(cJSON *obj, const char *name, int dflt){
    cJSON *it = cJSON_GetObjectItem(obj, name);
    if (!it || !cJSON_IsNumber(it)) return dflt;
    if (it->valuedouble != (double)it->valueint) return dflt;  // It's a non-integer -> float (e.g., 0.1, 5.5)
    return it->valueint;
}
/* Get string or default literal */
static const char *get_str_or_default(cJSON *obj, const char *name, const char *dflt){
    cJSON *it = cJSON_GetObjectItem(obj, name);
    if (!it || !cJSON_IsString(it) || !it->valuestring) return dflt;
    return it->valuestring;
}

esp_err_t load_device_cfg_from_json(const char *json_path){
    size_t need_holding = 0;
    size_t need_input = 0;
    size_t need_coil = 0;
    size_t need_discrete = 0;
    size_t off_holding  = 0;
    size_t off_input    = 0;
    size_t off_coil     = 0;
    size_t off_discrete = 0;
    size_t param_index = 0;

/* 1) Load modbus config file ******/
    char *json_str = load_file(MB_CONF_PATH);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to load %s", MB_CONF_PATH);
        return ESP_FAIL;
    } // ESP_LOGI(TAG, "file loaded %s", MB_CONF_PATH);
    
/* 2) Parse JSON ******/
    cJSON *root = cJSON_Parse(json_str);            // get json object from mb.json
    free(json_str);         // free memory
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        return ESP_FAIL;
    }  //ESP_LOGI(TAG, "JSON parse done");

    cJSON *devices = cJSON_GetObjectItem(root, "devices");   // get json device from root json
    if (!devices || !cJSON_IsArray(devices)) {
        ESP_LOGE(TAG, "\"devices\" array missing");
        cJSON_Delete(root);
        return ESP_FAIL;
    }  

    int dev_count = cJSON_GetArraySize(devices);        // count number of slave in the mb.conf
    if (dev_count <= 0) {
        ESP_LOGE(TAG, "No devices in config");
        cJSON_Delete(root);
        return ESP_FAIL;
    }  
    
/* 3) Allocate memory from heap: cfg_list and g_params base on mb.json file    ******/
    if (cfg_list) { cfg_list = NULL;    // clear pointer
        free(cfg_list);   // returns the block of memory pointed back to the system heap
    }
    if (g_params) { g_params = NULL;  // NOTE: param_key/param_units were strdup'ed; in a real cleanup,  also free those strings here.
        free(g_params);
    }
    if (g_my_param_type) { g_my_param_type = NULL;  // NOTE: param_key/param_units were strdup'ed; in a real cleanup,  also free those strings here.
        free(g_my_param_type);
    }
    cfg_count = dev_count;          // number of slave 
    cfg_list = calloc(cfg_count, sizeof(device_cfg_t));   // allocation heap memory for config list
    g_params_count = 0;         // init all point to zero

    for (int i = 0; i < dev_count; i++) {
        cJSON *dev = cJSON_GetArrayItem(devices, i);            // get device 1 by 1
        cJSON *points = cJSON_GetObjectItem(dev, "points");     // get points from device
        if (!points || !cJSON_IsArray(points)) continue;

        int npts = cJSON_GetArraySize(points);      // count number of point in the device
        g_params_count += npts;                     // count all point list

        // create  dynamic “value pools” for holding_reg_params_t, input_reg_params_t, coil_reg_params_t, discrete_reg_params_t
        for (int j = 0; j < npts; j++) {
            cJSON *p = cJSON_GetArrayItem(points, j);            // get point 1 by 1
            if (!p) continue;
            
            const char *reg_type  = get_str_or_default(p, "reg_type", "H");
            const char *data_type  = get_str_or_default(p, "datatype", "IB");
            mb_param_type_t rtype = parse_reg_type(reg_type);
            uint16_t mb_size = (uint16_t)get_int_or_default(p, "reg_size", 1);
            mb_descr_type_t datatype = parse_datatype(data_type, mb_size);
            uint8_t datatypesize = descr_type_size(datatype);
            
            switch (rtype) {
                case MB_PARAM_HOLDING:
                need_holding += datatypesize;
                break;
                case MB_PARAM_INPUT:
                    need_input += datatypesize;
                    break;
                case MB_PARAM_COIL:
                    need_coil += datatypesize;
                    break;
                case MB_PARAM_DISCRETE:
                    need_discrete += datatypesize;
                    break;
                default:
                    break;
            }
        }
        // fill cfg_list row
        cfg_list[i].dev_id     = (uint8_t)get_int_or_default(dev, "slave_id", 1);       // save slave id to config list
        cfg_list[i].num_points = (uint16_t)npts;                                        // save number of point of each slave to config list
    }
    g_pool_holding  = calloc(need_holding, 1);
    g_pool_input    = calloc(need_input, 1);
    g_pool_coil     = calloc(need_coil, 1);
    g_pool_discrete = calloc(need_discrete, 1);

    g_pool_holding_size  = need_holding;
    g_pool_input_size    = need_input;
    g_pool_coil_size     = need_coil;
    g_pool_discrete_size = need_discrete;
    ESP_LOGI(TAG, "reg size H: %u I: %u C: %u D: %u ", g_pool_holding_size, g_pool_input_size, g_pool_coil_size, g_pool_discrete_size);
    
    if (g_params_count == 0) {      
        ESP_LOGE(TAG, "No points defined");
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    g_params = calloc(g_params_count, sizeof(mb_parameter_descriptor_t));   // allocation heap memory for comfig modbus parameter descriptor
    if (!g_params) {
        ESP_LOGE(TAG, "No memory for g_params");
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    g_my_param_type = calloc(g_params_count, sizeof(modbus_param_type_t));   // allocation heap memory for comfig modbus parameter descriptor
    if (!g_my_param_type) {
        ESP_LOGE(TAG, "No memory for g_my_param_type");
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

/* 4) Parse mb.json to g_params[] to create mb_parameter_descriptor_t   ******/
    for (int i = 0; i < dev_count; i++) {
        cJSON *dev    = cJSON_GetArrayItem(devices, i);             // get device 1 by 1
        uint8_t slave = (uint8_t)get_int_or_default(dev, "slave_id", 1);
        cJSON *points = cJSON_GetObjectItem(dev, "points");         // get point from device
        if (!points || !cJSON_IsArray(points)) continue;

        const char *acc = get_str_or_default(dev, "access", "R");

        int npts = cJSON_GetArraySize(points);                  // get number of point 

        for (int j = 0; j < npts; j++) {
            cJSON *p = cJSON_GetArrayItem(points, j);            // get point 1 by 1
            if (!p) continue;

            mb_parameter_descriptor_t *d = &g_params[param_index];
            modbus_param_type_t *my_param_type = &g_my_param_type[param_index];
            param_index++;
            /***** mapping to mb_parameter_descriptor_t *****
                .cid = CID_HOLD_DATA_0,                     
                .mb_slave_addr = MB_DEVICE_ADDR3,           
                .mb_param_type = MB_PARAM_HOLDING,        
                .mb_reg_start = 0,                          
                .mb_size = 2,                              
                .param_key = STR("TotalEnergy"),                
                .param_units = STR("kWh"),                    
                .param_offset = HOLD_OFFSET(holding_data0),    *****
                .param_type = PARAM_TYPE_U32,              
                .param_size = 4,                            
                .param_opts = OPTS( 0, 0xFFFFFFFF, 100),          
                .access = PAR_PERMS_READ,                   
            */
            int cid = get_int_or_default(p, "cid", -1);
            if (cid < 0) cid = (int)param_index - 1;             // fallback: auto-cid

            d->cid           = (uint16_t)cid;       
            d->mb_slave_addr = slave;                       

            
            const char *name   = get_str_or_default(p, "name", "undefined");
            const char *unit   = get_str_or_default(p, "unit", "-");
            const char *reg_type  = get_str_or_default(p, "reg_type", "H");
            const char *data_type  = get_str_or_default(p, "datatype", "IB");

            *my_param_type = parse_paramtype(data_type);
            // ESP_LOGI(TAG, "cid: %u param type: %u ", cid, my_param_type);

            d->mb_param_type = parse_reg_type(reg_type);
            d->mb_reg_start  = (uint16_t)get_int_or_default(p, "reg_addr", 0);
            d->mb_size       = (uint16_t)get_int_or_default(p, "reg_size", 1);
            d->param_key   = strdup(name);  // NOTE: remember to free at cleanup if needed
            d->param_units = strdup(unit);
            d->param_type = parse_datatype(data_type, d->mb_size);
            d->param_size = descr_type_size(d->param_type);

            // ── Scaling:   min  → param_opts.min   max  → param_opts.max   step → param_opts.step (e.g. scale factor)
            d->param_opts.min  = get_int_or_default(p, "min", 0);   
            d->param_opts.max  = get_int_or_default(p, "max", 0);       // .max type is int 32 bit =  -2,147,483,648 to 2,147,483,647
            d->param_opts.step = get_int_or_default(p, "scale", 1);     
            ESP_LOGI(TAG, "%d max %d min %d step %d", d->cid, d->param_opts.max, d->param_opts.min, d->param_opts.step);
            
            d->access = parse_access(acc);      //  Access mode 
            ESP_LOGI(TAG, "cid: %u access: %u ", cid, d->access);

            // ── param_offset ─────────────────────────────────────────
            switch (d->mb_param_type) {
                case MB_PARAM_HOLDING:
                    d->param_offset = off_holding;
                    off_holding += d->param_size;
                    break;
                case MB_PARAM_INPUT:
                    d->param_offset = off_input;
                    off_input += d->param_size;
                    break;
                case MB_PARAM_COIL:
                    d->param_offset = off_coil;
                    off_coil += d->param_size;
                    break;
                case MB_PARAM_DISCRETE:
                    d->param_offset = off_discrete;
                    off_discrete += d->param_size;
                    break;
                default:
                    d->param_offset = 0;
                    break;
            }
            // ESP_LOGI(TAG, "reg type: %u size: %u mb: %u ", d->param_type, d->param_size, d->mb_size);
        }
    }
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Config loaded: %u devices, %u params", (unsigned)cfg_count, (unsigned)g_params_count);

    return ESP_OK;
}


esp_err_t init_esp_sd_mmc(void){
    esp_err_t err;
    // CONFIG_WL_SECTOR_SIZE
    
    ESP_LOGI(TAG, "init sd card");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 4096         // unit = byte    allocation_unit_size is cluster size needs to be a multiple of sector size of SD-Card it's 512 byte
    };
    ESP_LOGI(TAG, "Mounting SD card - FATFS, with cluster size %d bytes", mount_config.allocation_unit_size);
    sdmmc_card_t *card;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();       // setting host use default 
    
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        slot_config.width = 4;                           // width of data 4 bit full speed 20MHz
        slot_config.cd = SDMMC_SLOT_CD_PIN;             // It is only checked at sdmmc mount time. driver does not handle hot-plug, interrupts, or re-mounts
        slot_config.wp = SDMMC_SLOT_NO_WP;
        slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;   // Enable internal pullups on enabled pins
    
    err = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        sdmmcStatus = false;
        ESP_LOGW(TAG, "sdmmc mounte fail");
        return ESP_FAIL;
    }
    
    sdmmcStatus = true;
    ESP_LOGI(TAG, "mounted %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, card);

    makeDir("/sd/logs");
    makeDir("/sd/conf");

    load_device_cfg_from_json(MB_CONF_PATH);

    xTaskCreate(sd_write_task, "manage sdmmc", 8192, NULL, 7, NULL);

    return ESP_OK;
}


/*
fwrite -> dump data to RAM buffer → stdio buffer
fflush -> dump data to FATFS layer
fsync  -> dump data to Physical SD card

    App RAM (your struct/queue)
        ↓ fwrite()
    C stdio buffer (FILE*)
        ↓ fflush()
    FATFS internal cache
        ↓ fsync()
    SD card flash

*/











void write_file(const char *path, char *content){
    ESP_LOGI(TAG, "writing file %s", path);
    FILE *file = fopen(path, "w");
    fputs(content, file);
    fclose(file);
}
void read_file(const char *path){
    ESP_LOGI(TAG, "reading file %s", path);
    FILE *file = fopen(path, "r");
    char buffer[1024];
    fgets(buffer, 1023, file);
    fclose(file);
    // ESP_LOGI(TAG, "file contains: %s", buffer);
}


/* schema */
esp_err_t read_i2c_schema_str(const char *path, i2c_schema_t *_schema, uint8_t readSize){
    ESP_LOGI(TAG, "Read schema: %s", path);
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        ESP_LOGW(TAG, "File not exist: %s", path);
        fclose(file);
        return ESP_FAIL;
    }
    if(fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return ESP_FAIL;
    }   
    if(fread(_schema, sizeof(i2c_schema_t), readSize, file) != readSize) {  // Attempt to read the existing index
        ESP_LOGW(TAG, "Read failed or no existing data");
        fclose(file);
        return ESP_FAIL;
    }
    fclose(file);        // close file
    return ESP_OK;
}
esp_err_t write_i2c_schema_str(const char *path, i2c_schema_t *_schema, uint8_t updateSize){
    ESP_LOGI(TAG, "write i2c schema: %s", path);
    FILE *file = fopen(path, "rb+");
    if (file == NULL) {
        ESP_LOGW(TAG, "File not exist...create: %s", path);
        file = fopen(path, "wb+");
        if (file == NULL){
            ESP_LOGW(TAG, "Open fail: %s", path);
            fclose(file);
            return ESP_FAIL;
        }
    }
    if(fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return ESP_FAIL;
    } 
    if(fwrite(_schema, sizeof(i2c_schema_t), updateSize, file) != updateSize) {
        ESP_LOGW(TAG, "Write failed: %s", path);
        fclose(file);
        return ESP_FAIL;
    }
    fclose(file);        // close file
    return ESP_OK;
}


/* data_log */
// esp_err_t read_data_log_str(const char *path, data_log_t *_data, index_data_t *_index, uint8_t readSize){
//     ESP_LOGI(TAG, "read data log: %s", path);
//     FILE *file = fopen(path, "rb");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "Read data Open fail: %s", path);
//         return ESP_FAIL;
//     }
//     uint32_t _offset = _index->rIndex * sizeof(data_log_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }       
//     if(readSize == 0) readSize = 1;
//     if(fread(_data, sizeof(data_log_t), readSize, file) != readSize) {  // Attempt to read the existing index
//         ESP_LOGW(TAG, "Read failed or no existing data");
//         fclose(file);        // close file
//         return ESP_FAIL;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }
// esp_err_t read_data_log_str(const char *path, data_log_t *_data, uint8_t _id, uint8_t readSize){
//     ESP_LOGI(TAG, "read data log: %s", path);
//     FILE *file = fopen(path, "rb");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "Read data Open fail: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//     uint32_t _offset = indexData[_id].rIndex * sizeof(data_log_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }       
//     if(readSize == 0) readSize = 1;
//     if(fread(_data, sizeof(data_log_t), readSize, file) != readSize) {  // Attempt to read the existing index
//         ESP_LOGW(TAG, "Read failed or no existing data");
//         fclose(file);        // close file
//         return ESP_FAIL;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }
// esp_err_t write_data_log_str(const char *path, const data_log_t *_data, uint8_t _id){
//     ESP_LOGI(TAG, "write data log: %s", path);
//     FILE *file = fopen(path, "rb+");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist...create: %s", path);
//         file = fopen(path, "wb+");
//         if (file == NULL){
//             ESP_LOGW(TAG, "Open fail: %s", path);
//             fclose(file);
//             return ESP_FAIL;
//         }
//     }
//     uint32_t _offset = indexData[_id].wIndex * sizeof(data_log_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }   
//     if (fwrite(_data, sizeof(data_log_t), 1, file) != 1) {
//         ESP_LOGW(TAG, "Write fail: %s", path);
//         fclose(file);        // close file
//         return ESP_FAIL;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }
esp_err_t append_data_log_str(const char *path, const data_log_t *_data){
    ESP_LOGI(TAG, "Append data log: %s", path);
    FILE *file = fopen(path, "ab");
    if (file == NULL) {
        ESP_LOGW(TAG, "Append data open fail: %s", path);
        fclose(file);
        return ESP_FAIL;
    }
    if (fwrite(_data, sizeof(data_log_t), 1, file) != 1) {
        ESP_LOGW(TAG, "Append fail: %s", path);
        fclose(file);        // close file
        return ESP_FAIL;
    }
    fclose(file);        // close file
    return ESP_OK;
}
// esp_err_t read_combine_data_log_str(const char *path, data_log_t *_dataCombined, uint8_t _id){
//     data_log_t _data[COMBINE_SIZE];
//     ESP_LOGI(TAG, "read&combine data log: %s", path);
//   /* open file */
//     FILE *file = fopen(path, "rb");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "Read data Open fail: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//   /* seek file */  
//     uint32_t _offset = indexData[_id].rIndex * sizeof(data_log_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }  
 
//     data_log_t _dataBuffer;
//     uint8_t _count = 0;
//     for(uint8_t i = 0; i < COMBINE_SIZE; i++){
//       /* read file */    
//         if(fread(&_data, sizeof(data_log_t), COMBINE_SIZE, file) != COMBINE_SIZE) {  // Attempt to read the existing index
//             ESP_LOGW(TAG, "Read failed or no existing data");
//             fclose(file);        // close file
//             return ESP_FAIL;
//         }
       
//         for(uint8_t j = 0; j < COMBINE_SIZE; j++){
//             if(_data[j].qly){
//                 _dataBuffer.value += _data[j].value;
//                 _count++;
//             } 
//         }
        
//         if(_count != 0){
//             _dataCombined[i].value = _dataBuffer.value/_count;
//             _dataCombined[i].qly = true;
//         }else{
//             _dataCombined[i].value = 0;
//             _dataCombined[i].qly = false;
//         }
//         _dataBuffer.value = 0;
//         _count = 0;
//         _dataCombined[i].ts = _data[COMBINE_SIZE-1].ts;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }



/* index */
// esp_err_t read_allIndex_str(const char *path, index_t *_index, uint8_t readSize){
//     ESP_LOGI(TAG, "Read all index: %s", path);
//     FILE *file = fopen(path, "rb");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//     if(fseek(file, 0, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }   
//     if(fread(_index, sizeof(index_t), readSize, file) != readSize) {  // Attempt to read the existing index
//         ESP_LOGW(TAG, "Read failed or no existing data");
//         fclose(file);
//         return ESP_FAIL;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }
// esp_err_t write_allIndex_str(const char *path, index_t *_index, uint8_t updateSize){
//     ESP_LOGI(TAG, "Update all Index: %s", path);
//     FILE *file = fopen(path, "rb+");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist...create: %s", path);
//         file = fopen(path, "wb+");
//         if (file == NULL){
//             ESP_LOGW(TAG, "Open fail: %s", path);
//             fclose(file);
//             return ESP_FAIL;
//         }
//     }
//     if(fseek(file, 0, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }       
//     if(fwrite(_index, sizeof(index_t), updateSize, file) != updateSize) {
//         ESP_LOGW(TAG, "Write failed: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }
// esp_err_t clear_index_str(const char *path, uint8_t _id){
//     index_t _index = {0};
//     ESP_LOGI(TAG, "clear index: %s", path);
//     FILE *file = fopen(path, "rb+");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist...create: %s", path);
//         file = fopen(path, "wb+");
//         if (file == NULL){
//             ESP_LOGW(TAG, "Open fail: %s", path);
//             fclose(file);
//             return ESP_FAIL;
//         }
//     }
//     uint32_t _offset = _id * sizeof(index_t);
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }
//     if(fwrite(&_index, sizeof(index_t), 1, file) != 1) {
//         ESP_LOGW(TAG, "Write failed: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//     fclose(file);        // close file
//     return ESP_OK;
// }


// esp_err_t logging_str(const char *path, const data_log_t *_data, uint8_t _id){
//     esp_err_t err;
//     int64_t start_read, end_read;
//   /* append data log to file */  
//     // start_read = esp_timer_get_time();
//     // err = append_data_log_str(path, _data);
//     // if (err != ESP_OK) return ESP_FAIL;
//     // end_read = esp_timer_get_time();
//     // printf("i2c logdata1 %lldus\n", end_read-start_read);

//   /* write data log to file */   
//     start_read = esp_timer_get_time(); 
//     err =  write_data_log_str(path, _data, _id);
//     if (err != ESP_OK) return ESP_FAIL;
//     end_read = esp_timer_get_time();
//     printf("i2c write data %lldus\n", end_read-start_read);

//   /* update index when write success */
//     // start_read = esp_timer_get_time();
//     // err = update_wIndex_str(I2C_INDEX_PATH, _id);
//     // if (err != ESP_OK) return ESP_FAIL;
//     // end_read = esp_timer_get_time();
//     // printf("i2c logdata1 %lldus\n", end_read-start_read);

//     return ESP_OK;
// }


uint16_t getFileSize(const char *path){
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        ESP_LOGW(TAG, "getFileSize Open fail: %s", path);
        fclose(file);
        return 0;
    }
    if(fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }       
    uint16_t size = ftell(file);  // Returns the current position of the file pointer, which at this point is the size of the file in bytes.
    fclose(file);             // Close the file
    return size;
}


esp_err_t remove_file(const char *path){
    ESP_LOGI(TAG, "remove file: %s", path);
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        ESP_LOGI(TAG, "File not exists %s", path);
        fclose(file);
        return ESP_OK; // File not exists
    }
    // File exist
    if (remove(path) != 0){
        ESP_LOGW(TAG, "Remove fail: %s", path);
        fclose(file);
        return ESP_FAIL;
    } 
    ESP_LOGI(TAG, "remove file done: %s", path);
    fclose(file);
    return ESP_OK;
}

bool makeDir(const char *path){
    
    if(isDirExist(path) == ESP_OK){
        printf("dir exist %s\n", path);
        return false;
    }
    
    if(mkdir(path, ACCESSPERMS) == 0){
        printf("create dir: %s\n", path);
        return true;
    }
    return false;
}


esp_err_t isDirExist(const char *path){
    struct stat s;
    stat(path, &s);
    
    if(S_ISDIR(s.st_mode)) return ESP_OK;
    
    return ESP_FAIL;
}



// esp_err_t update_wIndex_str(const char *path, uint8_t _id){
//     index_data_t _index = {0};
//     ESP_LOGI(TAG, "Write index: %s", path);
//     FILE *file = fopen(path, "rb+");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist...create: %s", path);
//         file = fopen(path, "wb+");
//         if (file == NULL){
//             ESP_LOGW(TAG, "Open fail: %s", path);
//             return ESP_FAIL;
//         }
//     }
//     uint32_t _offset = _id * sizeof(index_data_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }       
//     if(fread(&_index, sizeof(index_data_t), 1, file) != 1) {  // Attempt to read the existing index
//         ESP_LOGW(TAG, "Read failed or no existing data");
//         _index.id = _id;  // Assign the index ID for a new entry
//         _index.wIndex = 0; // Initialize the write count
//         _index.rIndex= 0; // Initialize the write count
//         _index.file = 0; // Initialize the write count
//     }
//     _index.wIndex++;   // Modify the index
//     if(fseek(file, _offset, SEEK_SET) != 0) {   // Seek back to the same offset before writing
//         fclose(file);
//         return ESP_FAIL;
//     }
//     if(fwrite(&_index, sizeof(index_data_t), 1, file) != 1) {
//         ESP_LOGW(TAG, "Write failed: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//     ESP_LOGI(TAG, "Update wIndex id: %u, w: %u", _index.id, _index.wIndex);
//     fclose(file);        // close file
//     return ESP_OK;
// }
// esp_err_t read_rIndex_str(const char *path, index_data_t *_index, uint8_t _id){
//     ESP_LOGI(TAG, "Read index: %s", path);
//     FILE *file = fopen(path, "rb");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist: %s", path);
//         return ESP_FAIL;
//     }
//     uint32_t _offset = _id * sizeof(index_data_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }       
//     if(fread(_index, sizeof(index_data_t), 1, file) != 1) {  // Attempt to read the existing index
//         ESP_LOGW(TAG, "Read failed or no existing data");
//         _index->id = _id;  // Assign the index ID for a new entry
//         _index->wIndex = 0; // Initialize the write count
//         _index->rIndex= 0; // Initialize the write count
//         _index->file = 0; // Initialize the write count
//     }
//     ESP_LOGI(TAG, "Read rIndex id: %u, r: %u", _index->id, _index->rIndex);
//     fclose(file);        // close file
//     return ESP_OK;
// }

// esp_err_t update_rIndex_str(const char *path, index_data_t *_index, uint8_t _id){
//     ESP_LOGI(TAG, "Update rIndex: %s", path);
//     FILE *file = fopen(path, "rb+");
//     if (file == NULL) {
//         ESP_LOGW(TAG, "File not exist: %s", path);
//         return ESP_FAIL;
//     }
//     uint32_t _offset = _id * sizeof(index_data_t);      // Calculate the offset for the specified index
//     if(fseek(file, _offset, SEEK_SET) != 0) {
//         fclose(file);
//         return ESP_FAIL;
//     }       
//     if(fwrite(_index, sizeof(index_data_t), 1, file) != 1) {
//         ESP_LOGW(TAG, "Write failed: %s", path);
//         fclose(file);
//         return ESP_FAIL;
//     }
//     ESP_LOGI(TAG, "Update rIndex id: %u, r: %u", _index->id, _index->rIndex);
//     fclose(file);        // close file
//     return ESP_OK;
// }


// {
//     FILE* outfile;
//     // open file for writing
//     outfile = fopen("person.bin", "wb");
//     if (outfile == NULL) {
//         fprintf(stderr, "\nError opened file\n");
//         exit(1);
//     }
//     struct person input1 = { 1, "rohan", "sharma" };
//     // write struct to file
//     int flag = 0;
//     flag = fwrite(&input1, sizeof(struct person), 1, outfile);
//     if (flag) {
//         printf("Contents of the structure written successfully");
//     }
//     else
//         printf("Error Writing to File!");
//     // close file
//     fclose(outfile);
//     return 0;
// }


// {
//     FILE* infile;
//     // Open person.dat for reading
//     infile = fopen("person1.dat", "wb+");
//     if (infile == NULL) {
//         fprintf(stderr, "\nError opening file\n");
//         exit(1);
//     }
//     struct person write_struct = { 1, "Rohan", "Sharma" };
//     // writing to file
//     fwrite(&write_struct, sizeof(write_struct), 1, infile);
//     struct person read_struct;
//     // setting pointer to start of the file
//     rewind(infile);
//     // reading to read_struct
//     fread(&read_struct, sizeof(read_struct), 1, infile);
//     printf("Name: %s %s \nID: %d", read_struct.fname,
//            read_struct.lname, read_struct.id);
//     // close file
//     fclose(infile);
//     return 0;
// }


// static esp_err_t s_example_write_file(const char *path, char *data)
// {
//     ESP_LOGI(TAG, "Opening file %s", path);
//     FILE *f = fopen(path, "w");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for writing");
//         return ESP_FAIL;
//     }
//     fprintf(f, data);
//     fclose(f);
//     ESP_LOGI(TAG, "File written");

//     return ESP_OK;
// }

// static esp_err_t s_example_read_file(const char *path)
// {
//     ESP_LOGI(TAG, "Reading file %s", path);
//     FILE *f = fopen(path, "r");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for reading");
//         return ESP_FAIL;
//     }
//     char line[EXAMPLE_MAX_CHAR_SIZE];
//     fgets(line, sizeof(line), f);
//     fclose(f);

//     // strip newline
//     char *pos = strchr(line, '\n');
//     if (pos) {
//         *pos = '\0';
//     }
//     ESP_LOGI(TAG, "Read from file: '%s'", line);

//     return ESP_OK;
// }



// DMA_ATTR static char long_text[1024];
//       memset(&long_text, 'c', 1023);
      
//       int64_t start_write = esp_timer_get_time();             // start timer

//       FILE *file = fopen("/store/text.txt", "w");
//       fputs(long_text, file);
//       fclose(file);

//       int64_t start_read = esp_timer_get_time(); 

//       file = fopen("/store/text.txt", "r");
//       fgets(long_text, 1023, file);
//       fclose(file);
//       int64_t end_read = esp_timer_get_time(); 
//       // printf("read %s\n", long_text);

//       printf("write time %lldus, read time %lldus\n", start_read - start_write, end_read - start_read);

//       esp_vfs_fat_sdcard_unmount("/store", card);



// void read_file(char *path){
//     ESP_LOGI(TAG, "reading file %s", path);
//     FILE *file = fopen(path, "r");
//     char buffer[100];
//     fgets(buffer, 99, file);
//     fclose(file);
//     ESP_LOGI(TAG, "file contains: %s", buffer);
// }

// void write_file(char *path, char *content){
//     ESP_LOGI(TAG, "writing \"%s\" to %s", content, path);
//     FILE *file = fopen(path, "w");
//     fputs(content, file);
//     fclose(file);
// }

  /*****  sd spi config *****/
      
      // esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      //   .format_if_mount_failed = true,
      //   .max_files = 5,
      //   .allocation_unit_size = 16 * 1024       // 1 sector = 16 KB 
      // };

      // spi_bus_config_t spi_bus_config = {         // config pin
      //   .mosi_io_num = PIN_MOSI, 
      //   .miso_io_num = PIN_MISO,
      //   .sclk_io_num = PIN_CLK,
      //   .quadhd_io_num = -1,                 // quad hd
      //   .quadwp_io_num = -1                  // write protection
      // };

      // sdmmc_host_t host = SDSPI_HOST_DEFAULT();       // setting host
      // ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA));
      
      // sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
      // slot_config.gpio_cs = PIN_CS;           // redefine cs pin
      // slot_config.host_id = host.slot;        // make sure to use same slot with host

      // sdmmc_card_t *card;

      // ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount("/store", &host, &slot_config, &mount_config, &card));
      // sdmmc_card_print_info(stdout, card);
   