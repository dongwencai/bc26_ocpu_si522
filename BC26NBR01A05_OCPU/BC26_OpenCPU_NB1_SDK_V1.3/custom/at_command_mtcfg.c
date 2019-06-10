#include <ql_stdlib.h>
#include "device_info.h"
#include "at_command_mtcfg.h"

static bool atci_cmd_execution(char *cmd)
{
	uint16_t value = 0;
	char  ctx[64] ={0};
	atci_param_t parse_param;
	if(!atci_param_parse(&parse_param, cmd, "AT+%[^=]"))	return FALSE;

	if(parse_param.param_cnt > 0){
		if(Ql_strcmp(parse_param.param[0], "\"version\"") == 0){
			if(parse_param.param_cnt == 2){
				Ql_sscanf(parse_param.param[1], "%d", &value);
				device_info_set(offsetof(device_info_t, version), (uint8_t *)&value, 2);
				return TRUE;
			}
		}else if(Ql_strcmp(parse_param.param[0], "\"session\"") == 0){
			if(parse_param.param_cnt == 2){
				Ql_sscanf(parse_param.param[1], "%d", &value);
				device_info_set(offsetof(device_info_t, session), (uint8_t *)&value, 2);
				return TRUE;
			}
		}else if(Ql_strcmp(parse_param.param[0], "\"keepalive\"") == 0){
			if(parse_param.param_cnt == 2){
				Ql_sscanf(parse_param.param[1], "%d", &value);
				device_info_set(offsetof(device_info_t, keepalive), (uint8_t *)&value, 2);
				return TRUE;
			}
		}else if(Ql_strcmp(parse_param.param[0], "\"timeout\"") == 0){
			if(parse_param.param_cnt == 3){
				Ql_sscanf(parse_param.param[1], "%d", &value);
				device_info_set(offsetof(device_info_t, sendtimeout), (uint8_t *)&value, 2);
				Ql_sscanf(parse_param.param[2], "%d", &value);
				device_info_set(offsetof(device_info_t, sendrepeatcnt), (uint8_t *)&value, 2);
				return TRUE;
			}
		}else if(Ql_strcmp(parse_param.param[0], "\"server\"") == 0){
			if(parse_param.param_cnt == 5){
				if(parse_param.param[1][0] == '"' && parse_param.param[3][0] == '"' && parse_param.param[4][0] == '"'){
					Ql_sscanf(&parse_param.param[1][1], "%[^\"]", ctx);
					device_info_set(offsetof(device_info_t, serverip), (uint8_t *)ctx, strlen(ctx) + 1);
					Ql_sscanf(parse_param.param[2], "%d", &value);
					device_info_set(offsetof(device_info_t, serverport), (uint8_t *)&value, 2);
					Ql_sscanf(&parse_param.param[3][1], "%[^\"]", ctx);
					device_info_set(offsetof(device_info_t, username), (uint8_t *)ctx, strlen(ctx) + 1);
					Ql_sscanf(&parse_param.param[4][1], "%[^\"]", ctx);
					device_info_set(offsetof(device_info_t, password), (uint8_t *)ctx, strlen(ctx) + 1);
					return TRUE;
					
				}
			}
		}
	}	

	return FALSE;
}
atci_status_t atci_cmd_mtcfg(atci_parse_cmd_param_t *parse_cmd)
{
	char buf[64];
	atci_response_t response[1] = {0};
	
	switch (parse_cmd->mode){
		case ATCI_CMD_MODE_READ:		// rec: AT+TEST?
			Ql_sprintf(response->response_buf, "+MTCFG:\"version\",%d\r\n", DEVICE_PARAM(version));
			Ql_sprintf(buf, "+MTCFG:\"keepalive\",%d\r\n", DEVICE_PARAM(keepalive));Ql_strcat(response->response_buf, buf);
			Ql_sprintf(buf, "+MTCFG:\"session\",%d\r\n", DEVICE_PARAM(session));Ql_strcat(response->response_buf, buf);
			Ql_sprintf(buf, "+MTCFG:\"timeout\",%d,%d\r\n", DEVICE_PARAM(sendtimeout), DEVICE_PARAM(sendrepeatcnt));
			Ql_strcat(response->response_buf, buf);
			Ql_sprintf(buf, "+MTCFG:\"server\",\"%s\",%d,", DEVICE_PARAM(serverip), DEVICE_PARAM(serverport));
			Ql_strcat(response->response_buf, buf);
			Ql_sprintf(buf, "\"%s\",\"%s\"\r\n", DEVICE_PARAM(username), DEVICE_PARAM(password));
			Ql_strcat(response->response_buf, buf);
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

