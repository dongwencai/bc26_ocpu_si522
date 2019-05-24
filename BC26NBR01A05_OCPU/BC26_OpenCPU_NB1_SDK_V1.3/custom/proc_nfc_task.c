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

void Ql_GPIO_Toggle(Enum_PinName pinName)
{
	if(Ql_GPIO_GetLevel(pinName)){
		Ql_GPIO_SetLevel(pinName, PINLEVEL_LOW);
	}else{
		Ql_GPIO_SetLevel(pinName, PINLEVEL_HIGH);
	}
}


static void rtc_callback(u32 timerId, void* param)
{
}

void proc_nfc_task(s32 taskId)
{
	ST_MSG msg;

#if(1)

//	Ql_SleepEnable();

//	Ql_GPIO_Init(PINNAME_GPIO2, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_PULLUP);
#endif
	


	while(TRUE){
		Ql_OS_GetMessage(&msg);
		switch(msg.message){
			case MSG_ID_APP_TEST:
				APP_DEBUG("MSG_ID_APP_TEST\r\n");
				Ql_SleepEnable();
				break;
			default:
				break;
		}
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
	}

}

