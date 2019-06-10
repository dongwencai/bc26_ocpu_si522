#ifndef __AT_COMMAND_MTPUB_H__
#define __AT_COMMAND_MTPUB_H__
#include "atci.h"

#define AT_MTPUB_USAGE 	"AT+MTPUB=\"<topic>\"\r\n"

extern atci_status_t atci_cmd_mtpub(atci_parse_cmd_param_t *parse_cmd);

#endif
