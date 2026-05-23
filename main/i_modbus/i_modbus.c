#include "i_modbus.h"

#define TAG  "* MB"
#define LOOP_INTERVAL 3000

void* master_handler = NULL;

holding_reg_params_t holding_reg_params = { 0 };
input_reg_params_t input_reg_params = { 0 };
coil_reg_params_t coil_reg_params = { 0 };
discrete_reg_params_t discrete_reg_params = { 0 };
data1_log_t data_log[100];

/*
    Example Data (Object) Dictionary for Modbus parameters:
        The CID field in the table must be unique.
        Modbus Slave Addr field defines slave address of the device with correspond parameter.
        Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
        Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
        The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
        Data Type, Data Size specify type of the characteristic and its data size.
        Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
        Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
*/
// const mb_parameter_descriptor_t device_parameters[] = {
//     // { CID,           param_key,              Units,        Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type,      Data Size, Parameter Options,       Access Mode}
//     { 
//         // .cid = CID_HOLD_DATA_0,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 0,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 2,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("TotalEnergy"),                /*!< The key (name) of the parameter */
//         .param_units = STR("kWh"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data0), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U32,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 4,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFFFFFF, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_1,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 8,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 2,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("ExpEnergy"),                /*!< The key (name) of the parameter */
//         .param_units = STR("kWh"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data1), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U32,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 4,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFFFFFF, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_2,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 10,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 2,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("ImpEnergy"),                /*!< The key (name) of the parameter */
//         .param_units = STR("kWh"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data2), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U32,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 4,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFFFFFF, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_3,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 12,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("Voltage"),                /*!< The key (name) of the parameter */
//         .param_units = STR("V"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data3), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 5000, 10),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_4,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 13,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("Current"),                /*!< The key (name) of the parameter */
//         .param_units = STR("A"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data4), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 1000, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_5,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 14,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("AcPower"),                /*!< The key (name) of the parameter */
//         .param_units = STR("W"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data5), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFF, 1),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_6,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 15,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("ReAcPower"),                /*!< The key (name) of the parameter */
//         .param_units = STR("VAr"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data6), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFF, 1),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_7,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 16,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("PF"),                /*!< The key (name) of the parameter */
//         .param_units = STR("-"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data7), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 2000, 1000),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_8,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 17,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("Freq"),                /*!< The key (name) of the parameter */
//         .param_units = STR("Hz"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data8), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 10000, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
    
//     { 
//         .cid = CID_HOLD_DATA_9,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 0,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 2,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("TotalEnergy"),                /*!< The key (name) of the parameter */
//         .param_units = STR("kWh"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data9), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U32,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 4,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFFFFFF, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_10,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 8,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 2,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("ExpEnergy"),                /*!< The key (name) of the parameter */
//         .param_units = STR("kWh"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data10), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U32,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 4,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFFFFFF, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_11,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 10,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 2,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("ImpEnergy"),                /*!< The key (name) of the parameter */
//         .param_units = STR("kWh"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data11), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U32,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 4,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFFFFFF, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_12,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 12,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("Voltage"),                /*!< The key (name) of the parameter */
//         .param_units = STR("V"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data12), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 5000, 10),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_13,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 13,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("Current"),                /*!< The key (name) of the parameter */
//         .param_units = STR("A"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data13), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 1000, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_14,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 14,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("AcPower"),                /*!< The key (name) of the parameter */
//         .param_units = STR("W"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data14), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFF, 1),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_15,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 15,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("ReAcPower"),                /*!< The key (name) of the parameter */
//         .param_units = STR("VAr"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data15), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 0xFFFF, 1),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_16,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 16,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("PF"),                /*!< The key (name) of the parameter */
//         .param_units = STR("-"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data16), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 2000, 1000),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     },
//     { 
//         .cid = CID_HOLD_DATA_17,                     /*!< Characteristic cid */
//         .mb_slave_addr = MB_DEVICE_ADDR3,           /*!< Slave address of device in the Modbus segment */
//         .mb_param_type = MB_PARAM_HOLDING,          /*!< Type of modbus parameter */
//         .mb_reg_start = 17,                          /*!< This is the Modbus register address. This is the 0 based value. */
//         .mb_size = 1,                               /*!< Size of mb parameter in registers */
//         .param_key = STR("Freq"),                /*!< The key (name) of the parameter */
//         .param_units = STR("Hz"),                    /*!< The physical units of the parameter */
//         .param_offset = HOLD_OFFSET(holding_data17), /*!< Parameter name (OFFSET in the parameter structure or address of instance) */
//         .param_type = PARAM_TYPE_U16,               /*!< Float, U8, U16, U32, ASCII, etc. */
//         .param_size = 2,                            /*!< Number of bytes in the parameter. */
//         .param_opts = OPTS( 0, 10000, 100),            /*!< Parameter options used to check limits and etc. */
//         .access = PAR_PERMS_READ,                   /*!< Access permissions based on mode */
//     }
// };

