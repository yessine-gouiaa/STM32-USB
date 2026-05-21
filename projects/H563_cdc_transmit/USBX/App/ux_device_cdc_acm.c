/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_cdc_acm.c
  * @author  MCD Application Team
  * @brief   USBX Device applicative file
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
#include "string.h"
#include "main.h"

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
#define APP_RX_DATA_SIZE   2048
#define APP_TX_DATA_SIZE   2048
#define RX_NEW_RECEIVED_DATA      0x01
#define TX_NEW_TRANSMITTED_DATA   0x02
UX_SLAVE_CLASS_CDC_ACM  *cdc_acm;
extern TX_SEMAPHORE semaphore;
UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER CDC_VCP_LineCoding =
{
  115200, /* baud rate */
  0x00,   /* stop bits-1 */
  0x00,   /* parity - none */
  0x08    /* nb. of bits 8 */
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

static void USBD_CDC_VCP_Config(UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER *);
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
VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Activate */
	  cdc_acm = (UX_SLAVE_CLASS_CDC_ACM*) cdc_acm_instance;

	  /* Set device class_cdc_acm with default parameters */
	  if (ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
	                                    &CDC_VCP_LineCoding) != UX_SUCCESS)
	  {
	    Error_Handler();
	  }
  /* USER CODE END USBD_CDC_ACM_Activate */

  return;
}

/**
  * @brief  USBD_CDC_ACM_Deactivate
  *         This function is called when extraction of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Deactivate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  /* USER CODE END USBD_CDC_ACM_Deactivate */

  return;
}

/**
  * @brief  USBD_CDC_ACM_ParameterChange
  *         This function is invoked to manage the CDC ACM class requests.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_ParameterChange */
	UX_PARAMETER_NOT_USED(cdc_acm_instance);

	  ULONG request;
	  UX_SLAVE_TRANSFER *transfer_request;
	  UX_SLAVE_DEVICE *device;

	  /* Get the pointer to the device.  */
	  device = &_ux_system_slave -> ux_system_slave_device;

	  /* Get the pointer to the transfer request associated with the control endpoint. */
	  transfer_request = &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

	  request = *(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_REQUEST);

	  switch (request)
	  {
	    case UX_SLAVE_CLASS_CDC_ACM_SET_LINE_CODING :

	      /* Get the Line Coding parameters */
	      if (ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING,
	                                        &CDC_VCP_LineCoding) != UX_SUCCESS)
	      {
	        Error_Handler();
	      }
	      break;

	    case UX_SLAVE_CLASS_CDC_ACM_GET_LINE_CODING :

	      /* Set the Line Coding parameters */
	      if (ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
	                                        &CDC_VCP_LineCoding) != UX_SUCCESS)
	      {
	        Error_Handler();
	      }
	      break;

	    case UX_SLAVE_CLASS_CDC_ACM_SET_CONTROL_LINE_STATE :
	    default :
	      break;
	  }
  /* USER CODE END USBD_CDC_ACM_ParameterChange */

  return;
}

/* USER CODE BEGIN 1 */

VOID usbx_cdc_acm_write_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);
    ULONG actual_length;
    unsigned int counter = 1;

    /* Buffer must be large enough for base_msg + number + CRLF + '\0' */
    char Tx_Buffer[20];

    /* Base message without number */
    const char base_msg[] ="helloworld";

    while (1)
    {
        /* Format: base message + incrementing number + CRLF */
        int len = snprintf(Tx_Buffer, sizeof(Tx_Buffer), "%s%u\r\n", base_msg, counter);

        /* Check for truncation or error (optional but good practice) */

            /* Formatting error or truncated – handle as needed, e.g. clip len */
            len = sizeof(Tx_Buffer);

        /* Transmit the string over USB CDC ACM */
        ux_device_class_cdc_acm_write(cdc_acm, (UCHAR *)Tx_Buffer, (ULONG)len, &actual_length);

        /* Increment counter so next message has next number */
        counter++;

        /* Delay between transmissions */
        tx_thread_sleep(10UL);
    }
}
/* USER CODE END 1 */
