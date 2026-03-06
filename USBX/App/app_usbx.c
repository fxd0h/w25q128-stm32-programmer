/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_usbx.c
 * @author  MCD Application Team
 * @brief   USBX applicative file
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
#include "app_usbx.h"

/* USER CODE BEGIN UX_Memory_Buffer */
/* CRITICAL: USB OTG FS DMA cannot access DTCMRAM (0x20000000).
   The USBX memory pool MUST be in AHB-accessible RAM (RAM_D2 = 0x30000000). */
/* USER CODE END UX_Memory_Buffer */
#if defined(__ICCARM__)
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN static UCHAR ux_byte_pool_buffer[UX_APP_MEM_POOL_SIZE] __ALIGN_END
    __attribute__((section(".RAM_D2")));

volatile uint32_t usbx_debug_phase = 0;

/**
 * @brief  Application USBX Initialization.
 * @param  none
 * @retval status
 */
UINT MX_USBX_Init(VOID) {
  UINT ret = UX_SUCCESS;

  UCHAR *pointer;

  /* USER CODE BEGIN MX_USBX_Init0 */
  /* Zero-initialize the memory pool — it's in RAM_D2 (NOLOAD section),
     so the startup code does NOT zero it automatically. */
  {
    volatile uint8_t *p = (volatile uint8_t *)ux_byte_pool_buffer;
    for (uint32_t i = 0; i < UX_APP_MEM_POOL_SIZE; i++)
      p[i] = 0;
  }
  /* USER CODE END MX_USBX_Init0 */

  pointer = ux_byte_pool_buffer;

  /* Initialize USBX Memory: split pool into regular + cache-safe halves */
  usbx_debug_phase = 1;
  {
    ULONG regular_size = UX_APP_MEM_POOL_SIZE / 2;
    ULONG cache_safe_size = UX_APP_MEM_POOL_SIZE - regular_size;
    if (ux_system_initialize(pointer, regular_size, pointer + regular_size,
                             cache_safe_size) != UX_SUCCESS) {
      /* USER CODE BEGIN USBX_SYSTEM_INITIALIZE_ERROR */
      return UX_ERROR;
      /* USER CODE END USBX_SYSTEM_INITIALIZE_ERROR */
    }
  }

  usbx_debug_phase = 2;
  if (MX_USBX_Device_Init() != UX_SUCCESS) {
    /* USER CODE BEGIN MX_USBX_Device_Init_Error */
    return UX_ERROR;
    /* USER CODE END MX_USBX_Device_Init_Error */
  }
  usbx_debug_phase = 3;

  /* USER CODE BEGIN MX_USBX_Init1 */

  /* USER CODE END MX_USBX_Init1 */

  return ret;
}

/**
 * @brief  _ux_utility_interrupt_disable
 *         USB utility interrupt disable.
 * @param  none
 * @retval none
 */
ALIGN_TYPE _ux_utility_interrupt_disable(VOID) {
  UINT interrupt_save;

  /* USER CODE BEGIN _ux_utility_interrupt_disable */
  interrupt_save = __get_PRIMASK();
  __disable_irq();
  /* USER CODE END _ux_utility_interrupt_disable */

  return interrupt_save;
}

/**
 * @brief  _ux_utility_interrupt_restore
 *         USB utility interrupt restore.
 * @param  flags
 * @retval none
 */
VOID _ux_utility_interrupt_restore(ALIGN_TYPE flags) {
  /* USER CODE BEGIN _ux_utility_interrupt_restore */
  __set_PRIMASK(flags);
  /* USER CODE END _ux_utility_interrupt_restore */
}

/**
 * @brief  _ux_utility_time_get
 *         Get Time Tick for host timing.
 * @param  none
 * @retval time tick
 */
ULONG _ux_utility_time_get(VOID) {
  ULONG time_tick = 0U;

  /* USER CODE BEGIN _ux_utility_time_get */
  time_tick = HAL_GetTick();
  /* USER CODE END _ux_utility_time_get */

  return time_tick;
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
