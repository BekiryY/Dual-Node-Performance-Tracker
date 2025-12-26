/* ========================================
   File: nrf24.c
   nRF24L01 Wireless Transceiver Driver
   ======================================== */

#include "nrf24.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

/* GPIO Pins */
#define nRF24_CE_PORT   GPIOC
#define nRF24_CE_PIN    GPIO_PIN_7
#define nRF24_CSN_PORT  GPIOA
#define nRF24_CSN_PIN   GPIO_PIN_4

/* Helper macros */
#define nRF24_CE_LOW()  HAL_GPIO_WritePin(nRF24_CE_PORT, nRF24_CE_PIN, GPIO_PIN_RESET)
#define nRF24_CE_HIGH() HAL_GPIO_WritePin(nRF24_CE_PORT, nRF24_CE_PIN, GPIO_PIN_SET)
#define nRF24_CSN_LOW() HAL_GPIO_WritePin(nRF24_CSN_PORT, nRF24_CSN_PIN, GPIO_PIN_RESET)
#define nRF24_CSN_HIGH() HAL_GPIO_WritePin(nRF24_CSN_PORT, nRF24_CSN_PIN, GPIO_PIN_SET)

/* Private functions */
static uint8_t nRF24_ReadRegister(uint8_t reg)
{
    uint8_t cmd = nRF24_CMD_R_REGISTER | reg;
    uint8_t value = 0;
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, &value, 1, 100);
    nRF24_CSN_HIGH();
    
    return value;
}

static void nRF24_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {nRF24_CMD_W_REGISTER | reg, value};
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, data, 2, 100);
    nRF24_CSN_HIGH();
}

static void nRF24_WriteRegisterMulti(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t cmd = nRF24_CMD_W_REGISTER | reg;
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, data, length, 100);
    nRF24_CSN_HIGH();
}

void nRF24_Init(void)
{
    nRF24_CE_LOW();
    nRF24_CSN_HIGH();
    HAL_Delay(100);
    
    /* Reset all registers */
    nRF24_WriteRegister(nRF24_REG_CONFIG, 0x08);
    nRF24_WriteRegister(nRF24_REG_EN_AA, 0x3F);
    nRF24_WriteRegister(nRF24_REG_EN_RXADDR, 0x03);
    nRF24_WriteRegister(nRF24_REG_SETUP_AW, 0x03);
    nRF24_WriteRegister(nRF24_REG_SETUP_RETR, 0x03);
    nRF24_WriteRegister(nRF24_REG_RF_CH, 0x02);
    nRF24_WriteRegister(nRF24_REG_RF_SETUP, 0x0E);
    
    /* Clear status flags */
    nRF24_WriteRegister(nRF24_REG_STATUS, 0x70);
    
    /* Flush FIFOs */
    nRF24_FlushRX();
    nRF24_FlushTX();
    
    HAL_Delay(100);
}

void nRF24_SetRFChannel(uint8_t channel)
{
    if (channel <= 125) {
        nRF24_WriteRegister(nRF24_REG_RF_CH, channel);
    }
}

void nRF24_SetDataRate(nRF24_DataRate_t rate)
{
    uint8_t rf_setup = nRF24_ReadRegister(nRF24_REG_RF_SETUP);
    rf_setup &= ~0x28;
    
    if (rate == nRF24_DR_250kbps) {
        rf_setup |= 0x20;
    } else if (rate == nRF24_DR_2Mbps) {
        rf_setup |= 0x08;
    }
    
    nRF24_WriteRegister(nRF24_REG_RF_SETUP, rf_setup);
}

void nRF24_SetPALevel(nRF24_PALevel_t level)
{
    uint8_t rf_setup = nRF24_ReadRegister(nRF24_REG_RF_SETUP);
    rf_setup &= ~0x06;
    rf_setup |= (level << 1);
    nRF24_WriteRegister(nRF24_REG_RF_SETUP, rf_setup);
}

void nRF24_SetCRCLength(nRF24_CRCLength_t length)
{
    uint8_t config = nRF24_ReadRegister(nRF24_REG_CONFIG);
    config &= ~0x0C;
    
    if (length == nRF24_CRC_1byte) {
        config |= 0x08;
    } else if (length == nRF24_CRC_2byte) {
        config |= 0x0C;
    }
    
    nRF24_WriteRegister(nRF24_REG_CONFIG, config);
}

void nRF24_SetRXAddress(uint8_t pipe, uint8_t *address)
{
    if (pipe == 0) {
        nRF24_WriteRegisterMulti(nRF24_REG_RX_ADDR_P0, address, 5);
    }
}

void nRF24_SetTXAddress(uint8_t *address)
{
    nRF24_WriteRegisterMulti(nRF24_REG_TX_ADDR, address, 5);
}

void nRF24_SetPayloadSize(uint8_t size)
{
    if (size <= 32) {
        nRF24_WriteRegister(nRF24_REG_RX_PW_P0, size);
    }
}

void nRF24_RXMode(void)
{
    nRF24_CE_LOW();
    
    uint8_t config = nRF24_ReadRegister(nRF24_REG_CONFIG);
    config |= nRF24_CONFIG_PWR_UP | nRF24_CONFIG_PRIM_RX;
    nRF24_WriteRegister(nRF24_REG_CONFIG, config);
    
    nRF24_WriteRegister(nRF24_REG_STATUS, 0x70);
    nRF24_CE_HIGH();
    HAL_Delay(1);
}

void nRF24_TXMode(void)
{
    nRF24_CE_LOW();
    
    uint8_t config = nRF24_ReadRegister(nRF24_REG_CONFIG);
    config |= nRF24_CONFIG_PWR_UP;
    config &= ~nRF24_CONFIG_PRIM_RX;
    nRF24_WriteRegister(nRF24_REG_CONFIG, config);
    
    nRF24_WriteRegister(nRF24_REG_STATUS, 0x70);
    HAL_Delay(1);
}

uint8_t nRF24_DataReady(void)
{
    uint8_t status = nRF24_ReadRegister(nRF24_REG_STATUS);
    
    if (status & nRF24_STATUS_RX_DR) {
        nRF24_WriteRegister(nRF24_REG_STATUS, nRF24_STATUS_RX_DR);
        return 1;
    }
    
    return 0;
}

void nRF24_ReadPayload(uint8_t *data, uint8_t length)
{
    uint8_t cmd = nRF24_CMD_R_RX_PAYLOAD;
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, data, length, 100);
    nRF24_CSN_HIGH();
    
    nRF24_WriteRegister(nRF24_REG_STATUS, nRF24_STATUS_RX_DR);
}

void nRF24_WritePayload(uint8_t *data, uint8_t length)
{
    uint8_t cmd = nRF24_CMD_W_TX_PAYLOAD;
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, data, length, 100);
    nRF24_CSN_HIGH();
    
    nRF24_CE_HIGH();
    HAL_Delay(1);
    nRF24_CE_LOW();
}

void nRF24_FlushRX(void)
{
    uint8_t cmd = nRF24_CMD_FLUSH_RX;
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    nRF24_CSN_HIGH();
}

void nRF24_FlushTX(void)
{
    uint8_t cmd = nRF24_CMD_FLUSH_TX;
    
    nRF24_CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    nRF24_CSN_HIGH();
}