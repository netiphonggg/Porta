#ifndef _I_I2C_H_
#define _I_I2C_H_

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"   
#include "esp_types.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_sntp.h"

#include "i_wifi_connect/i_wifi_connect.h"
#include "i_sd_mmc/i_sd_mmc.h"

#include "i_type_def.h"
#include "i_define.h"

#define SCL_GPIO 25
#define SDA_GPIO 26
#define I2C_FREQ 100000

// Bit masks
#define DI_MASK   0x0F      // P0..P3 inputs
#define DO_MASK   0xF0      // P4..P7 outputs

#define PCF8574_ADDR 0x20   // address of PCF8574

#define AHT20_ADDR   0x38   // address of AHT20
#define AHT20_CMD_MEAS  0xAC
#define AHT20_CMD_DATA0 0x33
#define AHT20_CMD_DATA1 0x00

#define ADS1115_ADDR 0x49   // address of ADS1115
#define ADS1115_CONVER_REG 0x00  // Pointer register - Set to conversion mode (reading)
#define ADS1115_CONFIG_REG 0x01  // Pointer register - Set to config mode (setting)
#define ADS1115_CH1 0b000  // Difference channel 1 (AI0-AI1)
#define ADS1115_CH2 0b011  // Difference channel 2 (AI2-AI3)
#define ADS1115_CH1_S 0b100  // Difference channel 1 (AI0-GND)
#define ADS1115_CH2_S 0b110  // Difference channel 2 (AI2-GND)

#define RV3028_ADDR 0x52     // address of RV3028
//  Time registers
#define RV_SEC   0x00
#define RV_MIN   0x01
#define RV_HOUR  0x02
#define RV_WDAY  0x03
#define RV_DATE  0x04
#define RV_MON   0x05
#define RV_YEAR  0x06

esp_err_t init_esp_i2c(void);
esp_err_t pcf8574_write_do_bit(int channel, bool level);

#endif


/* RV3028
Time registers (BCD):
    0x00 Seconds (0–59)
    0x01 Minutes (0–59)
    0x02 Hours (0–23) (24-hour mode)
    0x03 Weekday (1–7) (optional)
    0x04 Date (1–31)
    0x05 Month (1–12)
    0x06 Year (00–99) → add 2000 for full year

*/