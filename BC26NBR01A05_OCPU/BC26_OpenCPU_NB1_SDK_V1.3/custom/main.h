#ifndef __MAIN_H__
#define __MAIN_H__
#include "ql_uart.h"
#include "ql_stdlib.h"
#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT0
#define DBG_BUF_LEN   512
extern char debug_buffer[DBG_BUF_LEN];;
#define APP_DEBUG(FORMAT,...) do{\
    Ql_memset(debug_buffer, 0, DBG_BUF_LEN);\
    Ql_sprintf(debug_buffer,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(debug_buffer);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(debug_buffer), Ql_strlen((const char *)(debug_buffer)));\
    }\
	}while(0)
#else
#define APP_DEBUG(FORMAT,...) 
#endif

#endif
