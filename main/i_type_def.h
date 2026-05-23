#ifndef _I_TYPE_DEF_H_
#define _I_TYPE_DEF_H_

#include "esp_sntp.h"

/****   datatype size in esp32  ****
int      : 32 bit
short    : 16 bit
long     : 32 bit
long long: 64 bit
uint8_t  : 8  bit
uint16_t : 16 bit
uint32_t : 32 bit
uint64_t : 64 bit
int8_t   : 8  bit
int16_t  : 16 bit
int32_t  : 32 bit
int64_t  : 64 bit
float    : 32 bit
*/

/*  schema_data_t    size = 52 byte
    id: 0-255 
    data type "type"
        1: 32 bit signed,       big-endian
        2: 32 bit signed,       little-endian
        3: 32 bit unsigned,     big-endian
        4: 32 bit unsigned,     little-endian
        5: 32 bit float,        big-endian
        6: 32 bit float,        little-endian
        7: 64 bit signed,       big-endian
        8: 64 bit signed,       little-endian
        9: 64 bit unsigned,     big-endian
        10: 64 bit unsigned,    little-endian
        11: 64 bit float,       big-endian
        12: 64 bit float,       little-endian
    quantity: number of register
*/
typedef struct schema_data_t{
    uint8_t id;     // max = 255 id
    char key[30]; 
    uint8_t type;   // data type 
    uint8_t qty;    // quantity
    float multi;    // multiplier
    char unit[10];  
} schema_data_t;

typedef struct i2c_schema_t{
    uint8_t id;     // max = 255 id
    char key[10]; 
    char unit[2];  
} i2c_schema_t;

/*  index_data_t  size = 6 byte
    id: range 0-255 id
    wIndex: write index for -> range max = 0-10080 count (1week every 1min)
    rIndex: read index 
    file: range 0-53 week
        0: log real time data
        1-53: log for a week
*/
// typedef struct index_data_t{        
//     uint8_t id;         // max = 255 id
//     uint16_t wIndex;    
//     uint16_t rIndex; 
//     uint8_t file;
// } index_data_t;
typedef struct index_t{        
    uint16_t wIndex;    
    uint16_t rIndex;    
    uint8_t file;       // read file : bit 15-8, write file : bit 7-0           -> keep data for 3 month  12 week
} index_t;

/*  data1_log_t   size = 16 byte -> max file size = 10080*16 = 161280 bytes
    epoch_s: timestamp in ms
    qly: quality
        0: bad signal
        1: good signal
*/
typedef struct data_log_t{
    time_t ts;
    float value;
    bool qly;        
} data_log_t;
typedef struct {
    time_t   epoch_s;      // 0 until SNTP synced
    float    value;        // engineering units (or store scaled int if you prefer)
    uint16_t point_id;     // 0..N-1 (which sensor/MB CID this is)
    uint8_t device_id;
    uint8_t qly;            // 0=OK, bitfield if you like
} data1_log_t;              // 8+4+2+1+1 = 16 bytes (will pad to 16 on 32-bit ABI)

/* sd card command set */
typedef enum { 
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
    PARAM_TYPE_FLS = 11      // Float_LittleEndian_Byte Swap
} modbus_param_type_t;

typedef struct {
    uint8_t  dev_id;        // Modbus slave address
    uint16_t num_points;    // how many points in this device
} device_cfg_t;


/* sd card command set */
typedef enum { SD_CMD_READ_RANGE, SD_CMD_STAT, SD_CMD_ROTATE, SD_CMD_FLUSH_NOW } sd_cmd_t;
/* sd card command structure */
typedef struct {
    sd_cmd_t op;
    uint32_t read_file;
    uint32_t from_offset;      // absolute file byte offset to start
    uint32_t max_len;          // max bytes to return (e.g., 16KB)
    uint8_t *dst;              // caller-provided buffer
    QueueHandle_t reply_q;     // where SD task posts the result
} sd_cmd_msg_t;
/* sd card reply structure */
typedef struct {
    esp_err_t err;
    uint32_t  len;             // bytes filled into dst
    uint32_t  next_offset;     // handy for caller
    uint16_t  end_of_file;     // 1 = reached file end, 0 = more data available
    uint16_t  is_old_file;     // 1 = old file, 0 = current file
} sd_cmd_reply_t;

#endif
