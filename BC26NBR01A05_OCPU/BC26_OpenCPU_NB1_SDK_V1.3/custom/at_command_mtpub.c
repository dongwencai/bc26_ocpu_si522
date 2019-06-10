#include <string.h>
#include "device_info.h"
#include "at_command_mtpub.h"

static bool atci_cmd_execution(char *cmd)
{
	char topic[16] = {0};
	atci_param_t parse_param;
	if(!atci_param_parse(&parse_param, cmd, "AT+%[^=]"))	return FALSE;
	if(parse_param.param_cnt == 1){
		if(parse_param.param[0][0] == '"'){
			Ql_sscanf(&parse_param.param[0][1], "%[^\"]", topic);
			device_info_set(offsetof(device_info_t, publishtopic), (uint8_t *)topic, 16);
			return TRUE;
		}
	}
	return FALSE;
}

atci_status_t atci_cmd_mtpub(atci_parse_cmd_param_t *parse_cmd)
{
	atci_response_t response[1] = {0};
	
	switch (parse_cmd->mode){
		case ATCI_CMD_MODE_READ:		// rec: AT+TEST?
			Ql_sprintf(response->response_buf, "+MTPUB:\"%s\"\r\n", DEVICE_PARAM(publishtopic));
			response->response_len = Ql_strlen((char *)response->response_buf);
			response->response_flag |= ATCI_RESPONSE_FLAG_APPEND_OK;  // ATCI will help append OK at the end of resonse buffer
			atci_send_response(response);
			break;
		case ATCI_CMD_MODE_ACTIVE:	// rec: AT+TEST
			// assume the active mode is invalid and we will return "ERROR"
			Ql_strcpy((char *)response->response_buf, "ERROR\r\n");
			response->response_len = Ql_strlen((char *)response->response_buf);
			atci_send_response(response);
			break;
		case ATCI_CMD_MODE_EXECUTION: // rec: AT+TEST=<p1>	the handler need to parse the parameters
			//parsing the parameter
			if(atci_cmd_execution(parse_cmd->string_ptr)){
				response->response_len = 0;
				response->response_flag |= ATCI_RESPONSE_FLAG_APPEND_OK;
			}else{
				Ql_strcpy((char *)response->response_buf, "ERROR\r\n");
				response->response_len = Ql_strlen((char *)response->response_buf);
			}
			atci_send_response(response);
			break;
		default :
			Ql_strcpy((char *)response->response_buf, "ERROR\r\n");
			response->response_len = Ql_strlen((char *)response->response_buf);
			atci_send_response(response);
			break;
	}
	return ATCI_STATUS_OK;
}


