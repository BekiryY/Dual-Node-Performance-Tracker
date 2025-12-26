/* ========================================
   File: max30102.h
   Description: MAX30102 driver header
   ======================================== */

#ifndef MAX30102_H
#define MAX30102_H

#include "stm32f4xx_hal.h"

/* I2C Address */
#define MAX30102_I2C_ADDR       0xAE  // 0x57 << 1

/* Register Addresses */
#define MAX30102_INT_STATUS_1   0x00
#define MAX30102_INT_STATUS_2   0x01
#define MAX30102_INT_ENABLE_1   0x02
#define MAX30102_INT_ENABLE_2   0x03
#define MAX30102_FIFO_WR_PTR    0x04
#define MAX30102_FIFO_RD_PTR    0x06
#define MAX30102_FIFO_DATA      0x07
#define MAX30102_FIFO_CONFIG    0x08
#define MAX30102_MODE_CONFIG    0x09
#define MAX30102_SPO2_CONFIG    0x0A
#define MAX30102_LED1_PA        0x0C
#define MAX30102_LED2_PA        0x0D
#define MAX30102_TEMP_INT       0x1F
#define MAX30102_TEMP_FRAC      0x20
#define MAX30102_REV_ID         0xFE
#define MAX30102_PART_ID        0xFF

/* Function prototypes */
uint8_t MAX30102_Init(I2C_HandleTypeDef *hi2c);
uint8_t MAX30102_ReadFIFO(uint32_t *ir_value, uint32_t *red_value);
float MAX30102_ReadTemperature(void);
void MAX30102_CalculateHeartRate(uint32_t ir_value, int32_t *heart_rate, uint8_t *valid);
void MAX30102_CalculateSpO2(uint32_t ir_value, uint32_t red_value, int32_t *spo2, uint8_t *valid);

#endif
