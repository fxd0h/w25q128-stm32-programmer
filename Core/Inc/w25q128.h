/**
 * @file w25q128.h
 * @brief W25Q128 SPI Flash Driver for STM32H743
 */
#ifndef W25Q128_H
#define W25Q128_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* W25Q128 Commands */
#define W25Q_CMD_WRITE_ENABLE 0x06
#define W25Q_CMD_WRITE_DISABLE 0x04
#define W25Q_CMD_READ_STATUS_REG1 0x05
#define W25Q_CMD_READ_STATUS_REG2 0x35
#define W25Q_CMD_WRITE_STATUS_REG 0x01
#define W25Q_CMD_PAGE_PROGRAM 0x02
#define W25Q_CMD_READ_DATA 0x03
#define W25Q_CMD_FAST_READ 0x0B
#define W25Q_CMD_SECTOR_ERASE_4K 0x20
#define W25Q_CMD_BLOCK_ERASE_32K 0x52
#define W25Q_CMD_BLOCK_ERASE_64K 0xD8
#define W25Q_CMD_CHIP_ERASE 0xC7
#define W25Q_CMD_JEDEC_ID 0x9F
#define W25Q_CMD_POWER_DOWN 0xB9
#define W25Q_CMD_RELEASE_PD 0xAB
#define W25Q_CMD_ENABLE_RESET 0x66
#define W25Q_CMD_RESET_DEVICE 0x99

/* W25Q128 Properties */
#define W25Q_PAGE_SIZE 256
#define W25Q_SECTOR_SIZE 4096
#define W25Q_BLOCK_SIZE_32K 32768
#define W25Q_BLOCK_SIZE_64K 65536
#define W25Q_CHIP_SIZE (16 * 1024 * 1024) /* 16 MB */
#define W25Q_NUM_PAGES (W25Q_CHIP_SIZE / W25Q_PAGE_SIZE)
#define W25Q_NUM_SECTORS (W25Q_CHIP_SIZE / W25Q_SECTOR_SIZE)

/* Expected JEDEC ID for W25Q128 */
#define W25Q128_JEDEC_MFR 0xEF /* Winbond */
#define W25Q128_JEDEC_TYPE 0x40
#define W25Q128_JEDEC_CAP 0x18 /* 128 Mbit */

/* Status register bits */
#define W25Q_SR_BUSY 0x01
#define W25Q_SR_WEL 0x02

/* Timeout (ms) */
#define W25Q_TIMEOUT_PAGE_PROG 10
#define W25Q_TIMEOUT_SECTOR_ER 400
#define W25Q_TIMEOUT_BLOCK_ER 2000
#define W25Q_TIMEOUT_CHIP_ER 100000

/* Return codes */
typedef enum {
  W25Q_OK = 0,
  W25Q_ERR_SPI,
  W25Q_ERR_TIMEOUT,
  W25Q_ERR_JEDEC,
} W25Q_Status;

/* Driver handle */
typedef struct {
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
} W25Q_HandleTypeDef;

/* API */
W25Q_Status W25Q_Init(W25Q_HandleTypeDef *hw, SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port, uint16_t cs_pin);
W25Q_Status W25Q_ReadJEDEC(W25Q_HandleTypeDef *hw, uint8_t id[3]);
W25Q_Status W25Q_Read(W25Q_HandleTypeDef *hw, uint32_t addr, uint8_t *buf,
                      uint32_t len);
W25Q_Status W25Q_EraseSector(W25Q_HandleTypeDef *hw, uint32_t addr);
W25Q_Status W25Q_EraseBlock64(W25Q_HandleTypeDef *hw, uint32_t addr);
W25Q_Status W25Q_EraseChip(W25Q_HandleTypeDef *hw);
W25Q_Status W25Q_PageProgram(W25Q_HandleTypeDef *hw, uint32_t addr,
                             const uint8_t *data, uint16_t len);

#endif /* W25Q128_H */
