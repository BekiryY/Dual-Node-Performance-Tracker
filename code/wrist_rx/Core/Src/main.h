/* ========================================
   File: main.c
   STM32F446RE Data Logger - Main Program
   ======================================== */

#include "main.h"
#include "nrf24.h"
#include "max30102.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>

/* Peripheral handles */
SPI_HandleTypeDef hspi1;  // nRF24L01
SPI_HandleTypeDef hspi3;  // SD Card
I2C_HandleTypeDef hi2c1;  // MAX30102
UART_HandleTypeDef huart2; // USB Serial (ST-Link)

/* Application variables */
sentData_t received_data;
uint8_t nrf_data_ready = 0;

/* MAX30102 data */
uint32_t ir_value = 0;
uint32_t red_value = 0;
int32_t heart_rate = 0;
int32_t spo2 = 0;
uint8_t valid_heart_rate = 0;
uint8_t valid_spo2 = 0;

/* SD Card */
FATFS FatFs;
FIL Fil;
FRESULT fres;

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI3_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
void Read_MAX30102_Data(void);
void Save_Combined_Data_To_SD(void);
void Print_Received_Data(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    /* Initialize peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_SPI3_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();
    MX_FATFS_Init();
    
    printf("\r\n========================================\r\n");
    printf("  STM32F446RE Data Logger\r\n");
    printf("  MAX30102 + nRF24L01 + SD Card\r\n");
    printf("========================================\r\n\r\n");
    
    /* Initialize MAX30102 */
    printf("Initializing MAX30102...\r\n");
    if (MAX30102_Init(&hi2c1) == 0) {
        printf("MAX30102 initialized successfully!\r\n");
    } else {
        printf("MAX30102 initialization FAILED!\r\n");
        printf("Check I2C connections and pull-up resistors\r\n");
    }
    
    /* Initialize nRF24L01 */
    printf("Initializing nRF24L01...\r\n");
    nRF24_Init();
    nRF24_SetRFChannel(76);
    nRF24_SetDataRate(nRF24_DR_250kbps);
    nRF24_SetCRCLength(nRF24_CRC_2byte);
    nRF24_SetPALevel(nRF24_PA_0dBm);
    nRF24_SetRXAddress(0, (uint8_t *)"Node1");
    nRF24_SetPayloadSize(sizeof(sentData_t));
    nRF24_RXMode();
    printf("nRF24L01 initialized! Payload size: %d bytes\r\n", sizeof(sentData_t));
    
    /* Mount SD Card */
    printf("Mounting SD Card...\r\n");
    fres = f_mount(&FatFs, "", 1);
    if (fres != FR_OK) {
        printf("SD Card mount FAILED! Error: %d\r\n", fres);
        printf("Continuing without SD card logging...\r\n");
    } else {
        printf("SD Card mounted successfully!\r\n");
        
        /* Create CSV header if file is new */
        fres = f_open(&Fil, "sensor_data.csv", FA_WRITE | FA_OPEN_APPEND);
        if (fres == FR_OK) {
            if (f_size(&Fil) == 0) {
                UINT bw;
                const char *header = "Timestamp,HR,SpO2,IR,Red,StepInitial,";
                f_write(&Fil, header, strlen(header), &bw);
                
                for (int i = 0; i < 5; i++) {
                    char step_header[32];
                    sprintf(step_header, "Step%d_Period,Step%d_Intensity,", i+1, i+1);
                    f_write(&Fil, step_header, strlen(step_header), &bw);
                }
                f_write(&Fil, "Temperature\r\n", 13, &bw);
            }
            f_close(&Fil);
            printf("CSV file ready\r\n");
        }
    }
    
    printf("\r\nSystem Ready! Waiting for data...\r\n");
    printf("Place finger on MAX30102 sensor\r\n\r\n");
    
    uint32_t last_max30102_read = 0;
    uint32_t last_save_time = 0;
    uint32_t led_toggle = 0;
    
    while (1)
    {
        /* Toggle LED to show activity */
        if (HAL_GetTick() - led_toggle >= 500) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // Nucleo LED
            led_toggle = HAL_GetTick();
        }
        
        /* Check for nRF24 data */
        if (nRF24_DataReady()) {
            nRF24_ReadPayload((uint8_t*)&received_data, sizeof(sentData_t));
            nrf_data_ready = 1;
            
            printf("\r\n>>> nRF24 Data Received! <<<\r\n");
            Print_Received_Data();
        }
        
        /* Read MAX30102 every 100ms */
        if (HAL_GetTick() - last_max30102_read >= 100) {
            Read_MAX30102_Data();
            last_max30102_read = HAL_GetTick();
        }
        
        /* Save combined data every 1 second if nRF24 data available */
        if (nrf_data_ready && (HAL_GetTick() - last_save_time >= 1000)) {
            Save_Combined_Data_To_SD();
            last_save_time = HAL_GetTick();
        }
        
        HAL_Delay(10);
    }
}

