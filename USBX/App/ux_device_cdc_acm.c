/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_cdc_acm.c
 * @author  MCD Application Team
 * @brief   USBX Device CDC ACM applicative source file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "stm32h7xx_nucleo.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static UX_SLAVE_CLASS_CDC_ACM *cdc_acm = UX_NULL;
static uint8_t rx_buffer[64] __attribute__((
    section(".RAM_D2"))); /* USB FS max packet size, in DMA-accessible RAM */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  USBD_CDC_ACM_Activate
 *         This function is called when insertion of a CDC ACM device.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance) {
  /* USER CODE BEGIN USBD_CDC_ACM_Activate */
  cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *)cdc_acm_instance;
  BSP_LED_On(LED_YELLOW); /* Signal: USB host connected */
  /* USER CODE END USBD_CDC_ACM_Activate */

  return;
}

/**
 * @brief  USBD_CDC_ACM_Deactivate
 *         This function is called when extraction of a CDC ACM device.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance) {
  /* USER CODE BEGIN USBD_CDC_ACM_Deactivate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  cdc_acm = UX_NULL;
  BSP_LED_Off(LED_YELLOW);
  /* USER CODE END USBD_CDC_ACM_Deactivate */

  return;
}

/**
 * @brief  USBD_CDC_ACM_ParameterChange
 *         This function is invoked to inform the application of parameter
 * change.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance) {
  /* USER CODE BEGIN USBD_CDC_ACM_ParameterChange */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  /* USER CODE END USBD_CDC_ACM_ParameterChange */

  return;
}

/* USER CODE BEGIN 2 */
#include "w25q128.h"
#include <string.h>

extern UART_HandleTypeDef hcom_uart[];
extern W25Q_HandleTypeDef hw25q;

/* CDC protocol commands */
#define CMD_JEDEC_ID 0x01
#define CMD_READ 0x02       /* ADDR[3] LEN_HI LEN_LO → data[LEN] */
#define CMD_ERASE_4K 0x03   /* ADDR[3] → ACK */
#define CMD_PAGE_PROG 0x04  /* ADDR[3] LEN DATA[LEN] → ACK */
#define CMD_CHIP_ERASE 0x05 /* → ACK */
#define CMD_ERASE_64K 0x06  /* ADDR[3] → ACK */
#define CMD_SELF_TEST                                                          \
  0x10                /* → ACK + result (internal SPI test, no CDC data)     \
                       */
#define CMD_PING 0xFF /* → "OK" */

#define ACK_OK 0x06
#define ACK_ERR 0x15

/* Buffers in D2 SRAM for USB DMA compatibility */
static uint8_t rxbuf[64] __attribute__((section(".RAM_D2")));
static uint8_t txbuf[512] __attribute__((section(".RAM_D2")));

/* Accumulation buffer — collects USB packets until command is complete */
static uint8_t cmdbuf[512] __attribute__((section(".RAM_D2")));
static uint32_t cmd_pos = 0;

static uint32_t tx_state = 0; /* 0=idle, 1=writing */
static uint32_t tx_len = 0;

/* Debug output via VCP */
static void dbg(const char *s, uint16_t len) {
  HAL_UART_Transmit(&hcom_uart[0], (uint8_t *)s, len, 10);
}

