#ifndef __AT_COMMAND_MTCFG_H__
#define __AT_COMMAND_MTCFG_H__

#include "atci.h"

#define AT_MTCFG_USAGE "AT+MTCFG=\"vession\",(3,4)\r\n\
AT+MTCFG=\"keepalive\",(0-360)\r\n\
AT+MTCFG=\"timeout\",(1-60),(1-10)\r\n\
AT+MTCFG=\"session\",0\r\n\
AT+MTCFG=\"server\",\"ip\",port,\"username\",\"password\"\r\n"

extern atci_status_t atci_cmd_mtcfg(atci_parse_cmd_param_t *parse_cmd);

#endif
