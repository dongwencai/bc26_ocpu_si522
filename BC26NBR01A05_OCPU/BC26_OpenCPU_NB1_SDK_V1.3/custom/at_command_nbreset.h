#ifndef __AT_COMMAND_NBRESET_H__
#define __AT_COMMAND_NBRESET_H__
#include "atci.h"

#define AT_NBRESET_USAGE "AT+NBRESET\r\n"
extern atci_status_t atci_cmd_nbreset(atci_parse_cmd_param_t *parse_cmd);
#endif
