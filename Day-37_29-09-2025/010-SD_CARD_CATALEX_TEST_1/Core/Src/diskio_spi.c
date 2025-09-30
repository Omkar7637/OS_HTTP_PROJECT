/*
 * diskio_spi.c
 *
 *  Created on: Sep 30, 2025
 *      Author: Omkar
 */


#include "diskio.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "ff.h"
/* SPI SD card definitions */
#define SD_CS_LOW()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define SD_CS_HIGH()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;

static uint8_t SPI_Transmit(uint8_t data) {
    uint8_t recv;
    HAL_SPI_TransmitReceive(&hspi1, &data, &recv, 1, HAL_MAX_DELAY);
    return recv;
}

DSTATUS SD_disk_initialize(void) {
    SD_CS_HIGH();
    HAL_Delay(1);
    // Add your SD card init sequence here
    // For basic FATFS test, return 0
    return 0;
}

DSTATUS SD_disk_status(void) {
    return 0; // 0 = OK
}

DRESULT SD_disk_read(BYTE *buff, DWORD sector, UINT count) {
    // Add SPI read sector implementation here
    // For testing, fill buffer with 0
    memset(buff, 0, 512*count);
    return RES_OK;
}

DRESULT SD_disk_write(const BYTE *buff, DWORD sector, UINT count) {
    // Add SPI write sector implementation here
    return RES_OK;
}

DRESULT SD_disk_ioctl(BYTE cmd, void *buff) {
    return RES_OK;
}
