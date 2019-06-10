#ifndef __AT_COMMAND_MTSUB_H__
#define __AT_COMMAND_MTSUB_H__
#include "atci.h"

#define AT_MTSUB_USAGE	"AT+MTSUB=<topic1>,,<topic2>...\r\n"

extern atci_status_t atci_cmd_mtsub(atci_parse_cmd_param_t *parse_cmd);
#endif

