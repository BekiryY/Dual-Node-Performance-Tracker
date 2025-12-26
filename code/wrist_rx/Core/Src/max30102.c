/* ========================================
   File: max30102.c
   MAX30102 Pulse Oximeter Sensor Driver
   ======================================== */

#include "max30102.h"
#include <math.h>

static I2C_HandleTypeDef *hi2c_max30102;

#define BUFFER_SIZE 100
static uint32_t ir_buffer[BUFFER_SIZE];
static uint32_t red_buffer[BUFFER_SIZE];
static uint8_t buffer_index = 0;

static uint8_t MAX30102_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return HAL_I2C_Master_Transmit(hi2c_max30102, MAX30102_I2C_ADDR, data, 2, 100);
}

static uint8_t MAX30102_ReadRegister(uint8_t reg, uint8_t *value)
{
    if (HAL_I2C_Master_Transmit(hi2c_max30102, MAX30102_I2C_ADDR, &reg, 1, 100) != HAL_OK)
        return 1;
    return HAL_I2C_Master_Receive(hi2c_max30102, MAX30102_I2C_ADDR, value, 1, 100);
}

uint8_t MAX30102_Init(I2C_HandleTypeDef *hi2c)
{
    hi2c_max30102 = hi2c;
    uint8_t part_id;
    
    HAL_Delay(100);
    
    if (MAX30102_ReadRegister(MAX30102_PART_ID, &part_id) != HAL_OK) {
        return 1;
    }
    
    if (part_id != 0x15) {
        return 1;
    }
    
    MAX30102_WriteRegister(MAX30102_MODE_CONFIG, 0x40);
    HAL_Delay(100);
    
    MAX30102_WriteRegister(MAX30102_FIFO_CONFIG, 0x4F);
    MAX30102_WriteRegister(MAX30102_MODE_CONFIG, 0x03);
    MAX30102_WriteRegister(MAX30102_SPO2_CONFIG, 0x27);
    MAX30102_WriteRegister(MAX30102_LED1_PA, 0x24);
    MAX30102_WriteRegister(MAX30102_LED2_PA, 0x24);
    MAX30102_WriteRegister(MAX30102_INT_ENABLE_1, 0xE0);
    
    HAL_Delay(100);
    return 0;
}

uint8_t MAX30102_ReadFIFO(uint32_t *ir_value, uint32_t *red_value)
{
    uint8_t data[6];
    uint8_t wr_ptr, rd_ptr, num_samples;
    
    MAX30102_ReadRegister(MAX30102_FIFO_WR_PTR, &wr_ptr);
    MAX30102_ReadRegister(MAX30102_FIFO_RD_PTR, &rd_ptr);
    
    num_samples = (wr_ptr - rd_ptr) & 0x1F;
    
    if (num_samples == 0) return 1;
    
    uint8_t reg = MAX30102_FIFO_DATA;
    if (HAL_I2C_Master_Transmit(hi2c_max30102, MAX30102_I2C_ADDR, &reg, 1, 100) != HAL_OK)
        return 1;
    
    if (HAL_I2C_Master_Receive(hi2c_max30102, MAX30102_I2C_ADDR, data, 6, 100) != HAL_OK)
        return 1;
    
    *red_value = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    *red_value &= 0x3FFFF;
    
    *ir_value = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
    *ir_value &= 0x3FFFF;
    
    ir_buffer[buffer_index] = *ir_value;
    red_buffer[buffer_index] = *red_value;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
    
    return 0;
}

float MAX30102_ReadTemperature(void)
{
    uint8_t temp_int, temp_frac;
    MAX30102_WriteRegister(MAX30102_MODE_CONFIG, 0x03);
    HAL_Delay(30);
    MAX30102_ReadRegister(MAX30102_TEMP_INT, &temp_int);
    MAX30102_ReadRegister(MAX30102_TEMP_FRAC, &temp_frac);
    return (float)temp_int + ((float)temp_frac * 0.0625f);
}

void MAX30102_CalculateHeartRate(uint32_t ir_value, int32_t *heart_rate, uint8_t *valid)
{
    static uint32_t last_peak_time = 0;
    static uint32_t last_ir = 0;
    static int32_t accumulated_hr = 0;
    static uint8_t hr_count = 0;
    
    *valid = 0;
    
    if (ir_value < 50000) {
        *heart_rate = 0;
        return;
    }
    
    if (ir_value > last_ir && (ir_value - last_ir) > 1000) {
        uint32_t current_time = HAL_GetTick();
        
        if (last_peak_time != 0) {
            uint32_t interval = current_time - last_peak_time;
            
            if (interval > 300 && interval < 2000) {
                int32_t bpm = 60000 / interval;
                
                if (bpm >= 40 && bpm <= 200) {
                    accumulated_hr += bpm;
                    hr_count++;
                    
                    if (hr_count >= 3) {
                        *heart_rate = accumulated_hr / hr_count;
                        *valid = 1;
                        accumulated_hr = 0;
                        hr_count = 0;
                    }
                }
            }
        }
        last_peak_time = current_time;
    }
    last_ir = ir_value;
}

void MAX30102_CalculateSpO2(uint32_t ir_value, uint32_t red_value, int32_t *spo2, uint8_t *valid)
{
    *valid = 0;
    
    if (ir_value < 50000 || red_value < 50000) {
        *spo2 = 0;
        return;
    }
    
    float ac_red = 0, dc_red = 0, ac_ir = 0, dc_ir = 0;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        dc_red += red_buffer[i];
        dc_ir += ir_buffer[i];
    }
    dc_red /= BUFFER_SIZE;
    dc_ir /= BUFFER_SIZE;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        ac_red += fabs((float)red_buffer[i] - dc_red);
        ac_ir += fabs((float)ir_buffer[i] - dc_ir);
    }
    
    if (dc_red == 0 || dc_ir == 0) {
        *spo2 = 0;
        return;
    }
    
    float R = (ac_red / dc_red) / (ac_ir / dc_ir);
    *spo2 = (int32_t)(-45.060f * R * R + 30.354f * R + 94.845f);
    
    if (*spo2 >= 80 && *spo2 <= 100) {
        *valid = 1;
    } else {
        *spo2 = 0;
    }
}