/* Process a received command and prepare response in txbuf */
static void process_cmd(uint8_t *cmd, uint32_t len) {
  W25Q_Status st;

  if (len == 0)
    return;

  switch (cmd[0]) {
  case CMD_PING:
    txbuf[0] = 'O';
    txbuf[1] = 'K';
    tx_len = 2;
    break;

  case CMD_JEDEC_ID: {
    uint8_t id[3];
    st = W25Q_ReadJEDEC(&hw25q, id);
    if (st == W25Q_OK) {
      txbuf[0] = ACK_OK;
      txbuf[1] = id[0];
      txbuf[2] = id[1];
      txbuf[3] = id[2];
      tx_len = 4;
    } else {
      txbuf[0] = ACK_ERR;
      txbuf[1] = (uint8_t)st;
      tx_len = 2;
    }
    break;
  }

  case CMD_READ: {
    /* CMD_READ ADDR[3] LEN_HI LEN_LO */
    if (len < 6) {
      txbuf[0] = ACK_ERR;
      tx_len = 1;
      break;
    }
    uint32_t addr = ((uint32_t)cmd[1] << 16) | ((uint32_t)cmd[2] << 8) | cmd[3];
    uint16_t rlen = ((uint16_t)cmd[4] << 8) | cmd[5];
    if (rlen > sizeof(txbuf) - 1)
      rlen = sizeof(txbuf) - 1;
    txbuf[0] = ACK_OK;
    st = W25Q_Read(&hw25q, addr, &txbuf[1], rlen);
    if (st == W25Q_OK) {
      tx_len = 1 + rlen;
    } else {
      txbuf[0] = ACK_ERR;
      txbuf[1] = (uint8_t)st;
      tx_len = 2;
    }
    break;
  }

  case CMD_ERASE_4K: {
    if (len < 4) {
      txbuf[0] = ACK_ERR;
      tx_len = 1;
      break;
    }
    uint32_t addr = ((uint32_t)cmd[1] << 16) | ((uint32_t)cmd[2] << 8) | cmd[3];
    st = W25Q_EraseSector(&hw25q, addr);
    txbuf[0] = (st == W25Q_OK) ? ACK_OK : ACK_ERR;
    tx_len = 1;
    break;
  }

  case CMD_ERASE_64K: {
    if (len < 4) {
      txbuf[0] = ACK_ERR;
      tx_len = 1;
      break;
    }
    uint32_t addr = ((uint32_t)cmd[1] << 16) | ((uint32_t)cmd[2] << 8) | cmd[3];
    st = W25Q_EraseBlock64(&hw25q, addr);
    txbuf[0] = (st == W25Q_OK) ? ACK_OK : ACK_ERR;
    tx_len = 1;
    break;
  }

  case CMD_PAGE_PROG: {
    /* CMD_PAGE_PROG ADDR[3] LEN DATA[LEN] (LEN=0 means 256) */
    if (len < 5) {
      txbuf[0] = ACK_ERR;
      tx_len = 1;
      break;
    }
    uint32_t addr = ((uint32_t)cmd[1] << 16) | ((uint32_t)cmd[2] << 8) | cmd[3];
    uint16_t plen = cmd[4] ? cmd[4] : 256;
    if (len < (uint32_t)(5 + plen)) {
      txbuf[0] = ACK_ERR;
      tx_len = 1;
      break;
    }
    st = W25Q_PageProgram(&hw25q, addr, &cmd[5], plen);
    txbuf[0] = (st == W25Q_OK) ? ACK_OK : ACK_ERR;
    tx_len = 1;
    break;
  }

  case CMD_CHIP_ERASE:
    dbg("CHIP_ERASE...\r\n", 15);
    st = W25Q_EraseChip(&hw25q);
    txbuf[0] = (st == W25Q_OK) ? ACK_OK : ACK_ERR;
    tx_len = 1;
    dbg("CHIP_ERASE done\r\n", 17);
    break;

  case CMD_SELF_TEST: {
    /* Internal SPI test — no CDC data involved */
    /* Uses sector at 0x1FF000 (last 4K of 2MB, safe area) */
    dbg("SELF_TEST start\r\n", 17);
    uint32_t test_addr = 0x1FF000;
    uint8_t pattern[256];
    uint8_t readback[256];
    uint16_t pass = 0;
    uint16_t fail_at = 0xFFFF;

    /* 1. Erase sector */
    st = W25Q_EraseSector(&hw25q, test_addr);
    if (st != W25Q_OK) {
      dbg("ERASE FAIL\r\n", 11);
      txbuf[0] = ACK_ERR;
      txbuf[1] = 0x01;
      tx_len = 2;
      break;
    }
    dbg("erase ok\r\n", 10);

    /* 2. Verify erased (first 256 bytes) */
    st = W25Q_Read(&hw25q, test_addr, readback, 256);
    if (st != W25Q_OK) {
      txbuf[0] = ACK_ERR;
      txbuf[1] = 0x02;
      tx_len = 2;
      break;
    }
    for (int i = 0; i < 256; i++) {
      if (readback[i] != 0xFF) {
        dbg("ERASE VFY FAIL\r\n", 16);
        txbuf[0] = ACK_ERR;
        txbuf[1] = 0x03;
        tx_len = 2;
        break;
      }
    }
    if (tx_len > 0)
      break; /* error already set */
    dbg("erase vfy ok\r\n", 14);

    /* 3. Write test pattern (4 pages of 256 bytes) */
    for (int pg = 0; pg < 4; pg++) {
      for (int i = 0; i < 256; i++)
        pattern[i] = (uint8_t)((pg * 256 + i) & 0xFF);
      st = W25Q_PageProgram(&hw25q, test_addr + pg * 256, pattern, 256);
      if (st != W25Q_OK) {
        dbg("PROG FAIL\r\n", 11);
        txbuf[0] = ACK_ERR;
        txbuf[1] = 0x04;
        tx_len = 2;
        break;
      }
    }
    if (tx_len > 0)
      break;
    dbg("prog ok\r\n", 9);

    /* 4. Verify written data */
    for (int pg = 0; pg < 4; pg++) {
      st = W25Q_Read(&hw25q, test_addr + pg * 256, readback, 256);
      if (st != W25Q_OK) {
        txbuf[0] = ACK_ERR;
        txbuf[1] = 0x05;
        tx_len = 2;
        break;
      }
      for (int i = 0; i < 256; i++) {
        uint8_t expected = (uint8_t)((pg * 256 + i) & 0xFF);
        if (readback[i] == expected) {
          pass++;
        } else {
          if (fail_at == 0xFFFF)
            fail_at = pg * 256 + i;
        }
      }
    }
    if (tx_len > 0)
      break;

    if (fail_at == 0xFFFF) {
      dbg("SELF_TEST PASS\r\n", 16);
      txbuf[0] = ACK_OK;
      txbuf[1] = (pass >> 8) & 0xFF;
      txbuf[2] = pass & 0xFF;
      tx_len = 3;
    } else {
      dbg("SELF_TEST FAIL\r\n", 16);
      txbuf[0] = ACK_ERR;
      txbuf[1] = 0x06;
      txbuf[2] = (fail_at >> 8) & 0xFF;
      txbuf[3] = fail_at & 0xFF;
      tx_len = 4;
    }
    break;
  }

  default:
    txbuf[0] = ACK_ERR;
    tx_len = 1;
    break;
  }
}

