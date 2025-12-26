/* ========================================
   File: nrf24.h
   ======================================== */

#ifndef NRF24_H_
#define NRF24_H_

#include "stm32f4xx_hal.h"

#define nRF24_CMD_R_REGISTER    0x00
#define nRF24_CMD_W_REGISTER    0x20
#define nRF24_CMD_R_RX_PAYLOAD  0x61
#define nRF24_CMD_W_TX_PAYLOAD  0xA0
#define nRF24_CMD_FLUSH_TX      0xE1
#define nRF24_CMD_FLUSH_RX      0xE2
#define nRF24_CMD_NOP           0xFF

#define nRF24_REG_CONFIG        0x00
#define nRF24_REG_EN_AA         0x01
#define nRF24_REG_EN_RXADDR     0x02
#define nRF24_REG_SETUP_AW      0x03
#define nRF24_REG_SETUP_RETR    0x04
#define nRF24_REG_RF_CH         0x05
#define nRF24_REG_RF_SETUP      0x06
#define nRF24_REG_STATUS        0x07
#define nRF24_REG_RX_ADDR_P0    0x0A
#define nRF24_REG_TX_ADDR       0x10
#define nRF24_REG_RX_PW_P0      0x11
#define nRF24_REG_FIFO_STATUS   0x17

#define nRF24_CE_PORT   GPIOB
#define nRF24_CE_PIN    GPIO_PIN_0
#define nRF24_CSN_PORT  GPIOA
#define nRF24_CSN_PIN   GPIO_PIN_4

typedef enum {
    nRF24_DR_250kbps = 0x20,
    nRF24_DR_1Mbps = 0x00,
    nRF24_DR_2Mbps = 0x08
} nRF24_DataRate_t;

typedef enum {
    nRF24_PA_m18dBm = 0x00,
    nRF24_PA_m12dBm = 0x02,
    nRF24_PA_m6dBm = 0x04,
    nRF24_PA_0dBm = 0x06
} nRF24_PALevel_t;

typedef enum {
    nRF24_CRC_off = 0x00,
    nRF24_CRC_1byte = 0x08,
    nRF24_CRC_2byte = 0x0C
} nRF24_CRCLength_t;

void nRF24_Init(void);
void nRF24_SetRFChannel(uint8_t channel);
void nRF24_SetDataRate(nRF24_DataRate_t rate);
void nRF24_SetPALevel(nRF24_PALevel_t level);
void nRF24_SetCRCLength(nRF24_CRCLength_t length);
void nRF24_SetRXAddress(uint8_t pipe, uint8_t *address);
void nRF24_SetPayloadSize(uint8_t size);
void nRF24_RXMode(void);
void nRF24_TXMode(void);
uint8_t nRF24_DataReady(void);
void nRF24_ReadPayload(uint8_t *data, uint8_t length);
void nRF24_WritePayload(uint8_t *data, uint8_t length);

#endif
