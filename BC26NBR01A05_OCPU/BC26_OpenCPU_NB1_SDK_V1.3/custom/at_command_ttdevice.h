#ifndef __AT_COMMAND_TTMODE_H__
#define __AT_COMMAND_TTMODE_H__
#include "atci.h"

#define AT_TTDEVICE_USAGE "+TTDEVICE:<client>/<host>(0,1),180(0-600),type,device name\r\n"

extern atci_status_t atci_cmd_ttdevice(atci_parse_cmd_param_t *parse_cmd);
#endif
