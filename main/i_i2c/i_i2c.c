/* 
Time handle - RTC SNTP NVS
    That covers all cases:
    ✅ power loss → use RTC
    ✅ dead battery → use NVS backup
    ✅ internet available → resync sntp periodically

    Work flow
    [1] Power-on
    [2] Try to read time from RTC (RV3028) → t_rtc
    [3] Try to read time from NVS (last backup) → t_nvs
    [4] Pick max(t_rtc, t_nvs, MIN_VALID_TIME)
    [5] settimeofday(&tv, NULL);   // set to ESP32 system clock
    [6] Start SNTP (esp_sntp_init)
    [7] When SNTP gets valid time:
            - callback on_got_time() fires
            - write new time to RTC and NVS
    [8] Periodically (every 1h):
            - read current time (gettimeofday)
            - update RTC and NVS for redundancy

    🔋 6️⃣ RV-3028 battery life estimate - The RV-3028 is one of the lowest-power RTCs in existence.
        RV-3028: Worst-case current consumption = 100 nA
        CR2025: capacity = 150 mAh
        Lifetime: 150 mA / 100 nA = 150000 hour => more than 15 years
*/
#include "i_i2c.h"

#define TAG  "* I2C"
#define TAG_PCF8574  "* PCF8574"
#define TAG_AHT20    "* AHT20"
#define TAG_ADS1015  "* ADS1015"
#define TAG_RV3028   "* RV3028"
#define LOOP_INTERVAL 2000

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle_PCF8574;
static i2c_master_dev_handle_t dev_handle_AHT20;
static i2c_master_dev_handle_t dev_handle_ADS1015;
static i2c_master_dev_handle_t dev_handle_RV3028;

static uint8_t pcf_shadow = 0xFF;   // Shadow latch of the last byte we wrote (starts all 1's = inputs/high)
static SemaphoreHandle_t pcf_shadow_mutex = NULL;

static time_t min_time_t = 1732924800;
esp_err_t isPCF8574_ok, isAHT20_ok, isADS1115_ok, isRV3028_ok;

/* add PCF8574 to i2c bus */
esp_err_t activatePCF8574(void){  
    i2c_device_config_t i2c_pcf8574_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCF8574_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &i2c_pcf8574_config, &dev_handle_PCF8574);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_PCF8574, "add fail");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_PCF8574, "device added");
    return ESP_OK;
}
/* add AHT20 to i2c bus */
esp_err_t activateAHT20(void){  
    i2c_device_config_t i2c_aht20_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT20_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &i2c_aht20_config, &dev_handle_AHT20);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_AHT20, "add fail");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_AHT20, "device added");
    return ESP_OK;
}
/* add ADS1115 to i2c bus */
esp_err_t activateADS1115(void){  
    i2c_device_config_t i2c_ads1115_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADS1115_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &i2c_ads1115_config, &dev_handle_ADS1015);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_ADS1015, "add fail");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_ADS1015, "device added");
    return ESP_OK;
}
/* add RV3028 to i2c bus */
esp_err_t activateRV3028(void){  
    i2c_device_config_t i2c_rv3028_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RV3028_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &i2c_rv3028_config, &dev_handle_RV3028);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_RV3028, "add fail");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_RV3028, "device added");
    return ESP_OK;
}

// Read the 4 input bits (P0..P3). Returns 0..15 (bit0=P0)
esp_err_t pcf8574_read_di(uint8_t *di4){
    uint8_t v;
    esp_err_t err = i2c_master_receive(dev_handle_PCF8574, &v, 1, 1000/portTICK_PERIOD_MS); 
    if (err != ESP_OK) return err;
    *di4 = v & DI_MASK;
    return ESP_OK;
}
// Write the 4 output bits (P4..P7). Each bit: 1=HIGH-on (released), 0=LOW-off (sink)
esp_err_t pcf8574_write_do(uint8_t do4){
    // keep P0..P3 = 1 (inputs), update only upper nibble
    if (xSemaphoreTake(pcf_shadow_mutex, portMAX_DELAY) == pdTRUE) {   // --- Acquire Mutex (Block until access is granted) ---
        // --- CRITICAL SECTION START ---
        pcf_shadow = (pcf_shadow & ~DO_MASK) | ((do4 << 4) & DO_MASK) | DI_MASK;
        
        xSemaphoreGive(pcf_shadow_mutex); // Release Mutex
        // --- CRITICAL SECTION END ---

        return ESP_OK;
    }
    return ESP_FAIL;
}
// Optionally set individual DO pin (idx 4..7), level 0/1
esp_err_t pcf8574_write_do_bit(int channel, bool level){
    if (channel < 4 || channel > 7) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(pcf_shadow_mutex, portMAX_DELAY) == pdTRUE) {   // --- Acquire Mutex (Block until access is granted) ---
        // --- CRITICAL SECTION START ---
        if (level) pcf_shadow |=  (1u << channel);
        else       pcf_shadow &= ~(1u << channel);
        // Ensure inputs remain 1's
        pcf_shadow |= DI_MASK;
        
        xSemaphoreGive(pcf_shadow_mutex); // Release Mutex
        // --- CRITICAL SECTION END ---

        return ESP_OK;
    }
    return ESP_FAIL;
}

