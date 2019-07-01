#ifndef __MAIN_H__
#define __MAIN_H__
#include "ql_uart.h"
#include "ql_stdlib.h"

typedef enum{
	USR_MSG_BH_E,
	USR_MSG_NET_ATACHE_E,
	USR_MSG_CARD_ATACHE_E,
	USR_MSG_CARD_AUTHENT_E,
	USR_MSG_CARD_READ_E,
	USR_MSG_SLEEP_ENABLE,
	USR_MSG_BEEP_E,
	USR_MSG_MQTT_LED_START,
	USR_MSG_MQTT_LED_STOP,
	USR_MSG_END,
}usr_msg_t;

#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT0
#define DBG_BUF_LEN   512
extern char debug_buffer[DBG_BUF_LEN];;
#define APP_DEBUG(FORMAT,...) do{\
    Ql_memset(debug_buffer, 0, DBG_BUF_LEN);\
    Ql_sprintf(debug_buffer,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)){\
        Ql_Debug_Trace(debug_buffer);\
    }else{\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(debug_buffer), Ql_strlen((const char *)(debug_buffer)));\
    }\
	}while(0)
#else
#define APP_DEBUG(FORMAT,...) 
#endif
#define EVENT_RUNNING 		(1 << 0)
#define EVENT_NET_REG 	(1 << 1)
#define EVENT_CARD_ACTIVE (1 << 4)
#define EVENT_ENTER_PSM 		(1 << 5)
#define EVENT_MQTT_STAT (1 << 7)
#define EVENT_MQTT_OPEN_RESP (1 << 8)
#define EVENT_MQTT_OPEN_UCR (1 << 9)
#define EVENT_MQTT_CONN_RESP (1 << 10)
#define EVENT_MQTT_CONN_UCR (1 << 11)
#define EVENT_MQTT_PUB_RESP	(1 << 12)
#define EVENT_MQTT_PUB_UCR	(1 << 13)
#define EVENT_MQTT_SUB_RESP	(1 << 12)
#define EVENT_MQTT_SUB_UCR	(1 << 13)
#define EVENT_MQTT_CLOSE_RESP (1 << 14)

extern u32 sys_event_id;
extern u32 main_ticks_get(void);
#endif
