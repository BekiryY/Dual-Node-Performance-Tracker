#include "fatfs.h"

// Define the SPI Handle - Must match the one in main.c
extern SPI_HandleTypeDef hspi2;
#define HSPI_SD &hspi2

// SD Card Commands
#define CMD0     (0x40+0)     /* GO_IDLE_STATE */
#define CMD1     (0x40+1)     /* SEND_OP_COND */
#define CMD8     (0x40+8)     /* SEND_IF_COND */
#define CMD9     (0x40+9)     /* SEND_CSD */
#define CMD10    (0x40+10)    /* SEND_CID */
#define CMD12    (0x40+12)    /* STOP_TRANSMISSION */
#define CMD16    (0x40+16)    /* SET_BLOCKLEN */
#define CMD17    (0x40+17)    /* READ_SINGLE_BLOCK */
#define CMD18    (0x40+18)    /* READ_MULTIPLE_BLOCK */
#define CMD23    (0x40+23)    /* SET_BLOCK_COUNT */
#define CMD24    (0x40+24)    /* WRITE_BLOCK */
#define CMD25    (0x40+25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD41    (0x40+41)    /* SEND_OP_COND (ACMD) */
#define CMD55    (0x40+55)    /* APP_CMD */
#define CMD58    (0x40+58)    /* READ_OCR */

// CS Control (Change pin if needed - currently PB12)
#define SD_CS_PORT GPIOB
#define SD_CS_PIN  GPIO_PIN_12

#define SD_CS_LOW()  HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET)
#define SD_CS_HIGH() HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET)

static volatile DSTATUS Stat = STA_NOINIT;
static uint8_t CardType;

// SPI Helper Functions
static uint8_t SPI_RxByte(void) {
    uint8_t dummy, data;
    dummy = 0xFF;
    HAL_SPI_TransmitReceive(HSPI_SD, &dummy, &data, 1, 10);
    return data;
}

static void SPI_TxByte(uint8_t data) {
    uint8_t dummy;
    HAL_SPI_TransmitReceive(HSPI_SD, &data, &dummy, 1, 10);
}

// SD Helper Functions
static uint8_t SD_ReadyWait(void) {
    uint8_t res;
    uint16_t timer = 500; // Timeout
    SPI_RxByte();
    do {
        res = SPI_RxByte();
        if (res == 0xFF) return 0xFF; // Ready
    } while (timer--);
    return 0x00; // Busy
}

static void SD_PowerOn(void) {
    uint8_t cmd_arg[6];
    uint32_t count = 0x1FFF;
    SD_CS_HIGH();
    for(int i = 0; i < 10; i++) SPI_TxByte(0xFF); // 80 dummy clocks
    SD_CS_LOW();
    SD_CS_HIGH();
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t crc, res;

    // ACMD<n> is the command sequence of CMD55-CMD<n>
    if (cmd & 0x80) {
        cmd &= 0x7F;
        res = SD_SendCmd(CMD55, 0);
        if (res > 1) return res;
    }

    SD_CS_LOW();
    if (SD_ReadyWait() != 0xFF) {
        SD_CS_HIGH();
        return 0xFF;
    }

    SPI_TxByte(cmd);
    SPI_TxByte(arg >> 24);
    SPI_TxByte(arg >> 16);
    SPI_TxByte(arg >> 8);
    SPI_TxByte(arg);

    crc = 0x01;
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    SPI_TxByte(crc);

    uint8_t n = 10;
    do {
        res = SPI_RxByte();
    } while ((res & 0x80) && --n);

    return res;
}

// --- PUBLIC FUNCTIONS ---

DSTATUS SD_disk_initialize(BYTE drv) {
    uint8_t n, type, ocr[4];
    if (drv) return STA_NOINIT;

    SD_PowerOn(); // Wake up

    type = 0;
    if (SD_SendCmd(CMD0, 0) == 1) { // Enter Idle state
        if (SD_SendCmd(CMD8, 0x1AA) == 1) { // SDv2
            for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                if (SD_SendCmd(CMD58, 0) == 0) { // Check voltage range
                     for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
                     type = (ocr[1] & 0x40) ? 6 : 12; // SDv2 (Block or Byte address)
                }
                // Wait for initialization
                uint16_t tmr = 1000;
                while (tmr-- && SD_SendCmd(CMD41, 1UL << 30));
                if (tmr && SD_SendCmd(CMD58, 0) == 0) {
                     for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
                     type = (ocr[0] & 0x40) ? 6 : 2; // Check CCS bit
                }
            }
        } else { // SDv1 or MMC
            type = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) <= 1) ? 2 : 1;
            if (type == 1) {
                if (SD_SendCmd(CMD1, 0) > 1) type = 0;
            }
        }
    }
    CardType = type;
    SD_CS_HIGH();
    SPI_RxByte(); // Idle
    return type ? 0 : STA_NOINIT;
}

DSTATUS SD_disk_status(BYTE drv) {
    if (drv) return STA_NOINIT;
    return Stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    if (pdrv || !count) return RES_PARERR;
    if (!(CardType & 4)) sector *= 512; // Convert to byte address if needed

    SD_CS_LOW();
    if (count == 1) { // Single Block
        if ((SD_SendCmd(CMD17, sector) == 0) && (SPI_RxByte() == 0xFE)) { // Start Token
             for(int i=0; i<512; i++) *buff++ = SPI_RxByte();
             SPI_RxByte(); SPI_RxByte(); // CRC
             count = 0;
        }
    } else { // Multiple Block
        if (SD_SendCmd(CMD18, sector) == 0) {
            do {
                if (SPI_RxByte() == 0xFE) {
                     for(int i=0; i<512; i++) *buff++ = SPI_RxByte();
                     SPI_RxByte(); SPI_RxByte();
                } else {
                     break;
                }
            } while (--count);
            SD_SendCmd(CMD12, 0); // Stop transmission
        }
    }
    SD_CS_HIGH();
    SPI_RxByte();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    if (pdrv || !count) return RES_PARERR;
    if (!(CardType & 4)) sector *= 512;

    SD_CS_LOW();
    if (count == 1) { // Single Block
        if (SD_SendCmd(CMD24, sector) == 0) {
            SPI_TxByte(0xFE); // Start Token
            for(int i=0; i<512; i++) SPI_TxByte(*buff++);
            SPI_TxByte(0xFF); SPI_TxByte(0xFF); // Dummy CRC
            if ((SPI_RxByte() & 0x1F) != 0x05) count = 1; // Error
        }
    } else { // Multiple Block
        if (CardType & 2) {
            SD_SendCmd(CMD55, 0); SD_SendCmd(CMD23, count); // ACMD23
        }
        if (SD_SendCmd(CMD25, sector) == 0) {
            do {
                SPI_TxByte(0xFC); // Start Token Multi
                for(int i=0; i<512; i++) SPI_TxByte(*buff++);
                SPI_TxByte(0xFF); SPI_TxByte(0xFF);
                if ((SPI_RxByte() & 0x1F) != 0x05) break;
            } while (--count);
            SPI_TxByte(0xFD); // Stop Token
        }
    }
    SD_CS_HIGH();
    SPI_RxByte();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    DRESULT res = RES_ERROR;
    if (pdrv) return RES_PARERR;
    // Simplified IOCTL (Just return OK for sync)
    if (cmd == CTRL_SYNC) res = RES_OK;
    return res;
}
