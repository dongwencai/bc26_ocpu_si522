
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
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
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_system.h"
#include "ql_timer.h"
#include "ql_gpio.h"
#include "ql_spi.h"
#include "nfc.h"
#include "si522.h"
#define USR_SPI_CHANNAL		     (1)

#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT0
#define DBG_BUF_LEN   512
static char debug_buffer[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT,...) {\
    Ql_memset(debug_buffer, 0, DBG_BUF_LEN);\
    Ql_sprintf(debug_buffer,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(debug_buffer);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(debug_buffer), Ql_strlen((const char *)(debug_buffer)));\
    }\
}
#else
#define APP_DEBUG(FORMAT,...) 
#endif

// Define the UART port and the receive data buffer
static Enum_SerialPort m_myUartPort  = UART_PORT0;
#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_Uart[SERIAL_RX_BUFFER_LEN];

static void spi_init(u8 spi_type);

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static s32 ATResponse_Handler(char* line, u32 len, void* userData);

static void delay_ms(uint32_t ms)
{
	for(uint32_t i = 0; i < ms; i ++){
		for(uint32_t j = 0; j < 20000; j ++){
			
		}
	}
}
void Ql_GPIO_Toggle(Enum_PinName pinName)
{
	if(Ql_GPIO_GetLevel(pinName)){
		Ql_GPIO_SetLevel(pinName, PINLEVEL_LOW);
	}else{
		Ql_GPIO_SetLevel(pinName, PINLEVEL_HIGH);
	}
}
void proc_main_task(s32 taskId)
{ 
    s32 ret;
    ST_MSG msg;
		char version[32] = {0};
    Enum_PinName  gpioPin = PINNAME_GPIO2;
    Enum_PinLevel gpioLvl = PINLEVEL_HIGH;
    Ql_GetSDKVer(version, 32);
    ret = Ql_UART_Register(m_myUartPort, CallBack_UART_Hdlr, NULL);
    if (ret < QL_RET_OK){
      Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", m_myUartPort, ret);
    }
    ret = Ql_UART_Open(m_myUartPort, 115200, FC_NONE);
    if (ret < QL_RET_OK){
      Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", m_myUartPort, ret);
    }
		spi_init(1);
		Ql_GPIO_Init(gpioPin, PINDIRECTION_OUT, gpioLvl, PINPULLSEL_PULLUP);
		APP_DEBUG("version %s\r\n", version);
		
    APP_DEBUG("OpenCPU: Customer Application123\r\n");
		while(TRUE){
			int8_t status;
			uint8_t uid[6];
//			Ql_OS_GetMessage(&msg);
//			
			status = si522_card_search(uid);

			if(status == NFC_SUCCESS){
				APP_DEBUG("find the card %x%x%x%x\r\n", uid[0], uid[1], uid[2], uid[3]);
			}else if(status == NFC_ERR){
				si522_init();
			}else{

			}
			
			Ql_GPIO_Toggle(gpioPin);
			Ql_Sleep(500);
		}
    // START MESSAGE LOOP OF THIS TASK
    while(TRUE)
    {
      Ql_OS_GetMessage(&msg);
			APP_DEBUG("cmd:%x param1:%d\tparam2:%d\r\n", msg.message, msg.param1, msg.param2);
      switch(msg.message)
      {
	      case MSG_ID_RIL_READY:
          APP_DEBUG("<-- RIL is ready -->\r\n");
          Ql_RIL_Initialize();
          
          break;
	      case MSG_ID_URC_INDICATION:
          //APP_DEBUG("<-- Received URC: type: %d, -->\r\n", msg.param1);
          switch (msg.param1)
          {
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
	          default:
              APP_DEBUG("<-- Other URC: type=%d\r\n", msg.param1);
              break;
          }
          break;
	      default:
          break;
      }
    }

}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen)
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
    switch (msg){
    	case EVENT_UART_READY_TO_READ:
      {
        if (m_myUartPort == port){
          s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart, sizeof(m_RxBuf_Uart));
          if (totalBytes <= 0){
              APP_DEBUG("<-- No data in UART buffer! -->\r\n");
              return;
          }
          {
            s32 ret;
            char* pCh = NULL;
            Ql_UART_Write(m_myUartPort, m_RxBuf_Uart, totalBytes);

            pCh = Ql_strstr((char*)m_RxBuf_Uart, "\r\n");
            if (pCh){
              *(pCh + 0) = '\0';
              *(pCh + 1) = '\0';
            }
            if (Ql_strlen((char*)m_RxBuf_Uart) == 0){
              return;
            }
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
	APP_DEBUG("[ATResponse_Handler] %s\r\n", (u8*)line);

	if (Ql_RIL_FindLine(line, len, "OK")){  
	  return  RIL_ATRSP_SUCCESS;
	}else if (Ql_RIL_FindLine(line, len, "ERROR")){  
	  return  RIL_ATRSP_FAILED;
	}else if (Ql_RIL_FindString(line, len, "+CME ERROR")){
	  return  RIL_ATRSP_FAILED;
	}else if (Ql_RIL_FindString(line, len, "+CMS ERROR:")){
	  return  RIL_ATRSP_FAILED;
	}

	return RIL_ATRSP_CONTINUE; //continue wait
}
static void spi_init(u8 spi_type)
{
	s32 ret;
	
	ret = Ql_SPI_Init(USR_SPI_CHANNAL,PINNAME_SPI_SCLK,PINNAME_SPI_MISO,PINNAME_SPI_MOSI,PINNAME_SPI_CS,spi_type);
	
	if(ret <0){
		APP_DEBUG("\r\n<-- Failed!! Ql_SPI_Init fail , ret =%d-->\r\n",ret)
	}else{
		APP_DEBUG("\r\n<-- Ql_SPI_Init ret =%d -->\r\n",ret)	
	}
	ret = Ql_SPI_Config(USR_SPI_CHANNAL,1,1,1,8192); //config sclk about 30kHz;
	if(ret <0){
		APP_DEBUG("\r\n<--Failed!! Ql_SPI_Config fail  ret=%d -->\r\n",ret)
	}else{
		APP_DEBUG("\r\n<-- Ql_SPI_Config	=%d -->\r\n",ret)
	} 	
	
	
	if (!spi_type){
		Ql_GPIO_Init(PINNAME_SPI_CS,PINDIRECTION_OUT,PINLEVEL_HIGH,PINPULLSEL_PULLUP);	 //CS high
	}

}

#endif // __CUSTOMER_CODE__
