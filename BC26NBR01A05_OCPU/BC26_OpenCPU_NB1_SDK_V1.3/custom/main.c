
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
#include "ql_system.h"
#include "ql_timer.h"
#include "si522.h"
#include <stdbool.h>
static bool tt_init = false;
#if DEBUG_ENABLE > 0
char debug_buffer[DBG_BUF_LEN];
#endif

#define USR_SPI_CHANNAL		     (1)

// Define the UART port and the receive data buffer
static Enum_SerialPort m_myUartPort  = UART_PORT0;
#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_Uart[SERIAL_RX_BUFFER_LEN];


static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static s32 ATResponse_Handler(char* line, u32 len, void* userData);

static void callback_psm_eint(void *user_data)
{
//	Ql_SleepDisable();
	if(tt_init){
		si522_reset();
		si522_edge_trigger_mode();	
		
			APP_DEBUG("%x\t%x\t%x\t%x\t%x\r\n", 
				si522_read(ComIEnReg), si522_read(DivlEnReg), si522_read(ComIrqReg), si522_read(DivIrqReg), si522_read(CommandReg));
	}
//	si522_init();
//	si522_write(ComIrqReg, 0x04);
//	Ql_SleepDisable();

//	APP_DEBUG("wake up\r\n");
//spi_init(1);

//	APP_DEBUG("%x\t%x\t%x\t%x\t%x\r\n", 
//		si522_read(ComIEnReg), si522_read(DivlEnReg), si522_read(ComIrqReg), si522_read(DivIrqReg), si522_read(CommandReg));
//	si522_init();
////	si522_write();
//	si522_edge_trigger_mode();
}

static void spi_init(u8 spi_type)
{
	s32 ret;
	
	ret = Ql_SPI_Init(USR_SPI_CHANNAL,PINNAME_SPI_SCLK,PINNAME_SPI_MISO,PINNAME_SPI_MOSI,PINNAME_SPI_CS,spi_type);
	
	if(ret <0){
		APP_DEBUG("\r\n<-- Failed!! Ql_SPI_Init fail , ret =%d-->\r\n",ret);
	}else{
		APP_DEBUG("\r\n<-- Ql_SPI_Init ret =%d -->\r\n",ret)	;
	}
	ret = Ql_SPI_Config(USR_SPI_CHANNAL,1,1,1,8192); //config sclk about 30kHz;
	if(ret <0){
		APP_DEBUG("\r\n<--Failed!! Ql_SPI_Config fail  ret=%d -->\r\n",ret);
	}else{
		APP_DEBUG("\r\n<-- Ql_SPI_Config	=%d -->\r\n",ret);
	} 	
	
	
	if (!spi_type){
		Ql_GPIO_Init(PINNAME_SPI_CS,PINDIRECTION_OUT,PINLEVEL_HIGH,PINPULLSEL_PULLUP);	 //CS high
	}

}

void proc_main_task(s32 taskId)
{ 
    s32 ret;
    ST_MSG msg;
		char version[32] = {0};
		Ql_SleepEnable();
    Ql_GetSDKVer(version, 32);

		#if DEBUG_ENABLE > 0
		
    ret = Ql_UART_Register(m_myUartPort, CallBack_UART_Hdlr, NULL);
    if (ret < QL_RET_OK){
      Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", m_myUartPort, ret);
    }
    ret = Ql_UART_Open(m_myUartPort, 115200, FC_NONE);
    if (ret < QL_RET_OK){
      Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", m_myUartPort, ret);
    }
		ret = Ql_Psm_Eint_Register(callback_psm_eint,NULL);
		APP_DEBUG("psm_eint register , ret=%d\r\n",ret);
		ret = Ql_GetPowerOnReason();
		APP_DEBUG("power on reason, ret=%d\r\n",ret);
		spi_init(1);
		#endif
		APP_DEBUG("version %s\r\n", version);
		
    APP_DEBUG("OpenCPU: Customer Application\r\n");
		si522_reset();
		si522_edge_trigger_mode();
			APP_DEBUG("%x\t%x\t%x\t%x\t%x\r\n", 
				si522_read(ComIEnReg), si522_read(DivlEnReg), si522_read(ComIrqReg), si522_read(DivIrqReg), si522_read(CommandReg));
		tt_init = true;

    // START MESSAGE LOOP OF THIS TASK
    while(TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
        case MSG_ID_RIL_READY:
            APP_DEBUG("<-- RIL is ready -->\r\n");
            Ql_RIL_Initialize();
            
            break;
        case MSG_ID_URC_INDICATION:
            APP_DEBUG("<-- Received URC: type: %d, -->\r\n", msg.param1);
            switch (msg.param1)
            {
            case URC_SYS_INIT_STATE_IND:
                APP_DEBUG("<-- Sys Init Status %d -->\r\n", msg.param2);
                break;
            case URC_SIM_CARD_STATE_IND:
                APP_DEBUG("<-- SIM Card Status:%d -->\r\n", msg.param2);
                break;            
            case URC_EGPRS_NW_STATE_IND:
//								Ql_SleepEnable();
                APP_DEBUG("<-- EGPRS Network Status:%d -->\r\n", msg.param2);
//								Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, 0, 0);
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
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        APP_DEBUG("Fail to read from port[%d]\r\n", port);
        return -99;
    }
    return rdTotalLen;
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    //APP_DEBUG("CallBack_UART_Hdlr: port=%d, event=%d, level=%d, p=%x\r\n", port, msg, level, customizedPara);
    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {
            if (m_myUartPort == port)
            {
                s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart, sizeof(m_RxBuf_Uart));
                if (totalBytes <= 0)
                {
                    APP_DEBUG("<-- No data in UART buffer! -->\r\n");
                    return;
                }
                {// Read data from UART
                    s32 ret;
                    char* pCh = NULL;
                    
                    // Echo
                    Ql_UART_Write(m_myUartPort, m_RxBuf_Uart, totalBytes);

                    pCh = Ql_strstr((char*)m_RxBuf_Uart, "\r\n");
                    if (pCh)
                    {
                        *(pCh + 0) = '\0';
                        *(pCh + 1) = '\0';
                    }

                    // No permission for single <cr><lf>
                    if (Ql_strlen((char*)m_RxBuf_Uart) == 0)
                    {
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
    
    if (Ql_RIL_FindLine(line, len, "OK"))
    {  
        return  RIL_ATRSP_SUCCESS;
    }
    else if (Ql_RIL_FindLine(line, len, "ERROR"))
    {  
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CME ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CMS ERROR:"))
    {
        return  RIL_ATRSP_FAILED;
    }
    
    return RIL_ATRSP_CONTINUE; //continue wait
}

#endif // __CUSTOMER_CODE__
