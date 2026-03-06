/**
 * @file w25q128.c
 * @brief W25Q128 SPI Flash Driver
 */
#include "w25q128.h"

/* ---- Internal helpers ---- */

static inline void cs_low(W25Q_HandleTypeDef *hw) {
  HAL_GPIO_WritePin(hw->cs_port, hw->cs_pin, GPIO_PIN_RESET);
}

static inline void cs_high(W25Q_HandleTypeDef *hw) {
  HAL_GPIO_WritePin(hw->cs_port, hw->cs_pin, GPIO_PIN_SET);
}

static W25Q_Status spi_tx(W25Q_HandleTypeDef *hw, const uint8_t *d,
                          uint16_t n) {
  if (HAL_SPI_Transmit(hw->hspi, (uint8_t *)d, n, 100) != HAL_OK)
    return W25Q_ERR_SPI;
  return W25Q_OK;
}

static W25Q_Status spi_rx(W25Q_HandleTypeDef *hw, uint8_t *d, uint16_t n) {
  if (HAL_SPI_Receive(hw->hspi, d, n, 100) != HAL_OK)
    return W25Q_ERR_SPI;
  return W25Q_OK;
}

static W25Q_Status write_enable(W25Q_HandleTypeDef *hw) {
  uint8_t cmd = W25Q_CMD_WRITE_ENABLE;
  cs_low(hw);
  W25Q_Status st = spi_tx(hw, &cmd, 1);
  cs_high(hw);
  return st;
}

static W25Q_Status read_status(W25Q_HandleTypeDef *hw, uint8_t *sr) {
  uint8_t cmd = W25Q_CMD_READ_STATUS_REG1;
  cs_low(hw);
  W25Q_Status st = spi_tx(hw, &cmd, 1);
  if (st == W25Q_OK)
    st = spi_rx(hw, sr, 1);
  cs_high(hw);
  return st;
}

static W25Q_Status wait_busy(W25Q_HandleTypeDef *hw, uint32_t timeout_ms) {
  uint32_t t0 = HAL_GetTick();
  uint8_t sr;
  do {
    W25Q_Status st = read_status(hw, &sr);
    if (st != W25Q_OK)
      return st;
    if (!(sr & W25Q_SR_BUSY))
      return W25Q_OK;
  } while ((HAL_GetTick() - t0) < timeout_ms);
  return W25Q_ERR_TIMEOUT;
}

/* ---- Public API ---- */

W25Q_Status W25Q_Init(W25Q_HandleTypeDef *hw, SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port, uint16_t cs_pin) {
  hw->hspi = hspi;
  hw->cs_port = cs_port;
  hw->cs_pin = cs_pin;

  /* CS idle high */
  cs_high(hw);

  /* Release from power-down */
  uint8_t cmd = W25Q_CMD_RELEASE_PD;
  cs_low(hw);
  spi_tx(hw, &cmd, 1);
  cs_high(hw);
  HAL_Delay(1); /* tRES1 = 3 µs */

  return W25Q_OK;
}

W25Q_Status W25Q_ReadJEDEC(W25Q_HandleTypeDef *hw, uint8_t id[3]) {
  uint8_t cmd = W25Q_CMD_JEDEC_ID;
  cs_low(hw);
  W25Q_Status st = spi_tx(hw, &cmd, 1);
  if (st == W25Q_OK)
    st = spi_rx(hw, id, 3);
  cs_high(hw);
  return st;
}

W25Q_Status W25Q_Read(W25Q_HandleTypeDef *hw, uint32_t addr, uint8_t *buf,
                      uint32_t len) {
  uint8_t hdr[4];
  hdr[0] = W25Q_CMD_READ_DATA;
  hdr[1] = (addr >> 16) & 0xFF;
  hdr[2] = (addr >> 8) & 0xFF;
  hdr[3] = addr & 0xFF;

  cs_low(hw);
  W25Q_Status st = spi_tx(hw, hdr, 4);

  /* Read in chunks (HAL_SPI_Receive takes uint16_t len) */
  while (st == W25Q_OK && len > 0) {
    uint16_t chunk = (len > 0xFFFF) ? 0xFFFF : (uint16_t)len;
    st = spi_rx(hw, buf, chunk);
    buf += chunk;
    len -= chunk;
  }

  cs_high(hw);
  return st;
}

W25Q_Status W25Q_EraseSector(W25Q_HandleTypeDef *hw, uint32_t addr) {
  W25Q_Status st = write_enable(hw);
  if (st != W25Q_OK)
    return st;

  uint8_t hdr[4];
  hdr[0] = W25Q_CMD_SECTOR_ERASE_4K;
  hdr[1] = (addr >> 16) & 0xFF;
  hdr[2] = (addr >> 8) & 0xFF;
  hdr[3] = addr & 0xFF;

  cs_low(hw);
  st = spi_tx(hw, hdr, 4);
  cs_high(hw);
  if (st != W25Q_OK)
    return st;

  return wait_busy(hw, W25Q_TIMEOUT_SECTOR_ER);
}

W25Q_Status W25Q_EraseBlock64(W25Q_HandleTypeDef *hw, uint32_t addr) {
  W25Q_Status st = write_enable(hw);
  if (st != W25Q_OK)
    return st;

  uint8_t hdr[4];
  hdr[0] = W25Q_CMD_BLOCK_ERASE_64K;
  hdr[1] = (addr >> 16) & 0xFF;
  hdr[2] = (addr >> 8) & 0xFF;
  hdr[3] = addr & 0xFF;

  cs_low(hw);
  st = spi_tx(hw, hdr, 4);
  cs_high(hw);
  if (st != W25Q_OK)
    return st;

  return wait_busy(hw, W25Q_TIMEOUT_BLOCK_ER);
}

W25Q_Status W25Q_EraseChip(W25Q_HandleTypeDef *hw) {
  W25Q_Status st = write_enable(hw);
  if (st != W25Q_OK)
    return st;

  uint8_t cmd = W25Q_CMD_CHIP_ERASE;
  cs_low(hw);
  st = spi_tx(hw, &cmd, 1);
  cs_high(hw);
  if (st != W25Q_OK)
    return st;

  return wait_busy(hw, W25Q_TIMEOUT_CHIP_ER);
}

W25Q_Status W25Q_PageProgram(W25Q_HandleTypeDef *hw, uint32_t addr,
                             const uint8_t *data, uint16_t len) {
  if (len == 0 || len > W25Q_PAGE_SIZE)
    return W25Q_ERR_SPI;

  W25Q_Status st = write_enable(hw);
  if (st != W25Q_OK)
    return st;

  uint8_t hdr[4];
  hdr[0] = W25Q_CMD_PAGE_PROGRAM;
  hdr[1] = (addr >> 16) & 0xFF;
  hdr[2] = (addr >> 8) & 0xFF;
  hdr[3] = addr & 0xFF;

  cs_low(hw);
  st = spi_tx(hw, hdr, 4);
  if (st == W25Q_OK)
    st = spi_tx(hw, data, len);
  cs_high(hw);
  if (st != W25Q_OK)
    return st;

  return wait_busy(hw, W25Q_TIMEOUT_PAGE_PROG);
}
