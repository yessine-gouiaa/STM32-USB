/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    : USB_Host/HID_Standalone/USB_Host/App/usb_host.c
  * @author  : MCD Application Team
  * @brief   : This file implements the USB Host
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
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

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include "main.h"

/* USER CODE BEGIN Includes */


/**
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
__IO HID_APP_State hid_app_state;
extern HID_MOUSE_Info_TypeDef mouse_info;

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void USBH_Clock_Config(void);
extern void HID_MOUSE_App(USBH_HandleTypeDef *phost);
extern void HID_KEYBRD_App(USBH_HandleTypeDef *phost);
static void HID_MOUSE_ProcessData(HID_MOUSE_Info_TypeDef *data);
void HID_KEYBRD_App(USBH_HandleTypeDef *phost);

extern void Error_Handler(void);
/* USER CODE END PFP */

/* USB Host core handle declaration */
USBH_HandleTypeDef hUsbHostHS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */
/**
  * @brief USB Clock Configuration
  * @retval None
  */
void USBH_Clock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /* PLL3 for USB Clock */
  PeriphClkInitStruct.PLL3.PLL3M = 8;
  PeriphClkInitStruct.PLL3.PLL3N = 336;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 7;

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}
/* USER CODE END 0 */

/*
 * user callback declaration
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/**
* @brief  Manages HID Application Process.
* @param  None
* @retval None
*/
void USBH_HID_AppProcess(void)
{
  switch(hid_app_state)
  {
  case HID_APP_WAIT:
    if(Appli_state == APPLICATION_READY)
    {
      if(USBH_HID_GetDeviceType(&hUsbHostHS) == HID_KEYBOARD)
      {
        hid_app_state = HID_APP_KEYBOARD;
        UART3_Log("Use Keyboard to type characters:\r\n");
      }
      else if(USBH_HID_GetDeviceType(&hUsbHostHS) == HID_MOUSE)
      {
        hid_app_state = HID_APP_MOUSE;
        UART3_Log("USB HID Host Mouse App...\r\n");
      }
    }
    break;

  case HID_APP_MOUSE:
    if(Appli_state == APPLICATION_READY)
    {
      HID_MOUSE_App(&hUsbHostHS);
    }
    break;

  case HID_APP_KEYBOARD:
    if(Appli_state == APPLICATION_READY)
    {
      HID_KEYBRD_App(&hUsbHostHS);
    }
    break;

  default:
    break;
  }

  if(Appli_state == APPLICATION_DISCONNECT)
  {
    Appli_state = APPLICATION_IDLE;
    UART3_Log("USB device disconnected !!!\r\n");
    hid_app_state = HID_APP_WAIT;
  }
}

/**
  * @brief  Main routine for Mouse application
  * @param  phost: Host handle
  * @retval None
  */
void HID_MOUSE_App(USBH_HandleTypeDef *phost)
{
  HID_MOUSE_Info_TypeDef *m_pinfo;

  m_pinfo = USBH_HID_GetMouseInfo(phost);

  if(m_pinfo != NULL)
  {
    /* Handle Mouse data position */
    HID_MOUSE_ProcessData(&mouse_info);

    if(m_pinfo->buttons[0])
    {
      UART3_Log("Left Button Pressed\r\n");
    }
    if(m_pinfo->buttons[1])
    {
      UART3_Log("Right Button Pressed\r\n");
    }
    if(m_pinfo->buttons[2])
    {
      UART3_Log("Middle Button Pressed\r\n");
    }
  }
}

static void HID_MOUSE_ProcessData(HID_MOUSE_Info_TypeDef *data)
{
  char buffer[50];
  if((data->x != 0) || (data->y != 0))
  {
    snprintf(buffer, sizeof(buffer), "Mouse : X = %3d, Y = %3d\r\n", data->x, data->y);
    UART3_Log(buffer);
  }
}

void HID_KEYBRD_App(USBH_HandleTypeDef *phost)
{
  HID_KEYBD_Info_TypeDef *k_pinfo;
  char c;
  k_pinfo = USBH_HID_GetKeybdInfo(phost);

  if(k_pinfo != NULL)
  {
    c = USBH_HID_GetASCIICode(k_pinfo);
    if(c != 0)
    {
      char buffer[2] = {c, '\0'};
      UART3_Log(buffer);
    }
  }
}

/* USER CODE END 1 */

/**
  * Init USB host library, add supported class and start the library
  * @retval None
  */
void MX_USB_HOST_Init(void)
{
  /* USER CODE BEGIN USB_HOST_Init_PreTreatment */
  USBH_Clock_Config();

  /* USER CODE END USB_HOST_Init_PreTreatment */

  /* Init host Library, add supported class and start the library. */
  if (USBH_Init(&hUsbHostHS, USBH_UserProcess, HOST_HS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_RegisterClass(&hUsbHostHS, USBH_HID_CLASS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_Start(&hUsbHostHS) != USBH_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_HOST_Init_PostTreatment */

  USBH_UsrLog(" **** USB OTG HS in FS MSC Host **** \n");
  USBH_UsrLog("USB Host library started.\n");

  /* Initialize Application and HID process */

  USBH_UsrLog("Starting HID Application");
  USBH_UsrLog("Connect your HID Device\n");
  hid_app_state = HID_APP_WAIT;

  /* USER CODE END USB_HOST_Init_PostTreatment */
}

/*
 * Background task
 */
void MX_USB_HOST_Process(void)
{
  /* USB Host Background task */
  USBH_Process(&hUsbHostHS);

  /* HID Application Process */
  USBH_HID_AppProcess();
}
/*
 * user callback definition
 */
static void USBH_UserProcess  (USBH_HandleTypeDef *phost, uint8_t id)
{
  /* USER CODE BEGIN CALL_BACK_1 */
  switch(id)
  {
  case HOST_USER_CONNECTION:
  break;

  case HOST_USER_DISCONNECTION:
  Appli_state = APPLICATION_DISCONNECT;
  break;

  case HOST_USER_CLASS_ACTIVE:
  Appli_state = APPLICATION_READY;
  break;

  default:
  break;
  }
  /* USER CODE END CALL_BACK_1 */
}

/**
  * @}
  */

/**
  * @}
  */