// Calculate number of parameters in the table
// const uint16_t num_dev_params = (sizeof(g_params)/sizeof(g_params[0]));

// Return a pointer to the storage for the given CID based on its descriptor.
// Assumes param_offset is a BYTE offset from the base of the corresponding struct
// (typically filled using offsetof()).
static void *master_get_param_data(const mb_parameter_descriptor_t *pd){
    if (!pd) return NULL;

    // Offsets can be zero (first field), so don't reject 0. Select the correct storage base for this Modbus table.
    uint8_t *base = NULL;
    size_t   base_size = 0;

    switch (pd->mb_param_type) {
        case MB_PARAM_HOLDING:
            base = g_pool_holding;    // point to  holding_reg_params address
            base_size = g_pool_holding_size;   // size of holding register structure
            break;
        case MB_PARAM_INPUT:
            base = g_pool_input;
            base_size = g_pool_input_size;
            break;
        case MB_PARAM_COIL:
            base = g_pool_coil;
            base_size = g_pool_coil_size;
            break;
        case MB_PARAM_DISCRETE:
            base = g_pool_discrete;
            base_size = g_pool_discrete_size;
            break;
        default:
            ESP_LOGE(TAG, "Unknown table type for CID #%u", (unsigned)pd->cid);
            return NULL;
    }

    // Bounds check: param_offset is in bytes; param_size should be the storage size in bytes. If your descriptor doesn't have param_size, replace with a known size for that CID/type.
    const size_t off = (size_t)pd->param_offset;
    const size_t size = (size_t)pd->param_size;   // ensure your descriptor defines this

    if ((off + size) > base_size) {
        ESP_LOGE(TAG, "CID #%u offset out of range: off=%u size=%u base_size=%u", (unsigned)pd->cid, (unsigned)off, (unsigned)size, (unsigned)base_size);
        return NULL;
    }

    return (void *)(base + off); // Do byte-wise pointer arithmetic, return address if array param_offset
}

static uint32_t build_u32_from_regs(const void *dst, modbus_param_type_t t){
    const uint16_t *r = (const uint16_t *)dst;
    uint16_t r0 = r[0];   // first Modbus register
    uint16_t r1 = r[1];   // second Modbus register

    // break into bytes A,B,C,D from Modbus view
    uint8_t D = (r0 >> 8) & 0xFF;
    uint8_t C = (r0     ) & 0xFF;
    uint8_t B = (r1 >> 8) & 0xFF;
    uint8_t A = (r1     ) & 0xFF;

    uint8_t b0, b1, b2, b3; // final byte order in memory

    switch (t) {
        // Int / Uint Big Endian ABCD: A B C D
        case PARAM_TYPE_IB:
        case PARAM_TYPE_UB:
        case PARAM_TYPE_FB:   // float big-endian
            b0 = A; b1 = B; b2 = C; b3 = D;
            break;

        // Big Endian byte swapped CDAB: C D A B
        case PARAM_TYPE_IBS:
        case PARAM_TYPE_UBS:
        case PARAM_TYPE_FBS:
            b0 = C; b1 = D; b2 = A; b3 = B;
            break;

        // Little endian DCBA: D C B A
        case PARAM_TYPE_IL:
        case PARAM_TYPE_UL:
        case PARAM_TYPE_FL:
            b0 = D; b1 = C; b2 = B; b3 = A;
            break;

        // Little endian byte swap BADC: B A D C
        case PARAM_TYPE_ILS:
        case PARAM_TYPE_ULS:
        default:
            b0 = B; b1 = A; b2 = D; b3 = C;
            break;
    }

    // pack into host uint32_t in native byte order
    uint32_t v = ((uint32_t)b0) | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
    return v;
}

