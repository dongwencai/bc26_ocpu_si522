#include "nfc.h"
#include "si522.h"
#include "ql_gpio.h"
#include "ql_spi.h"
#include "stdint.h"
#include "ril.h"
#include "ql_rtc.h"
#include "ql_time.h"
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

static struct {
	s8 retry;
	uint8_t uid[4];
	card_step_t step;		
	u32 nfc_check_tmr_id;
}ccb;

//#define MANUAL_FIND_CARD
#define USR_SPI_CHANNAL		     (1)
#define OP_IS_TIMEOUT()	((--ccb.retry) <= 0)

void set_card_step(card_step_t step, s8 retry)
{
	ccb.step = step;
	ccb.retry = retry;
}

static bool card_search(uint8_t *uid)
{
	s8 ret;
	static s8 cnt = 0x00;
	ret = si522_card_search(uid);
	if(ret == NFC_ERR){
		cnt ++;
		if(cnt > 3){
			cnt = 0x00;
			set_card_step(CARD_INIT, 0);
		}
		return FALSE;
	}
	cnt = 0x00;
	return ret == NFC_SUCCESS ? TRUE : FALSE;
}


static void card_search_proc(void)
{
	uint8_t uid[6];
	APP_DEBUG("CARD SEARCH\r\n");
	
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_HALT_CHECK_START_E, 0);

	#ifdef MANUAL_FIND_CARD
	if(card_search(uid)){		
		set_card_step(CARD_AUTHENT, 3);

		Ql_memcpy(ccb.uid, uid, 4);
	}
	#else
	si522_manual();
	for(uint8_t i = 0; i < 3; i ++){
		if(card_search(uid)){		
			Ql_memcpy(ccb.uid, uid, 4);
			Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_CARD_AUTHENT_E, 0);
			return;
		}
	}
	Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_SLEEP_ENABLE, 0);
	#endif
}

static void card_read_proc(void)
{
	ST_Time s;
	uint8_t buf[16];
	
	APP_DEBUG("CARD READ\r\n");
	#ifdef MANUAL_FIND_CARD
	if(OP_IS_TIMEOUT()){
		set_card_step(CARD_IDLE, 10);
	}else{
		if(si522_card_block_read(buf, 0x0c) == NFC_SUCCESS){
			set_card_step(CARD_IDLE, 10);
			APP_DEBUG("card no:%d\r\n", buf[1]);
			break;
		}
	}
	#else
	for(uint8_t i = 0; i < 3; i ++){
		if(si522_card_block_read(buf, 0x0c) == NFC_SUCCESS){
			APP_DEBUG("card no:%d\r\n", buf[1]);
			Ql_GetLocalTime(&s);
			APP_DEBUG("%d-%d-%d %d:%d:%d\r\n", s.year, s.month, s.day, s.hour, s.minute, s.second);
			mqtt_send_data(&buf[1], 1);
			break;
		}
	}
	#endif
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_HALT_CHECK_STOP_E, 0);
	return;
}

static void card_authent_proc(void)
{
	uint8_t key[] = {0x12, 0x34, 0x56, 0x12, 0x34, 0x56};
	APP_DEBUG("CARD AUTHENT\r\n");
	#ifdef MANUAL_FIND_CARD
	if(OP_IS_TIMEOUT()){
		set_card_step(CARD_IDLE, 10);
	}else{
		if(si522_card_authent(ccb.uid, 0, key, 0x0c) == NFC_SUCCESS){
			set_card_step(CARD_READ, 3);
		}
	}
	#else
	for(uint8_t i = 0; i < 3; i ++){
		if(si522_card_authent(ccb.uid, 0, key, 0x0c) == NFC_SUCCESS){
			Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_CARD_READ_E, 0);
			return;
		}
	}
	Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_SLEEP_ENABLE, 0);
	#endif
}

static void card_psm_proc(void)
{
	APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_HALT_CHECK_START_E, 0);
	si522_reset();
	si522_edge_trigger_mode();
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_HALT_CHECK_STOP_E, 0);
	Ql_SleepEnable();

}
static void card_idle_proc(void)
{
	uint8_t uid[4];
	if(OP_IS_TIMEOUT()){
		set_card_step(CARD_SEARCH, 0);
	}else{
		if(card_search(uid)){
			if(Ql_memcmp(uid, ccb.uid, 4) == 0){
				set_card_step(CARD_IDLE, 10);
				return;
			}
		}
	}
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




void card_proc_timer(u32 timerId, void* param)
{
	switch(ccb.step){
		case CARD_INIT:
			APP_DEBUG("CARD_INIT\r\n");
//				si522_init();
			si522_manual();
			ccb.step = CARD_SEARCH;
			break;
		case CARD_SEARCH:
			card_search_proc();
			break;
		case CARD_READ:
			card_read_proc();
			break;
		case CARD_AUTHENT:
			card_authent_proc();
			break;
		case CARD_IDLE:
			card_idle_proc();
			break;
		case CARD_PSM:
			break;
		default:
		
			break;
	}
}

static void proc_nfc_init(void)
{
	spi_init(1);
	Ql_memset(&ccb, 0x00, sizeof(ccb));
	ccb.nfc_check_tmr_id = 0x104;
	Ql_Timer_Register(ccb.nfc_check_tmr_id, card_proc_timer, NULL);
}

void proc_nfc_task(s32 taskId)
{
	uint8_t uid[4];
	ST_MSG msg;
	proc_nfc_init();
//	Ql_Timer_Start(ccb.nfc_check_tmr_id, 200, TRUE);
	while(TRUE){
		Ql_OS_GetMessage(&msg);		
		APP_DEBUG("%s\t%d\t%x\r\n", __func__, __LINE__, msg.param1);
		switch(msg.param1){
			case USR_MSG_CARD_ATACHE_E:
				card_search_proc();
				break;
			case USR_MSG_CARD_AUTHENT_E:
				card_authent_proc();
				break;
			case USR_MSG_CARD_READ_E:
				card_read_proc();
				break;
			case USR_MSG_SLEEP_ENABLE:
				card_psm_proc();
				break;
		}
	}

}