void Read_MAX30102_Data(void)
{
    if (MAX30102_ReadFIFO(&ir_value, &red_value) == 0) {
        /* Calculate heart rate and SpO2 */
        MAX30102_CalculateHeartRate(ir_value, &heart_rate, &valid_heart_rate);
        MAX30102_CalculateSpO2(ir_value, red_value, &spo2, &valid_spo2);
        
        /* Print only when valid */
        static uint32_t last_print = 0;
        if (HAL_GetTick() - last_print >= 2000 && valid_heart_rate) {
            printf("HR: %ld bpm, SpO2: %ld%%, IR: %lu, Red: %lu\r\n", 
                   heart_rate, spo2, ir_value, red_value);
            last_print = HAL_GetTick();
        }
    }
}

void Print_Received_Data(void)
{
    printf("Step Initial Count: %u\r\n", received_data.step_initial_count);
    printf("Temperature: %.2f C\r\n", received_data.temp);
    printf("Steps Data:\r\n");
    
    for (int i = 0; i < 5; i++) {
        printf("  Step %d: Period=%u, Intensity=%u\r\n", 
               i+1,
               received_data.steps[i].period,
               received_data.steps[i].intensity);
    }
    printf("\r\n");
}

void Save_Combined_Data_To_SD(void)
{
    char buffer[512];
    UINT bytes_written;
    
    fres = f_open(&Fil, "sensor_data.csv", FA_WRITE | FA_OPEN_APPEND);
    
    if (fres == FR_OK) {
        /* Format: Timestamp, HR, SpO2, IR, Red, StepInitial, Steps[0-4], Temp */
        int len = sprintf(buffer, "%lu,%ld,%ld,%lu,%lu,%u,",
                         HAL_GetTick(),
                         heart_rate,
                         spo2,
                         ir_value,
                         red_value,
                         received_data.step_initial_count);
        
        /* Add all step data */
        for (int i = 0; i < 5; i++) {
            len += sprintf(buffer + len, "%u,%u,",
                          received_data.steps[i].period,
                          received_data.steps[i].intensity);
        }
        
        /* Add temperature */
        len += sprintf(buffer + len, "%.2f\r\n", received_data.temp);
        
        /* Write to file */
        f_write(&Fil, buffer, len, &bytes_written);
        f_close(&Fil);
        
        printf("✓ Data saved to SD card (%d bytes)\r\n\r\n", bytes_written);
        nrf_data_ready = 0; // Clear flag after saving
    } else {
        printf("✗ SD Write Error: %d\r\n", fres);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 180;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Activate the Over-Drive mode */
    HAL_PWREx_EnableOverDrive();

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

static void MX_SPI3_Init(void)
{
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi3);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED Pin - PA5 (Nucleo on-board LED) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* nRF24 CE Pin - PC7 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* nRF24 CSN Pin - PA4 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* SD Card CS Pin - PA15 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(100);
    }
}