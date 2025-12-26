/* ========================================
   File: nrf24.h
   nRF24L01 Wireless Transceiver Driver
   ======================================== */

#ifndef NRF24_H_
#define NRF24_H_

#include "stm32f4xx_hal.h"

/* Commands */
#define nRF24_CMD_R_REGISTER    0x00
#define nRF24_CMD_W_REGISTER    0x20
#define nRF24_CMD_R_RX_PAYLOAD  0x61
#define nRF24_CMD_W_TX_PAYLOAD  0xA0
#define nRF24_CMD_FLUSH_TX      0xE1
#define nRF24_CMD_FLUSH_RX      0xE2
#define nRF24_CMD_NOP           0xFF

/* Registers */
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

/* Configuration bits */
#define nRF24_CONFIG_PWR_UP     0x02
#define nRF24_CONFIG_PRIM_RX    0x01

/* Status bits */
#define nRF24_STATUS_RX_DR      0x40
#define nRF24_STATUS_TX_DS      0x20
#define nRF24_STATUS_MAX_RT     0x10

/* Data rates */
typedef enum {
    nRF24_DR_250kbps = 0,
    nRF24_DR_1Mbps,
    nRF24_DR_2Mbps
} nRF24_DataRate_t;

/* PA levels */
typedef enum {
    nRF24_PA_m18dBm = 0,
    nRF24_PA_m12dBm,
    nRF24_PA_m6dBm,
    nRF24_PA_0dBm
} nRF24_PALevel_t;

/* CRC length */
typedef enum {
    nRF24_CRC_Disabled = 0,
    nRF24_CRC_1byte,
    nRF24_CRC_2byte
} nRF24_CRCLength_t;

/* Function prototypes */
void nRF24_Init(void);
void nRF24_SetRFChannel(uint8_t channel);
void nRF24_SetDataRate(nRF24_DataRate_t rate);
void nRF24_SetPALevel(nRF24_PALevel_t level);
void nRF24_SetCRCLength(nRF24_CRCLength_t length);
void nRF24_SetRXAddress(uint8_t pipe, uint8_t *address);
void nRF24_SetTXAddress(uint8_t *address);
void nRF24_SetPayloadSize(uint8_t size);
void nRF24_RXMode(void);
void nRF24_TXMode(void);
uint8_t nRF24_DataReady(void);
void nRF24_ReadPayload(uint8_t *data, uint8_t length);
void nRF24_WritePayload(uint8_t *data, uint8_t length);
void nRF24_FlushRX(void);
void nRF24_FlushTX(void);

#endif /* NRF24_H_ */