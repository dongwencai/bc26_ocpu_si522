#ifndef __AT_COMMAND_TTDEBUG_H__
#define __AT_COMMAND_TTDEBUG_H__
#include "atci.h"

#define AT_LOG_USAGE "AT+TTDEBUG=\"log1,log2...\"(bc26,stm32tt)\r\n"

extern atci_status_t atci_cmd_ttdebug(atci_parse_cmd_param_t *parse_cmd);

#endif
