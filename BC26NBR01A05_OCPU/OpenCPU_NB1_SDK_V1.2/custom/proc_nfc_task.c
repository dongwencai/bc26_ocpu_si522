#include "nfc.h"
#include "si522.h"
#include "ql_gpio.h"
#include "ql_spi.h"
#include "stdint.h"
#include "ril.h"
#include "ql_rtc.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_system.h"
#include "ql_timer.h"
#include "main.h"
#include "ql_power.h"
#include "proc_nfc_task.h"

#define USR_SPI_CHANNAL		     (1)

void Ql_GPIO_Toggle(Enum_PinName pinName)
{
	if(Ql_GPIO_GetLevel(pinName)){
		Ql_GPIO_SetLevel(pinName, PINLEVEL_LOW);
	}else{
		Ql_GPIO_SetLevel(pinName, PINLEVEL_HIGH);
	}
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

static void rtc_callback(u32 timerId, void* param)
{
}

void proc_nfc_task(s32 taskId)
{
#if(1)

	Ql_GPIO_Init(PINNAME_GPIO2, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_PULLUP);
#endif
	spi_init(1);
	si522_reset();
	
	si522_edge_trigger_mode();

	while(TRUE){
		#if 1
//		si522_reset();
//
//		si522_edge_trigger_mode();

		#else
		uint8_t uid[4];
		si522_manual();
		if(si522_card_search(uid) == NFC_SUCCESS){
			APP_DEBUG("card sn:%x%x%x%x\r\n", uid[0], uid[1], uid[2], uid[3]);
		}
		#endif
		APP_DEBUG("app running\r\n");

//		Ql_SleepEnable();
		Ql_Sleep(3000);
	}

}

