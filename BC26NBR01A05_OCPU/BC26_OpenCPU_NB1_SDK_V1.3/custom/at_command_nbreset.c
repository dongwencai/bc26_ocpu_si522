#include <string.h>
#include "at_command_nbreset.h"

atci_status_t atci_cmd_nbreset(atci_parse_cmd_param_t *parse_cmd)
{
	atci_response_t response[1] = {0};
	switch (parse_cmd->mode){
		case ATCI_CMD_MODE_READ:		// rec: AT+TEST?
			Ql_sprintf(response->response_buf, "ERROR\r\n");
			response->response_len = Ql_strlen((char *)response->response_buf);
			atci_send_response(response);
			break;
		case ATCI_CMD_MODE_ACTIVE:	// rec: AT+TEST
			break;
		case ATCI_CMD_MODE_EXECUTION:
			Ql_strcpy((char *)response->response_buf, "ERROR\r\n");
			response->response_len = Ql_strlen((char *)response->response_buf);
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




