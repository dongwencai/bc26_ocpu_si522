#ifndef __AT_COMMAND_TTVOLUME_H__
#define __AT_COMMAND_TTVOLUME_H__
#include <atci.h>

#define AT_TTVOLUME_USAGE "AT+TTVOLUME=<volume>\r\n"

extern atci_status_t atci_cmd_ttvolume(atci_parse_cmd_param_t *parse_cmd);
#endif