// write 16-bit big-endian to a register
static esp_err_t ads_write_mul_u16(uint8_t reg, uint16_t cfg_reg) {
    uint8_t buf[3] = { reg, (uint8_t)(cfg_reg >> 8), (uint8_t)(cfg_reg & 0xFF) };
    return i2c_master_transmit(dev_handle_ADS1015, buf, sizeof(buf), 1000/portTICK_PERIOD_MS);
}
// write 16-bit big-endian to a register
static esp_err_t ads_write_sig_u16(uint8_t reg) {
    return i2c_master_transmit(dev_handle_ADS1015, &reg, 1, 1000/portTICK_PERIOD_MS);
}
// read 16-bit big-endian from a register
static esp_err_t ads_read_u16(uint8_t reg, uint16_t *out_be) {
    uint8_t rx[2];
    esp_err_t err = i2c_master_receive(dev_handle_ADS1015, rx, 2, 1000/portTICK_PERIOD_MS);
    if (err != ESP_OK) return err;
    *out_be = (((uint16_t)rx[0] << 8) | rx[1]);
    return ESP_OK;
}
// Build config word for single-shot differential read
static uint16_t ads_build_config(uint16_t mux_bits) {
    uint16_t cfg = 0;
    cfg |= (1 << 15);               // OS=1 (start single conversion)
    cfg |= (mux_bits & 0x7) << 12;  // MUX
    cfg |= (0b010 & 0x7) << 9;      // PGA  ±2.048 V (default, LSB 62.5 µV)
    // cfg |= (1 << 8);                // MODE=1 (single-shot)
    cfg |= (0b001 & 0x7) << 5;      // Data Rate   64 SPS (≈15.625 ms typical)
    cfg |= 0x0003;                  // COMP_QUE=11 (disable comparator)
    return cfg;
}
static inline uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static inline uint8_t bcd2bin(uint8_t v) { return (uint8_t)((((v >> 4) & 0x0F) * 10) + (v & 0x0F)); }