void modbusScanTask(void *params){
    esp_err_t err;
    bool isOk = true;
    bool alarm_state = false;
    const mb_parameter_descriptor_t *param_descriptor = NULL;
    uint16_t cid = 0;       // current polling slave id
    uint8_t dtype = 0;      // output: descriptor data type

    time_t now = 0;
    int64_t start, end;
    
    vTaskDelay(pdMS_TO_TICKS(10000));   // waiting everythink ready for 10s 
    ESP_LOGI(TAG, "Start modbus polling... (N=%u)", (unsigned)g_params_count);
    while (true){
        
        // if(sdmmc_status() && sntp_sync_status()){
        if(sdmmc_status()){    
            time(&now);
            err = ESP_OK; // Reset error before each CID
            isOk = true;

            start = esp_timer_get_time();

            // 1) Get descriptor for this CID
            err = mbc_master_get_cid_info(master_handler, cid, &param_descriptor); // Fetch descriptor for this CID
            if (err != ESP_OK || !param_descriptor) {
                ESP_LOGE(TAG, "CID %u: get_cid_info failed: %s", cid, esp_err_to_name(err));
                isOk = false;
            }

            // 2) find where storage location (address) (offset into your structs) Get data storage ptr for this param (must be valid & sized per descriptor)
            void *dst = master_get_param_data(param_descriptor);   // param_offset, dst = address of array that offset already in structure
            if (!dst) {
                ESP_LOGE(TAG, "CID %u (%s): no storage", param_descriptor->cid, param_descriptor->param_key);
                isOk = false;
            }

            // 3) polling slave with access read 
            if(isOk && param_descriptor->access == PAR_PERMS_READ){     
                err = mbc_master_get_parameter(master_handler, cid, (uint8_t *)dst, &dtype);  // Modbus poll call.
                if (err != ESP_OK) {
                    if (err == ESP_ERR_TIMEOUT) {
                        ESP_LOGW(TAG, "CID %u (%s): timeout", param_descriptor->cid, param_descriptor->param_key);
                    } else {
                        ESP_LOGE(TAG, "CID %u (%s): read failed err=0x%x (%s)", param_descriptor->cid, param_descriptor->param_key, (int)err, esp_err_to_name(err));
                    }
                    vTaskDelay(POLL_TIMEOUT_TICS);
                }else{
                    // 4) Interpret by type (your table shows floats for humidity), Interpret & log based on data type from descriptor,(examples — adjust to your descriptor enum names)
                    data_log[cid].device_id = param_descriptor->mb_slave_addr;
                    data_log[cid].point_id = cid;
                    data_log[cid].epoch_s = now;
                    // ESP_LOGI(TAG, "CID %u %s = %u", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_type);
                    float step = (param_descriptor->param_opts.step != 0) ? (float)param_descriptor->param_opts.step : 1.0f; // protect devider by zero
                                    
                    switch (param_descriptor->param_type) {
                        case PARAM_TYPE_U16:
                            switch (g_my_param_type[cid]) {
                                case PARAM_TYPE_IL:
                                case PARAM_TYPE_IB:
                                    int16_t s16 = *(int16_t *)dst;
                                    data_log[cid].value = s16 / step;
                                    ESP_LOGI(TAG, "S16 CID %u %s (%s) = %d", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_units, s16);
                                    if (s16 > (int16_t)param_descriptor->param_opts.max || s16 < (int16_t)param_descriptor->param_opts.min)  alarm_state = true;
                                    break;
                                case PARAM_TYPE_UL:
                                case PARAM_TYPE_UB:
                                    uint16_t u16 = *(uint16_t *)dst;
                                    data_log[cid].value = u16 / step;
                                    ESP_LOGI(TAG, "U16 CID %u %s (%s) = %u", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_units, u16);
                                    if (u16 > (uint16_t)param_descriptor->param_opts.max || u16 < (uint16_t)param_descriptor->param_opts.min) alarm_state = true;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case PARAM_TYPE_U32:
                            uint32_t raw = build_u32_from_regs(dst, g_my_param_type[cid]);
                            switch (g_my_param_type[cid]) {
                                case PARAM_TYPE_IB:
                                case PARAM_TYPE_IBS:
                                case PARAM_TYPE_IL:
                                case PARAM_TYPE_ILS:
                                    int32_t s32;
                                    memcpy(&s32, &raw, sizeof(s32));   // reinterpret as signed
                                    data_log[cid].value = s32 / step;
                                    ESP_LOGI(TAG, "S32 CID %u %s (%s) = %ld", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_units, s32);
                                    if (s32 > (int32_t)param_descriptor->param_opts.max || s32 < (int32_t)param_descriptor->param_opts.min)  alarm_state = true;
                                    break;
                                case PARAM_TYPE_UB:
                                case PARAM_TYPE_UBS:
                                case PARAM_TYPE_UL:
                                case PARAM_TYPE_ULS:
                                    uint32_t u32 = raw;
                                    data_log[cid].value = u32 / step;
                                    ESP_LOGI(TAG, "U32 CID %u %s (%s) = %lu", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_units, u32);
                                    if (u32 > (uint32_t)param_descriptor->param_opts.max || u32 < (uint32_t)param_descriptor->param_opts.min)  alarm_state = true;
                                    break;
                                case PARAM_TYPE_FB:
                                case PARAM_TYPE_FBS:
                                case PARAM_TYPE_FL:
                                case PARAM_TYPE_FLS: 
                                    float temp_f; // assumes descriptor also encodes correct word order conversion
                                    memcpy(&temp_f, &raw, sizeof(temp_f));
                                    data_log[cid].value = temp_f / step;
                                    ESP_LOGI(TAG, "F CID %u %s (%s) = %.2f", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_units, temp_f);
                                    if (temp_f > (float)param_descriptor->param_opts.max || temp_f < (float)param_descriptor->param_opts.min) alarm_state = true;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        
                        default:
                            ESP_LOGW(TAG, "CID %u %s: unknown dtype=%u, size=%u", param_descriptor->cid, param_descriptor->param_key, dtype, param_descriptor->param_size);
                            break;
                        
                    }
                    ESP_LOGI(TAG, "CID %u %s (%s) = %.2f", param_descriptor->cid, param_descriptor->param_key, param_descriptor->param_units, data_log[cid].value);
                }
            }else if(isOk && param_descriptor->access == PAR_PERMS_WRITE){
                float x = (float)start;
                uint32_t raw;
                memcpy(&raw, &x, 4);

                uint16_t r[2];
                // CDAB → C D A B
                r[0] = (raw >> 16) & 0xFFFF;   // CD
                r[1] = (raw      ) & 0xFFFF;   // AB

                err = mbc_master_set_parameter(master_handler, cid, (uint8_t*)r, &dtype);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Write OK! : %0.2f", x);
                } else {
                    ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
                }
            }else{

            }

            if (alarm_state) ESP_LOGW(TAG, "High-Low alarm CID %u (%s).", param_descriptor->cid, param_descriptor->param_key);
            alarm_state = false;  // reset alarm
            end = esp_timer_get_time();
            // ESP_LOGI(TAG, "scan time = %.2f ms", (end - start) / 1000.0);

            /**** push data to sdmmc queue ****/
            if (xQueueSend(q_sd_write, &data_log[cid], 0) != pdPASS) {
                ESP_LOGW(TAG, "*sdmmc queue full");     // queue full: count & optionally drop oldest via a ring if needed
            }
                 
            if(++cid >= g_params_count) cid = 0; // rotate slave id
        }
        // int stackUnused = uxTaskGetStackHighWaterMark(NULL);
        // ESP_LOGI(TAG, "stack unused = %d", stackUnused);  
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL)); 
    } 
}

