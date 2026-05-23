/*
 * SPDX-FileCopyrightText: 2016-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*=====================================================================================
 * Description:
 *   The Modbus parameter structures used to define Modbus instances that
 *   can be addressed by Modbus protocol. Define these structures per your needs in
 *   your application. Below is just an example of possible parameters.
 *====================================================================================*/
#ifndef _DEVICE_PARAMS
#define _DEVICE_PARAMS

#include <stdint.h>

// This file defines structure of modbus parameters which reflect correspond modbus address space
// for each modbus register type (coils, discreet inputs, holding registers, input registers)
#pragma pack(push, 1)
typedef struct
{
    uint8_t discrete_input0:1;
    uint8_t discrete_input1:1;
    uint8_t discrete_input2:1;
    uint8_t discrete_input3:1;
    uint8_t discrete_input4:1;
    uint8_t discrete_input5:1;
    uint8_t discrete_input6:1;
    uint8_t discrete_input7:1;
    uint8_t discrete_input_port1;
    uint8_t discrete_input_port2;
} discrete_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint8_t coils_port0;
    uint8_t coils_port1;
    uint8_t coils_port2;
} coil_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    float input_data0; // 0
    float input_data1; // 2
    float input_data2; // 4
    float input_data3; // 6
    uint16_t data[150]; // 8 + 150 = 158
    float input_data4; // 158
    float input_data5;
    float input_data6;
    float input_data7;
    uint16_t data_block1[150];
} input_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint32_t holding_data0;
    uint32_t holding_data1;
    uint32_t holding_data2;
    uint16_t holding_data3;
    uint16_t holding_data4;
    int16_t  holding_data5;
    uint16_t holding_data6;
    uint16_t holding_data7;
    uint16_t holding_data8;
    uint32_t holding_data9;
    uint32_t holding_data10;
    uint32_t holding_data11;
    uint16_t holding_data12;
    uint16_t holding_data13;
    int16_t  holding_data14;
    uint16_t holding_data15;
    uint16_t holding_data16;
    uint16_t holding_data17;
} holding_reg_params_t;
#pragma pack(pop)

extern holding_reg_params_t holding_reg_params;
extern input_reg_params_t input_reg_params;
extern coil_reg_params_t coil_reg_params;
extern discrete_reg_params_t discrete_reg_params;

#endif // !defined(_DEVICE_PARAMS)




/*  Modbus error codes and troubleshooting
0x106   -  ESP_ERR_NOT_SUPPORTED - Invalid register request - slave returned an exception because the requested register is not supported.
0x107   -  ESP_ERR_TIMEOUT - Slave response timeout - Modbus slave did not send response during configured slave response timeout.
0x108   -  ESP_ERR_INVALID_RESPONSE - Received unsupported response from slave or frame check failure. Master can not execute command handler because the command is either not supported or is incorrect.
0x103   -  ESP_ERR_INVALID_STATE - Critical failure or FSM sequence failure or master FSM is busy processing previous request.
*/