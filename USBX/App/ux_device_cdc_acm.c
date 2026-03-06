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
static uint32_t tx_tick = 0;
static uint32_t tx_state = 0; /* 0=idle, 1=writing */
static uint8_t tx_msg[] __attribute__((section(".RAM_D2"))) = "HELLO\r\n";
static uint32_t write_wait_count = 0;
static uint32_t write_next_count = 0;
static uint32_t write_error_count = 0;

/**
 * @brief  Diagnostic: periodic write + LED status reporting
 */
void usbx_cdc_acm_read_write_run(void) {
  if (cdc_acm == UX_NULL)
    return;

  /* Check device state */
  UX_SLAVE_DEVICE *device = &_ux_system_slave->ux_system_slave_device;
  if (device->ux_slave_device_state != UX_DEVICE_CONFIGURED)
    return;

  UINT status;

  if (tx_state == 0) {
    if (HAL_GetTick() - tx_tick >= 2000) {
      tx_tick = HAL_GetTick();
      tx_state = 1;
      write_wait_count = 0;
    }
  }

  if (tx_state == 1) {
    ULONG actual = 0;
    status = ux_device_class_cdc_acm_write_run(cdc_acm, tx_msg,
                                               sizeof(tx_msg) - 1, &actual);
    if (status == UX_STATE_NEXT) {
      BSP_LED_Toggle(LED_GREEN); /* Success! */
      write_next_count++;
      tx_state = 0;
    } else if (status < UX_STATE_NEXT) {
      BSP_LED_Toggle(LED_YELLOW); /* Error/Exit */
      write_error_count++;
      tx_state = 0;
    } else {
      write_wait_count++;
    }
  }
}
/* USER CODE END 2 */
