#ifndef __AT_COMMAND_REBOOT_H__
#define __AT_COMMAND_REBOOT_H__

#include "atci.h"

#define AT_REBOOT_USAGE "AT+REBOOT\r\n"

extern atci_status_t atci_cmd_reboot(atci_parse_cmd_param_t *parse_cmd);

#endif