/* ---------- Low-level reg access using repeated-start ---------- */
static esp_err_t rv3028_read_regs(uint8_t start_reg, uint8_t *dst, size_t len, TickType_t to_ticks){
    return i2c_master_transmit_receive(dev_handle_RV3028, &start_reg, 1, dst, len, to_ticks);   // Best: use transmit+receive in one call to generate a repeated start
}
static esp_err_t rv3028_write_regs(uint8_t start_reg, const uint8_t *src, size_t len, TickType_t to_ticks){
    uint8_t buf[1 + 16]; // enough for writing a handful of bytes
    if (len > 16) return ESP_ERR_INVALID_ARG;
    buf[0] = start_reg;
    memcpy(&buf[1], src, len);
    return i2c_master_transmit(dev_handle_RV3028, buf, 1 + len, to_ticks);
}
/* ---------- Get time into struct tm ---------- */
esp_err_t rv3028_get_time(struct tm *out){
    if(isRV3028_ok != ESP_OK){
        ESP_LOGW(TAG_RV3028, "device not added'");
        return ESP_FAIL;
    }
    uint8_t b[7];
    esp_err_t err = rv3028_read_regs(RV_SEC, b, sizeof(b), pdMS_TO_TICKS(200));
    if (err != ESP_OK) return err;

    uint8_t sec   = bcd2bin(b[0] & 0x7F);
    uint8_t min   = bcd2bin(b[1] & 0x7F);
    uint8_t hour  = bcd2bin(b[2] & 0x3F);   // 24h
    uint8_t wday  = bcd2bin(b[3] & 0x07);   // 1..7 (optional)
    uint8_t mday  = bcd2bin(b[4] & 0x3F);
    uint8_t mon   = bcd2bin(b[5] & 0x1F);
    uint8_t year  = bcd2bin(b[6]);          // 00..99

    struct tm t = {0};
    t.tm_sec  = sec;
    t.tm_min  = min;
    t.tm_hour = hour;
    t.tm_mday = mday;
    t.tm_mon  = mon - 1;                    // tm: 0..11
    t.tm_year = (2000 + year) - 1900;       // tm: years since 1900
    t.tm_wday = (wday % 7);                 // optional

    *out = t;
    return ESP_OK;
}
/* ---------- Set time from struct tm ---------- */
esp_err_t rv3028_set_time(const struct tm *t){
    if(isRV3028_ok != ESP_OK){
        ESP_LOGW(TAG_RV3028, "device not added'");
        return ESP_FAIL;
    }
    uint8_t wday_1_7 = (t->tm_wday == 0) ? 7 : t->tm_wday;  // optional
    uint8_t b[7] = {
        bin2bcd((uint8_t)t->tm_sec),
        bin2bcd((uint8_t)t->tm_min),
        bin2bcd((uint8_t)t->tm_hour),       // 24h
        bin2bcd((uint8_t)wday_1_7),
        bin2bcd((uint8_t)t->tm_mday),
        bin2bcd((uint8_t)(t->tm_mon + 1)),
        bin2bcd((uint8_t)((t->tm_year + 1900) - 2000))
    };
    return rv3028_write_regs(RV_SEC, b, sizeof(b), pdMS_TO_TICKS(100));
}
// Enable automatic backup switchover (direct mode, safe for coin cell)
esp_err_t rv3028_enable_backup(void){
    if(isRV3028_ok != ESP_OK){
        ESP_LOGW(TAG_RV3028, "device not added'");
        return ESP_FAIL;
    }
    uint8_t reg37, bitBackup;
    uint8_t addr = 0x37;
    esp_err_t err = i2c_master_transmit_receive(dev_handle_RV3028, &addr, 1, &reg37, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return err;

    bitBackup = (reg37 >> 2) & 0x03;
    if(bitBackup == 1){ 
        ESP_LOGI(TAG_RV3028, "BMS: 'Direct Mode'");
        return ESP_OK;
    }

    ESP_LOGI(TAG_RV3028, "Read BMS REG37: %u", reg37);
    // Clear bits 3:2, then set 01 = direct switch mode
    reg37 &= ~(0b11 << 2);
    reg37 |=  (0b01 << 2);

    uint8_t wr[2] = {addr, reg37};
    err = i2c_master_transmit(dev_handle_RV3028, wr, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return err;

    // Now store to EEPROM permanently
    uint8_t cmd[2] = {0x3F, 0xA1};
    err = i2c_master_transmit(dev_handle_RV3028, cmd, 2, pdMS_TO_TICKS(100));
    if (err == ESP_OK) ESP_LOGI(TAG_RV3028, "BMS set to 'Direct' and saved to EEPROM.");
    return err;
}

/* print time  */
void print_time(struct tm *tm_time){
    time_t now = 0;
    now = mktime(tm_time);                                      // convert struct tm to time_t
    
    char time_buffer[50];
    strftime(time_buffer, sizeof(time_buffer), "%c", tm_time);  // %c = Date and time representation  "Sun Aug 19 02:56:02 2012"
    ESP_LOGI(TAG, "epoch: %lld, GMT+7: %s ", now, time_buffer);
}



/* i2c main task separate to 3 loop as follow
        1. for reading  PCF8574, AHT20, ADS1115
        2. for push data to sdmmc to queue
        3. sync esp32 system time to rtc and nvs

*/
void i2cScanTask(void *params){
    uint16_t count = 0, countMax = 10;
    uint16_t countSync = 0, countSyncMax = 150;     // sync loop 5min for test for production 1 hours
    uint8_t id = 0, idMax = 3;   // number of device for loop i2c scan
    esp_err_t err;
    time_t now = 0;
    struct tm t;
    struct tm rtc;
    
    data1_log_t data_log[4];
    data_log[0].device_id = 255;        // humidity
    data_log[1].device_id = 255;        // temp
    data_log[2].device_id = 255;        // analog ch 1
    data_log[3].device_id = 255;        // analog ch 2

    data_log[0].point_id = 201;         // humidity
    data_log[1].point_id = 202;         // temp
    data_log[2].point_id = 203;         // analog ch 1
    data_log[3].point_id = 204;         // analog ch 2

    /* PCF8574 */
    uint8_t _DI, _DO = 0b0000; 
    /* AHT20 */    
    uint8_t meas_Trigger[3] = {AHT20_CMD_MEAS, AHT20_CMD_DATA0, AHT20_CMD_DATA1};
    uint8_t read_data[7];
    bool isMeasCompleted;
    uint32_t humid, temp;
    float multiplier1 = 0.000095367;
    /* ADS1115 */
    bool isFirstRead = true;
    float ads_conv = 0.0000625;    // 2.048/0x7fff
    uint8_t data_log_ads = 2;

    vTaskDelay(pdMS_TO_TICKS(10000));   // waiting everythink ready for 10s 
    ESP_LOGI(TAG, "Start i2c reading...");
    while (true){
        
        // if(sdmmc_status() && sntp_sync_status()){
        if(sdmmc_status()){
            time(&now);

            // rotate i2c device for AHT20, ADS1115
            switch (id){
            case 0:
                if(isPCF8574_ok == ESP_OK){
                    // _DO = ~_DO;
                    // pcf8574_write_do(_DO);
                    if (xSemaphoreTake(pcf_shadow_mutex, portMAX_DELAY) == pdTRUE) {   // --- Acquire Mutex (Block until access is granted) ---
                        err = i2c_master_transmit(dev_handle_PCF8574, &pcf_shadow, 1, 1000/portTICK_PERIOD_MS);
                        // ESP_LOGI(TAG, "pcf_shadow: %u", pcf_shadow);
                        xSemaphoreGive(pcf_shadow_mutex); // Release Mutex 
                        if (err != ESP_OK) { 
                            ESP_LOGW(TAG_PCF8574, "write fail"); 
                        }
                    }

                    pcf8574_read_di(&_DI);     
                    // printf("PCF8574 read: %x\n", _DI);
                }
                break;

            case 1:
                if(isAHT20_ok == ESP_OK){
                    err = i2c_master_transmit(dev_handle_AHT20, meas_Trigger, 3, 1000/portTICK_PERIOD_MS);
                    if (err != ESP_OK) { 
                        ESP_LOGW(TAG_AHT20, "write fail"); 
                    }else{
                        err = i2c_master_receive(dev_handle_AHT20, read_data, 7, 1000/portTICK_PERIOD_MS);
                        if (err != ESP_OK) { 
                            ESP_LOGW(TAG_AHT20, "read fail"); 
                        }else{
                            data_log[0].epoch_s = now;
                            data_log[1].epoch_s = now;
                            isMeasCompleted = read_data[0] >> 7;
                            if(isMeasCompleted){
                                humid = read_data[1];
                                humid = (humid << 8) | read_data[2];
                                humid = (humid << 4) | (read_data[3] >> 4);
                                data_log[0].value = ((float)humid*multiplier1);                   // (Humid/2^20)*100
                                data_log[0].qly = true;

                                temp = (read_data[3] & 0x0F);
                                temp = (temp << 8) | read_data[4];
                                temp = (temp << 8) | read_data[5];
                                data_log[1].value = (((float)temp*multiplier1)*2)-50;              // (temp/2^20)*200-50
                                data_log[1].qly = true;
                                
                                // printf("AHT20 humid: %3.2f, temp: %3.2fC\n", data_log[0].value, data_log[1].value);
                            }else{             
                                data_log[0].qly = false;
                                data_log[1].qly = false;
                                
                                ESP_LOGW(TAG_AHT20, "measurement not complete");
                            }
                        }  
                    }  
                }
                break;

            case 2:
                if(isADS1115_ok == ESP_OK){
                    // init 
                    data_log[data_log_ads].epoch_s = now;
                    data_log[data_log_ads].qly = false;

                    // read conversion register 
                    if(!isFirstRead) {
                        uint16_t u16_raw;
                        if(ads_read_u16(ADS1115_CONVER_REG, &u16_raw) == ESP_OK){
                            data_log[data_log_ads].qly = true;
                            // data_log[data_log_ads].value = (data_log[data_log_ads].value * 0.05) + (ads_conv * u16_raw * 0.95);
                            int16_t i16_raw = (int16_t)u16_raw;
                            data_log[data_log_ads].value = ads_conv * (float)i16_raw;
                            // printf("ADS1115 CH%u_D = %3.2fV, %d\n", data_log_ads-1, data_log[data_log_ads].value, i16_raw);
                           
                        }
                        data_log_ads = data_log_ads == 2 ? 3 : 2;   // switch channel
                    }

                    // write to config register to switch channal 
                    uint16_t cfg = ads_build_config(data_log_ads == 2 ? ADS1115_CH1_S : ADS1115_CH2_S);
                    // printf("ADS1115 config %u\n", cfg);
                    ads_write_mul_u16(ADS1115_CONFIG_REG, cfg);
                    ads_write_sig_u16(ADS1115_CONVER_REG);
                    isFirstRead = false;
                    
                }
                break;

            default:
                ESP_LOGW(TAG, "error id %d not found", id);
                break;
            }

            /**** push data to sdmmc queue ****/
            if(count >= countMax){
                for(uint8_t i=0; i < 4; i++){
                    if (xQueueSend(q_sd_write, &data_log[i], 0) != pdPASS) {
                        ESP_LOGW(TAG, "*sdmmc queue full"); // queue full: count & optionally drop oldest via a ring if needed
                    }
                }
                rv3028_get_time(&rtc);
                print_time(&rtc);

                count = 0;
            }count++;


            /**** time sync  ****/
            if(countSync >= countSyncMax){

                /* handle rv3028 */
                localtime_r(&now, &t);  // UNIX timestamp (time_t) -> calendar time structure (struct tm)
                rv3028_set_time(&t); 
                set_time_to_nvs(now);

                countSync = 0;
            }countSync++;

            if(++id >= idMax) id = 0; // rotate device id
        }

        // int stackUnused = uxTaskGetStackHighWaterMark(NULL);
        // ESP_LOGI(TAG, "scan stack unused = %d", stackUnused);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL)); 
    }   
}


/*   Setting esp32 system time  
[2] Try to read time from RTC (RV3028) → t_rtc
[3] Try to read time from NVS (last backup) → t_nvs
[4] Pick max(t_rtc, t_nvs, MIN_VALID_TIME)
*/
esp_err_t set_esp_time_from_rtc_nvs(void){
    time_t now = 0, rtc_t, nvs_t;
    struct tm rtc;
    struct timeval tv;
    time(&now);

    setenv("TZ", "<+07>-7", 1);        // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    tzset();

    /* get time from rtc  */
    esp_err_t err = rv3028_get_time(&rtc); // read time from rtc   
    if(err != ESP_OK) {         //  if cannot read from rtc then set time from nvs
        ESP_LOGW(TAG, "rtc get time fail"); 
    }
    rtc_t = mktime(&rtc);       // convert struct tm to time_t
    
    /* get time from nvs  */
    nvs_flash_init();           // no harm to call it again
    nvs_handle_t nvs;
    if (nvs_open(NVS_SNTP_KEY, NVS_READWRITE, &nvs) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs get tv fail"); 
        // return ESP_FAIL; 
    }
    if (nvs_get_i64(nvs, "time", &nvs_t) != ESP_OK) { 
        ESP_LOGW(TAG, "nvs open fail");
        // return ESP_FAIL; 
    }
    nvs_close(nvs);

    if(rtc_t > min_time_t || nvs_t > min_time_t){
        tv.tv_sec = rtc_t > nvs_t ? rtc_t : nvs_t;
    }else{
        tv.tv_sec = min_time_t;
    }
    ESP_LOGI(TAG, "time_t: %lld, rtc: %lld, nvs: %lld", tv.tv_sec, rtc_t, nvs_t);
    esp_startup_time = tv.tv_sec;  // record startup time 
    tv.tv_usec = 0;      // set microsec to zero
    settimeofday(&tv, NULL);     // set to ESP32 system clock

    return ESP_OK; 
}

esp_err_t init_esp_i2c(void){
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_GPIO,
        .scl_io_num = SCL_GPIO,
        .glitch_ignore_cnt = 7,     // default ok
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "Create I2C fail"); 
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "I2C bus created"); 

    isPCF8574_ok = activatePCF8574();
    isAHT20_ok = activateAHT20();
    isADS1115_ok = activateADS1115();
    isRV3028_ok = activateRV3028();

    rv3028_enable_backup();
    vTaskDelay(pdMS_TO_TICKS(5)); 
    set_esp_time_from_rtc_nvs();

    pcf_shadow_mutex = xSemaphoreCreateMutex();

    xTaskCreate(i2cScanTask, "i2c scan", 8192, NULL, 3, NULL);   
    
    vTaskDelay(pdMS_TO_TICKS(10));  // delay for preparing i2c device added
    return ESP_OK;
}

    