/* Determine expected command length from header bytes.
   Returns 0 if we don't have enough bytes yet to tell. */
static uint32_t expected_cmd_len(const uint8_t *buf, uint32_t have) {
  if (have == 0)
    return 0;
  switch (buf[0]) {
  case CMD_PING:
    return 1;
  case CMD_JEDEC_ID:
    return 1;
  case CMD_CHIP_ERASE:
    return 1;
  case CMD_ERASE_4K:
    return 4;
  case CMD_ERASE_64K:
    return 4;
  case CMD_SELF_TEST:
    return 1;
  case CMD_READ:
    return 6;
  case CMD_PAGE_PROG:
    if (have < 5)
      return 0; /* need header to know data length */
    {
      uint16_t plen = buf[4] ? buf[4] : 256;
      return 5 + plen;
    }
  default:
    return 1;
  }
}

void usbx_cdc_acm_read_write_run(void) {
  if (cdc_acm == UX_NULL)
    return;

  UX_SLAVE_DEVICE *device = &_ux_system_slave->ux_system_slave_device;
  if (device->ux_slave_device_state != UX_DEVICE_CONFIGURED)
    return;

  UINT status;

  /* --- TX: send response if pending --- */
  if (tx_state == 1 && tx_len > 0) {
    ULONG actual = 0;
    status = ux_device_class_cdc_acm_write_run(cdc_acm, txbuf, tx_len, &actual);
    if (status == UX_STATE_NEXT || status < UX_STATE_NEXT) {
      tx_state = 0;
      tx_len = 0;
    }
    return;
  }

  /* --- RX: accumulate USB packets into cmdbuf --- */
  ULONG actual = 0;
  status =
      ux_device_class_cdc_acm_read_run(cdc_acm, rxbuf, sizeof(rxbuf), &actual);
  if (status == UX_STATE_NEXT && actual > 0) {
    uint32_t space = sizeof(cmdbuf) - cmd_pos;
    uint32_t copy = (actual < space) ? (uint32_t)actual : space;
    memcpy(&cmdbuf[cmd_pos], rxbuf, copy);
    cmd_pos += copy;

    /* Check if we have a complete command */
    uint32_t needed = expected_cmd_len(cmdbuf, cmd_pos);
    if (needed > 0 && cmd_pos >= needed) {
      process_cmd(cmdbuf, needed);
      /* Keep leftover bytes that belong to the next command */
      uint32_t leftover = cmd_pos - needed;
      if (leftover > 0) {
        memmove(cmdbuf, cmdbuf + needed, leftover);
      }
      cmd_pos = leftover;
      if (tx_len > 0) {
        tx_state = 1;
      }
    }
  }
}
/* USER CODE END 2 */
