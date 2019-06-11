
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2019
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   main.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This app demonstrates how to send AT command with RIL API, and transparently
 *   transfer the response through MAIN UART. And how to use UART port.
 *   Developer can program the application based on this example.
 * 
 ****************************************************************************/
#ifdef __CUSTOMER_CODE__
#include "custom_feature_def.h"
#include "ril.h"
#include "main.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_time.h"
#include "ql_system.h"
#include "ql_timer.h"
#include "si522.h"
#include <ql_type.h>
#include "ril_mqtt.h"
#include "atci_main.h"
#include "device_info.h"
#include "ril_network.h"
#include "proc_mqtt_task.h"

s32 sys_event_id;
static bool tt_init = FALSE;

#if DEBUG_ENABLE > 0
char debug_buffer[DBG_BUF_LEN];
#endif

static u32 halt_check_tmr_id = 0x105;
static u32 mqtt_led_tmr_id = 0x106;

#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_Uart[SERIAL_RX_BUFFER_LEN];
static Enum_SerialPort m_myUartPort  = UART_PORT0;
static atci_send_input_cmd_msg_t g_input_command = {{0}};

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static s32 ATResponse_Handler(char* line, u32 len, void* userData);

static void callback_psm_eint(void *user_data)
{
//	Ql_SleepDisable();
	if(tt_init){
		APP_DEBUG("PSM EXIT\r\n");
		Ql_SleepDisable();
		Ql_OS_SendMessageFromISR(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_CARD_ATACHE_E, 0);
	}
}

static void Ql_GPIO_Toggle(Enum_PinName pinName)
{
	if(Ql_GPIO_GetLevel(pinName)){
		Ql_GPIO_SetLevel(pinName, PINLEVEL_LOW);
	}else{
		Ql_GPIO_SetLevel(pinName, PINLEVEL_HIGH);
	}
}


static void halt_check_timer(u32 timerId, void* param)
{
	Ql_OS_SendMessage(main_task_id ,MSG_ID_URC_INDICATION, URC_SYS_END, 0);
//	Ql_Reset(0);
}

static void mqtt_led_timer(u32 timerId, void* param)
{
	Ql_GPIO_Toggle(PINNAME_GPIO2);
}


void proc_main_task(s32 taskId)
{ 
  s32 ret;
  ST_MSG msg;
	char version[32] = {0};
	Ql_SleepDisable();
  Ql_GetSDKVer(version, 32);
	sys_event_id = Ql_OS_CreateEvent();
	Ql_Timer_Register(halt_check_tmr_id, halt_check_timer, NULL);
	Ql_Timer_Register(mqtt_led_tmr_id, mqtt_led_timer, NULL);
	Ql_GPIO_Init(PINNAME_GPIO2, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_DISABLE);
  ret = Ql_UART_Register(m_myUartPort, CallBack_UART_Hdlr, NULL);
  if (ret < QL_RET_OK){
    Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", m_myUartPort, ret);
  }
  ret = Ql_UART_Open(m_myUartPort, 115200, FC_NONE);
  if (ret < QL_RET_OK){
    Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", m_myUartPort, ret);
  }		
	APP_DEBUG("version %s\r\n", version);
	atci_init(&m_myUartPort);
	ret = Ql_Psm_Eint_Register(callback_psm_eint,NULL);
	APP_DEBUG("psm_eint register , ret=%d\r\n",ret);
	ret = Ql_GetPowerOnReason();
	APP_DEBUG("power on reason, ret=%d\r\n",ret);
	Ql_Timer_Start(halt_check_tmr_id, 3000, TRUE);
	tt_init = TRUE;
	device_info_load();
  while(TRUE){
    Ql_OS_GetMessage(&msg);
    switch(msg.message){
      case MSG_ID_RIL_READY:
        APP_DEBUG("<-- RIL is ready -->\r\n");
        Ql_RIL_Initialize();			
				
				Ql_OS_SetEvent(sys_event_id, EVENT_RUNNING);
        break;
      case MSG_ID_URC_INDICATION:
        switch (msg.param1){
          case URC_SYS_INIT_STATE_IND:
            APP_DEBUG("<-- Sys Init Status %d -->\r\n", msg.param2);
            break;
          case URC_SIM_CARD_STATE_IND:
            APP_DEBUG("<-- SIM Card Status:%d -->\r\n", msg.param2);
            break;            
          case URC_EGPRS_NW_STATE_IND:
            APP_DEBUG("<-- EGPRS Network Status:%d -->\r\n", msg.param2);
            break;
          case URC_CFUN_STATE_IND:
            APP_DEBUG("<-- CFUN Status:%d -->\r\n", msg.param2);
            break;  
					case URC_MQTT_OPEN:
						RIL_SOC_QMTOPEN_SET_RES(msg.param2);
						APP_DEBUG("<-- MQTT Open Status:%d -->\r\n", msg.param2);
						break;
					case URC_MQTT_CONN:
						RIL_SOC_QMTCONN_SET_RES(msg.param2);
						break;
					case URC_MQTT_CONN_IND:
						RIL_SOC_QMTCONN_SET_URC(msg.param2);
						break;
					case URC_MQTT_PUB:
						RIL_SOC_QMTPUB_SET_RES(msg.param2);
						break;
					case URC_MQTT_SUB:
						break;
					case URC_MQTT_STAT:
						RIL_SOC_QMTSTAT_SET_URC(msg.param2);
						break;
					case URC_MQTT_CLOSE:
						RIL_SOC_QMTCLOSE_SET_RESP(msg.param2);
						break;
					case URC_IPADDR_IND:
						break;
          default:
						APP_DEBUG("%s\t%d\t%x\r\n", __func__, __LINE__, msg.param1);
            break;
        }
        break;
			case MSG_ID_APP_TEST:
				switch(msg.param1){					
					case USR_MSG_HALT_CHECK_STOP_E:
						APP_DEBUG("halt check stop\r\n");
						Ql_Timer_Stop(halt_check_tmr_id);
						break;
					case USR_MSG_HALT_CHECK_START_E:
						APP_DEBUG("halt check start\r\n");
						Ql_Timer_Start(halt_check_tmr_id, 2000, FALSE);
						break;						
					case USR_MSG_MQTT_LED_STOP:
						Ql_Timer_Stop(mqtt_led_tmr_id);
						Ql_GPIO_SetLevel(PINNAME_GPIO2, PINLEVEL_LOW);
						break;
					case USR_MSG_MQTT_LED_START:
						Ql_Timer_Start(mqtt_led_tmr_id, 100, TRUE);
						break;
				}
				break;
      default:
        break;
    }
  }

}

