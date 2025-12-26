/* ========================================
   File: main.h
   STM32F446RE Data Logger - Main Header
   ======================================== */

#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Data structures */
typedef struct __attribute__((packed)) {
    uint16_t period;
    uint16_t intensity;
} StepData_t;

typedef struct __attribute__((packed)) {
    uint16_t step_initial_count;
    StepData_t steps[5];
    float temp;
} sentData_t;

void Error_Handler(void);

#endif /* MAIN_H */