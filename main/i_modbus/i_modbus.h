#ifndef _I_MODBUS_H_
#define _I_MODBUS_H_

#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" 
#include "esp_log.h"
#include "esp_timer.h"

#include "mbcontroller.h"       // for mbcontroller defines and api
#include "esp_modbus_master.h"   // master helpers (descriptor API)
#include "driver/uart.h"
#include "i_mb_params.h"        // for modbus parameters structures

#include "i_wifi_connect/i_wifi_connect.h"
#include "i_sd_mmc/i_sd_mmc.h"

#include "i_type_def.h"
#include "i_define.h"

// schema_data_t read_schema_data;
#define MB_PORT_NUM         UART_NUM_2                   // Number of UART port used for Modbus connection
#define MB_DEV_SPEED        9600                // The communication speed of the UART
#define MB_UART_RXD         16
#define MB_UART_TXD         17
#define MB_UART_RTS         21      // DIR

#define MASTER_MAX_CIDS     9       // The number of parameters that intended to be used in the particular control process
#define MASTER_MAX_RETRY    1       // Number of reading of parameters from slave

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (500)
#define UPDATE_CIDS_TIMEOUT_TICS        (pdMS_TO_TICKS(UPDATE_CIDS_TIMEOUT_MS))

// Timeout between polls
#define POLL_TIMEOUT_MS                 (5)
#define POLL_TIMEOUT_TICS               (pdMS_TO_TICKS(POLL_TIMEOUT_MS))

// The macro to get offset for parameter in the appropriate structure
// #define HOLD_OFFSET(field)  ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define HOLD_OFFSET(field)  ((uint16_t)(offsetof(holding_reg_params_t, field)))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field)  ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
#define DISCR_OFFSET(field) ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))        // Discrete offset macro
#define STR(fieldname) ((const char*)( fieldname ))
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

// Enumeration of modbus device(slave) addresses accessed by master device
enum {
    MB_DEVICE_ADDR1 = 1,
    MB_DEVICE_ADDR2 = 2,
    MB_DEVICE_ADDR3 = 3
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum {
    CID_HOLD_DATA_0 = 0,  
    CID_HOLD_DATA_1,
    CID_HOLD_DATA_2,
    CID_HOLD_DATA_3,
    CID_HOLD_DATA_4,
    CID_HOLD_DATA_5,
    CID_HOLD_DATA_6,
    CID_HOLD_DATA_7,
    CID_HOLD_DATA_8,
    CID_HOLD_DATA_9,  
    CID_HOLD_DATA_10,
    CID_HOLD_DATA_11,
    CID_HOLD_DATA_12,
    CID_HOLD_DATA_13,
    CID_HOLD_DATA_14,
    CID_HOLD_DATA_15,
    CID_HOLD_DATA_16,
    CID_HOLD_DATA_17
};

esp_err_t init_esp_modbus(void);
static void master_operation_func(void *arg);

#endif


/*
modbus datatype
"access": 
	"R" = Read
	"W" = Write
	"RW" = Read/Write

"reg_type": 
	"H" = Holding register
	"I" = Input register
	
"datatype": 
	"IB"  = Int_BigEndian  					ABCD
    "IBS" = Int_BigEndian_ByteSwap			CDAB
	"IL"  = Int_LittleEndian				DCBA
	"ILS" = Int_LittleEndian_ByteSwap       BADC
    "UB"  = Uint_BigEndian                  
	"UL"  = Uint_LittleEndian
    "UBS" = Uint_BigEndian_Byte Swap
    "ULS" = Uint_LittleEndian_Byte Swap
	"FB"  = Float_BigEndian
	"FL"  = Float_LittleEndian
    "FBS" = Float_BigEndian_Byte Swap
	"FLS" = Float_LittleEndian_Byte Swap

"Example mdconf.json "
    {
        "serial": {
            "baud": 9600,
            "parity": "N",
            "data_bits": 8,
            "stop_bits": 1
        },
        "devices": [
            {
                "slave_id": 3,
                "name": "PM1",
                "access": ""
                "points": [
                    {
                        "cid": 0,
                        "name": "TotalEnergy",
                        "unit": "kWh",
                        "reg_type": "H",
                        "reg_addr": 0,
                        "reg_size": 2,
                        "datatype": "UB",
                        "scale": 100,
                        "min": 0,
                        "max": 4294967295
                    },
                    {
                        "cid": 1,
                        "name": "ExpEnergy",
                        "unit": "kWh",
                        "reg_type": "H",
                        "reg_addr": 8,
                        "reg_size": 2,
                        "datatype": "UB",
                        "scale": 100,
                        "min": 0,
                        "max": 4294967295
                    },
                    {
                        "cid": 2,
                        "name": "ImpEnergy",
                        "unit": "kWh",
                        "reg_type": "H",
                        "reg_addr": 10,
                        "reg_size": 2,
                        "datatype": "UB",
                        "scale": 100,
                        "min": 0,
                        "max": 4294967295
                    },
                    {
                        "cid": 3,
                        "name": "Voltage",
                        "unit": "V",
                        "reg_type": "H",
                        "reg_addr": 12,
                        "reg_size": 1,
                        "datatype": "UB",
                        "scale": 10,
                        "min": 0,
                        "max": 5000
                    },
                    {
                        "cid": 4,
                        "name": "Current",
                        "unit": "A",
                        "reg_type": "H",
                        "reg_addr": 13,
                        "reg_size": 1,
                        "datatype": "UB",
                        "scale": 100,
                        "min": 0,
                        "max": 1000
                    },
                    {
                        "cid": 5,
                        "name": "ActPower",
                        "unit": "W",
                        "reg_type": "H",
                        "reg_addr": 14,
                        "reg_size": 1,
                        "datatype": "UB",
                        "scale": 1,
                        "min": 0,
                        "max": 4294967295
                    },
                    {
                        "cid": 6,
                        "name": "ReActPower",
                        "unit": "VAr",
                        "reg_type": "H",
                        "reg_addr": 15,
                        "reg_size": 1,
                        "datatype": "UB",
                        "scale": 1,
                        "min": 0,
                        "max": 4294967295
                    },
                    {
                        "cid": 7,
                        "name": "PF",
                        "unit": "-",
                        "reg_type": "H",
                        "reg_addr": 16,
                        "reg_size": 1,
                        "datatype": "UB",
                        "scale": 1000,
                        "min": 0,
                        "max": 2000
                    },
                    {
                        "cid": 8,
                        "name": "Freq",
                        "unit": "Hz",
                        "reg_type": "H",
                        "reg_addr": 17,
                        "reg_size": 1,
                        "datatype": "UB",
                        "scale": 100,
                        "min": 0,
                        "max": 10000
                    }
                ]
            }
        ]
    }



*/