static s32 ReadSerialPort(Enum_SerialPort port, u8* pBuffer, u32 bufLen)
{
  s32 rdLen = 0;
  s32 rdTotalLen = 0;
  if (NULL == pBuffer || 0 == bufLen){
    return -1;
  }
  Ql_memset(pBuffer, 0x0, bufLen);
  while (1){
    rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
    if (rdLen <= 0){
      break;
    }
    rdTotalLen += rdLen;
  }
  if (rdLen < 0){
    APP_DEBUG("Fail to read from port[%d]\r\n", port);
    return -99;
  }
  return rdTotalLen;
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
	atci_status_t status;

  switch (msg){
    case EVENT_UART_READY_TO_READ:
    {
      if (m_myUartPort == port){
        s32 ret;
        char* pCh = NULL;
        s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart, sizeof(m_RxBuf_Uart));
        if (totalBytes <= 0){
          return;
        }
        Ql_UART_Write(m_myUartPort, m_RxBuf_Uart, totalBytes);
        pCh = Ql_strstr((char*)m_RxBuf_Uart, "\r\n");
        if (pCh){
          *(pCh + 0) = '\0';
          *(pCh + 1) = '\0';
        }
        if (Ql_strlen((char*)m_RxBuf_Uart) == 0){
          return;
        }
				g_input_command.input_len = totalBytes;
				Ql_memcpy(g_input_command.input_buf, m_RxBuf_Uart, totalBytes);
				status = atci_input_command_handler(&g_input_command);
				if(status != ATCI_STATUS_OK){
	        ret = Ql_RIL_SendATCmd((char*)m_RxBuf_Uart, totalBytes, ATResponse_Handler, NULL, 0);
				}
      }
      break;
    }
    case EVENT_UART_READY_TO_WRITE:
      break;
    default:
      break;
  }
}

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
  
  if (Ql_RIL_FindLine(line, len, "OK")){  
    return  RIL_ATRSP_SUCCESS;
  }else if (Ql_RIL_FindLine(line, len, "ERROR")){  
    return  RIL_ATRSP_FAILED;
  }else if (Ql_RIL_FindString(line, len, "+CME ERROR")){
    return  RIL_ATRSP_FAILED;
  }else if (Ql_RIL_FindString(line, len, "+CMS ERROR:")){
    return  RIL_ATRSP_FAILED;
  }else{
		APP_DEBUG("[ATResponse_Handler] %s\r\n", (u8*)line);
	}
  return RIL_ATRSP_CONTINUE;
}

#endif // __CUSTOMER_CODE__
