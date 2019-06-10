#include <stdint.h>
#include "atci.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
// For Register AT command handler
#include "at_command_nbreset.h"
#include "at_command_reboot.h"
#include "at_command_mtcfg.h"
#include "at_command_mtpub.h"
#include "at_command_mtsub.h"
#include "at_command_ttdebug.h"
#include "at_command_ttdevice.h"
#include "at_command_ttvolume.h"

static int atci_parse_string(char *param, char *context)
{
	for(uint16_t i = 0x01; i < Ql_strlen(context); i ++){
		if(context[i] == '\\'){
			i ++;
			continue;
		}
		if(context[i] == '"'){
			Ql_memcpy(param, context, i + 1);
			return i + 1;
		}
	}
	return -1;
}

bool atci_param_parse(atci_param_t *pb, char *param, char *delimit)
{
	char *p, *e, gap;
	uint16_t pos, len, pn = 0x00;
	
	Ql_memset(pb, 0x00, sizeof(atci_param_t));
	pb->cmd = pb->context;
	Ql_sscanf(param, delimit, pb->cmd);
	pos = Ql_strlen(pb->cmd) + 1;
	
	if(Ql_strcmp(delimit, "AT+%[^=]") == 0){
		gap = '=';
	}else{
		gap = ':';
	}
	p = Ql_strchr(param, gap);
	if(!p)	return FALSE;

	p ++;
	pb->param[pn] = &pb->context[pos];
	for(uint16_t i = 0x00; i < Ql_strlen(p); i ++){
		if(p[i] == ' '){
			continue;
		}else if(p[i] == '"'){
			if((len = atci_parse_string(pb->param[pn], &p[i])) < 0){
				return FALSE;
			}
		}else{
			if(e = Ql_strchr(&p[i], ',')){
				len = e - &p[i];
				Ql_strncpy(pb->param[pn], &p[i], len);
			}else{
				len = Ql_strlen(&p[i]);
				Ql_strncpy(pb->param[pn], &p[i], len);
			}
		}
		pn ++;	i += len; 	pos += len + 1;
		pb->param[pn] = &pb->context[pos];
		if(p[i] != ','){
			pb->param_cnt = pn;
			break;
		}
	}
	return TRUE;
}

#define LOGE(fmt,arg...)
#define LOGW(fmt,arg...)
#define LOGI(fmt,arg...)

atci_cmd_hdlr_item_t atcmd_table[] = {
  {"AT+MTCFG", AT_MTCFG_USAGE, atci_cmd_mtcfg,   0, 0},
  {"AT+MTPUB", AT_MTPUB_USAGE, atci_cmd_mtpub,   0, 0},
  {"AT+MTSUB", AT_MTSUB_USAGE, atci_cmd_mtsub,   0, 0},
  {"AT+TTDEVICE", AT_TTDEVICE_USAGE, atci_cmd_ttdevice,   0, 0},
  {"AT+TTVOLUME", AT_TTVOLUME_USAGE, atci_cmd_ttvolume,   0, 0},
	{"AT+TTDEBUG", AT_LOG_USAGE, atci_cmd_ttdebug, 0, 0},
	{"AT+REBOOT", AT_REBOOT_USAGE, atci_cmd_reboot, 0, 0},
	{"AT+NBRESET", AT_NBRESET_USAGE, atci_cmd_nbreset, 0, 0},
};

atci_status_t at_command_init(void)
{
  atci_status_t ret = ATCI_STATUS_REGISTRATION_FAILURE;
  //int32_t item_size;

  /* -------  Scenario: register AT handler in CM4 -------  */
  ret = atci_register_handler(atcmd_table, sizeof(atcmd_table) / sizeof(atci_cmd_hdlr_item_t));
  if (ret == ATCI_STATUS_OK) {
    LOGI("at_cmd_init register success\r\n");
  } else {
    LOGE("at_cmd_init register fail\r\n");
  }
  return ret;
}


