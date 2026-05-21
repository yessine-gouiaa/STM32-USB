/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_usbx_device.c
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
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_otg.h"
#include "ux_dcd_stm32.h"
#include "ux_device_cdc_acm.h"
#include "ux_device_descriptors.h"

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

static ULONG hid_mouse_interface_number;
static ULONG hid_mouse_configuration_number;
static ULONG cdc_acm_interface_number;
static ULONG cdc_acm_configuration_number;
static UX_SLAVE_CLASS_HID_PARAMETER hid_mouse_parameter;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter;
static TX_THREAD ux_device_app_thread;

/* USER CODE BEGIN PV */
static TX_THREAD ux_cdc_write_thread;
static TX_THREAD ux_hid_mouse_thread;

typedef enum {
  USB_DEVICE_MODE_HID = 0,
  USB_DEVICE_MODE_CDC = 1
} USB_DEVICE_MODE_t;

static USB_DEVICE_MODE_t g_usb_mode = USB_DEVICE_MODE_HID;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID app_ux_device_thread_entry(ULONG thread_input);
/* USER CODE BEGIN PFP */
static UINT USB_Device_ReInit_For_Mode(USB_DEVICE_MODE_t mode);

/* USER CODE END PFP */

/**
  * @brief  Application USBX Device Initialization.
  * @param  memory_ptr: memory pointer
  * @retval status
  */


UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
  UINT ret = UX_SUCCESS;
  UCHAR *pointer;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* Allocate the stack for USBX Memory */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_DEVICE_MEMORY_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Initialize USBX Memory */
  if (ux_system_initialize(pointer, USBX_DEVICE_MEMORY_STACK_SIZE, UX_NULL, 0) != UX_SUCCESS)
  {
    return UX_ERROR;
  }

  /* Start in HID mode */
  ret = USB_Device_ReInit_For_Mode(USB_DEVICE_MODE_HID);
  if (ret != UX_SUCCESS)
  {
    return ret;
  }

  /* Allocate the stack for device application main thread */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, UX_DEVICE_APP_THREAD_STACK_SIZE,
                       TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the device application main thread (starts USB PCD) */
  if (tx_thread_create(&ux_device_app_thread, UX_DEVICE_APP_THREAD_NAME,
                       app_ux_device_thread_entry,
                       0, pointer, UX_DEVICE_APP_THREAD_STACK_SIZE,
                       UX_DEVICE_APP_THREAD_PRIO,
                       UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD,
                       UX_DEVICE_APP_THREAD_TIME_SLICE,
                       UX_DEVICE_APP_THREAD_START_OPTION) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Allocate memory for the CDC TX thread */
  if (tx_byte_allocate(byte_pool, (VOID **)&pointer, 1024, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }



  return ret;
}
/**
  * @brief  Function implementing app_ux_device_thread_entry.
  * @param  thread_input: User thread input parameter.
  * @retval none
  */
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern TX_THREAD ux_cdc_write_thread;
extern USB_DEVICE_MODE_t g_usb_mode;
static UINT USB_Device_ReInit_For_Mode(USB_DEVICE_MODE_t mode);

static VOID app_ux_device_thread_entry(ULONG thread_input)
{
  /* USER CODE BEGIN app_ux_device_thread_entry */
  UX_PARAMETER_NOT_USED(thread_input);

  /* Initialize the USB OTG FS Peripheral */
  MX_USB_OTG_FS_PCD_Init();

  /* Allocate the RX and TX FIFOs */
  HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x200);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x100);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x100);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, 0x100);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 3, 0x100);

  /* Link the drivers to USBX */
  ux_dcd_stm32_initialize((ULONG)USB_OTG_FS, (ULONG)&hpcd_USB_OTG_FS);

  /* Start the USB device */
  HAL_PCD_Start(&hpcd_USB_OTG_FS);

    /* Let HID run for ~5 seconds */
    tx_thread_sleep(500);   /* 500 * 10 ms tick = 5 s, if default tick = 10 ms */

    /* Stop USB so host sees disconnect */
    HAL_PCD_Stop(&hpcd_USB_OTG_FS);

    /* Switch to CDC mode */
    USB_Device_ReInit_For_Mode(USB_DEVICE_MODE_CDC);

    /* Start CDC TX thread */
  //  tx_thread_resume(&ux_cdc_write_thread);

    /* Restart USB: host will re-enumerate as CDC ACM */
    HAL_PCD_Start(&hpcd_USB_OTG_FS);

    while (1)
    {
      tx_thread_sleep(1000);
    }

  /* USER CODE END app_ux_device_thread_entry */
}
/* USER CODE BEGIN 1 */
static UINT USB_Device_ReInit_For_Mode(USB_DEVICE_MODE_t mode)
{
  UINT ret;

  UCHAR *device_framework_high_speed;
  UCHAR *device_framework_full_speed;
  ULONG device_framework_hs_length;
  ULONG device_framework_fs_length;
  ULONG string_framework_length;
  ULONG language_id_framework_length;
  UCHAR *string_framework;
  UCHAR *language_id_framework;

  g_usb_mode = mode;

  /* Select descriptor mode */
  if (mode == USB_DEVICE_MODE_HID)
  {
    USBD_Set_Mode_HID();
  }
  else
  {
    USBD_Set_Mode_CDC();
  }

  /* Get Device Framework High Speed and length */
  device_framework_high_speed = USBD_Get_Device_Framework_Speed(USBD_HIGH_SPEED,
                                                                &device_framework_hs_length);

  /* Get Device Framework Full Speed and length */
  device_framework_full_speed = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED,
                                                                &device_framework_fs_length);

  /* Get String Framework and length */
  string_framework = USBD_Get_String_Framework(&string_framework_length);

  /* Get Language Id Framework and length */
  language_id_framework = USBD_Get_Language_Id_Framework(&language_id_framework_length);

  /* Re‑install the device portion of USBX */
  ret = ux_device_stack_initialize(device_framework_high_speed,
                                   device_framework_hs_length,
                                   device_framework_full_speed,
                                   device_framework_fs_length,
                                   string_framework,
                                   string_framework_length,
                                   language_id_framework,
                                   language_id_framework_length,
                                   UX_NULL);
  if (ret != UX_SUCCESS)
  {
    return ret;
  }

  if (mode == USB_DEVICE_MODE_HID)
  {
    /* Initialize the HID mouse class parameters */
    hid_mouse_parameter.ux_slave_class_hid_instance_activate         = USBD_HID_Mouse_Activate;
    hid_mouse_parameter.ux_slave_class_hid_instance_deactivate       = USBD_HID_Mouse_Deactivate;
    hid_mouse_parameter.ux_device_class_hid_parameter_report_address = USBD_HID_ReportDesc(INTERFACE_HID_MOUSE);
    hid_mouse_parameter.ux_device_class_hid_parameter_report_length  = USBD_HID_ReportDesc_length(INTERFACE_HID_MOUSE);
    hid_mouse_parameter.ux_device_class_hid_parameter_report_id      = UX_FALSE;
    hid_mouse_parameter.ux_device_class_hid_parameter_callback       = USBD_HID_Mouse_SetReport;
    hid_mouse_parameter.ux_device_class_hid_parameter_get_callback   = USBD_HID_Mouse_GetReport;

    /* Get HID mouse configuration and interface numbers */
    hid_mouse_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_HID, INTERFACE_HID_MOUSE);
    hid_mouse_interface_number     = USBD_Get_Interface_Number(CLASS_TYPE_HID, INTERFACE_HID_MOUSE);

    /* Register HID class only */
    ret = ux_device_stack_class_register(_ux_system_slave_class_hid_name,
                                         ux_device_class_hid_entry,
                                         hid_mouse_configuration_number,
                                         hid_mouse_interface_number,
                                         &hid_mouse_parameter);
    if (ret != UX_SUCCESS)
    {
      return ret;
    }
  }
  else  /* mode == USB_DEVICE_MODE_CDC */
  {
    /* Initialize the CDC ACM class parameters */
    cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate   = USBD_CDC_ACM_Activate;
    cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = USBD_CDC_ACM_Deactivate;
    cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change    = USBD_CDC_ACM_ParameterChange;

    /* Get CDC configuration and interface numbers */
    cdc_acm_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_CDC_ACM, 0);
    cdc_acm_interface_number     = USBD_Get_Interface_Number(CLASS_TYPE_CDC_ACM, 0);

    /* Register CDC class only */
    ret = ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name,
                                         ux_device_class_cdc_acm_entry,
                                         cdc_acm_configuration_number,
                                         cdc_acm_interface_number,
                                         &cdc_acm_parameter);
    if (ret != UX_SUCCESS)
    {
      return ret;
    }
  }

  return UX_SUCCESS;
}
/* USER CODE END 1 */