// Modbus master initialization
static esp_err_t mb_master_init(void){
    esp_err_t err = ESP_OK;

    /* 1) Serial communication options (matches your API: .ser_opts.*) */
    mb_communication_info_t comm = {
        .ser_opts.port = MB_PORT_NUM,       // Serial port number
        .ser_opts.mode = MB_RTU,       // Modbus mode of communication (MB_MODE_RTU or MB_MODE_ASCII)
        .ser_opts.baudrate = MB_DEV_SPEED,  // Modbus communication baud rate
        .ser_opts.parity = MB_PARITY_NONE,  // parity option for serial port
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
    };
    
    /* 2) Create controller (serial) */
    err = mbc_master_create_serial((void*)&comm, &master_handler);
    MB_RETURN_ON_FALSE((err == ESP_OK && master_handler), ESP_ERR_INVALID_STATE, TAG, "mb controller create fail, returns(0x%x).", (int)err);

    /* 3) Set UART pins and RS485 mode BEFORE start */
    //    TXD -> DI, RXD <- RO, RTS -> DE & /RE (tie DE and /RE together)
    err = uart_set_pin(MB_PORT_NUM, MB_UART_TXD, MB_UART_RXD, MB_UART_RTS, UART_PIN_NO_CHANGE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG, "mb serial set pin failure, uart_set_pin() returned (0x%x).", (int)err);
    // Half-duplex RS-485 mode so driver toggles RTS for TX enable
    err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG, "mb serial set mode failure, uart_set_mode() returned (0x%x).", (int)err);
    
    // (Recommended) Ensure RTS=HIGH enables your transceiver (MAX3485-style)
    // If your hardware uses active-low DE, pass 0 here.
    // #ifdef UART_SET_RS485_ACTIVE_LEVEL_SUPPORTED
    // err = uart_set_rs485_active_level(MB_PORT_NUM, 1);
    // MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
    //     "uart_set_rs485_active_level() failed (0x%x).", (int)err);
    // #endif

    vTaskDelay(pdMS_TO_TICKS(5)); // (Optional) Give hardware a tick

    /* 4) Provide your Data Dictionary (CID table) BEFORE start */
    if (g_params_count == 0 || g_params == NULL) return ESP_FAIL; 
    err = mbc_master_set_descriptor(master_handler, &g_params[0], g_params_count);          // you cannot free g_params after calling mbc_master_set_descriptor()
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG, "mbc_master_set_descriptor() failed (0x%x).", (int)err);

    ESP_LOGI(TAG, "dev params, num: %u size: %u size[0] %u", g_params_count, sizeof(g_params), sizeof(g_params[0]));
     

    /* 5) Start the master task (note: this API takes the handler) */
    err = mbc_master_start(master_handler);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG, "mbc_master_start() failed (0x%x).", (int)err);
    ESP_LOGI(TAG, "Modbus master stack initialized...");
    return ESP_OK;

}


esp_err_t init_esp_modbus(void){

    esp_err_t err = mb_master_init();
    if (err != ESP_OK) { 
        ESP_LOGW(TAG, "modbus initialized fail"); 
        return ESP_FAIL;
    }

    xTaskCreate(modbusScanTask, "modbus scan", 3072, NULL, 4, NULL);
    return ESP_OK;
}
