#include "ril.h"
#include "main.h"
#include "ql_type.h"
#include "ril_mqtt.h"

static s8 open_result, conn_result, conn_state, pub_result, sub_result, mqtt_err, close_result;

void RIL_SOC_QMTOPEN_SET_RES(s32 result)
{
	open_result = result;
	Ql_OS_SetEvent(sys_event_id, EVENT_MQTT_OPEN_RESP);
}

void RIL_SOC_QMTCONN_SET_RES(s32 result)
{
	conn_result = result;
	Ql_OS_SetEvent(sys_event_id, EVENT_MQTT_CONN_RESP);
}

void RIL_SOC_QMTCONN_SET_URC(s32 state)
{
	conn_state = state;
	Ql_OS_SetEvent(sys_event_id, EVENT_MQTT_CONN_UCR);
}

void RIL_SOC_QMTPUB_SET_RES(s32 result)
{
	pub_result = result;
	Ql_OS_SetEvent(sys_event_id, EVENT_MQTT_PUB_RESP);
}

void RIL_SOC_QMTSTAT_SET_URC(s32 err_code)
{
	mqtt_err = err_code;
}
void RIL_SOC_QMTCLOSE_SET_RESP(s32 result)
{
	close_result = result;
	Ql_OS_SetEvent(sys_event_id, EVENT_MQTT_CLOSE_RESP);
}

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
  APP_DEBUG("[MQTT_ATResponse_Handler] %s\r\n", (u8*)line);
  
  if (Ql_RIL_FindLine(line, len, "OK")){  
    return  RIL_ATRSP_SUCCESS;
  }else if (Ql_RIL_FindLine(line, len, "ERROR")){  
    return  RIL_ATRSP_FAILED;
  }else if (Ql_RIL_FindString(line, len, "+CME ERROR")){
    return  RIL_ATRSP_FAILED;
  }else if (Ql_RIL_FindString(line, len, "+CMS ERROR:")){
    return  RIL_ATRSP_FAILED;
  }
  
  return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_SOC_QMTOPEN(char *host, u16 port)
{
	s32 ret, event;
	char strAT[64];
	Ql_sprintf(strAT, "AT+QMTOPEN=0,\"%s\",%d\n", host, port);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,ATResponse_Handler,0);
  if (RIL_AT_SUCCESS != ret){
    return MQTT_OPEN_ERR;
  }
	event = Ql_OS_WaitEvent(sys_event_id, EVENT_MQTT_OPEN_RESP, 41000);
	if(event & EVENT_MQTT_OPEN_RESP){
		return open_result == 0 ? MQTT_OPEN_OK : MQTT_OPEN_ERR;
	}
	return MQTT_OPEN_ERR;
}

s32 RIL_SOC_QMTCFG(ST_Mqtt_Config_t *cfg)
{
	s32 ret, event;
	char strAT[64];
	Ql_sprintf(strAT, "AT+QMTCFG=\"keepalive\",0,%d\r\n", cfg->keepalive);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_Handler, NULL, 0);
	if (RIL_AT_SUCCESS != ret){
		return MQTT_FAIL;
	}
	Ql_sprintf(strAT, "AT+QMTCFG=\"session\",0,%d\r\n", cfg->session);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_Handler, NULL, 0);
	if (RIL_AT_SUCCESS != ret){
		return MQTT_FAIL;
	}
	Ql_sprintf(strAT, "AT+QMTCFG=\"timeout\",0,%d,%d,0\r\n", cfg->timeout, cfg->repeat_times);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_Handler, NULL, 0);
	if (RIL_AT_SUCCESS != ret){
		return MQTT_FAIL;
	}
	Ql_sprintf(strAT, "AT+QMTCFG=\"version\",0,%d\r\n", cfg->version);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_Handler, NULL, 0);
	if (RIL_AT_SUCCESS != ret){
		return MQTT_FAIL;
	}
	return MQTT_SUCCESS;
}

s32 RIL_SOC_QMTCONN(char *clientid, char *user, char *passwd)
{
	s32 ret, event;
	char strAT[64];

	Ql_sprintf(strAT, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n",clientid, user, passwd);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
	if (RIL_AT_SUCCESS != ret){
		APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
		return MQTT_CONN_ERR;
	}
	event = Ql_OS_WaitEvent(sys_event_id, EVENT_MQTT_CONN_RESP, 41000);
	if(event & EVENT_MQTT_CONN_RESP){
		return conn_result == 0 ? MQTT_CONN_OK : MQTT_CONN_ERR;
	}
	
	APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
	return  MQTT_CONN_ERR;
}

s32 RIL_SOC_QMTPUB(char *topic, u8 qos, u16 msgid, char *text)
{
	s32 ret, event;
	char strAT[64];

	Ql_sprintf(strAT, "AT+QMTPUB=0,%d,%d,0,\"%s\",\"%s\"\r\n", msgid, qos, topic, text);	
	APP_DEBUG("%s\t%s\r\n", __func__, strAT);
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
	if (RIL_AT_SUCCESS != ret){
		APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
		return ret;
	}	
	event = Ql_OS_WaitEvent(sys_event_id, EVENT_MQTT_PUB_RESP, 41000);
	if(event & EVENT_MQTT_PUB_RESP){
		APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
		return pub_result == 0 ? MQTT_PUB_OK : MQTT_PUB_ERR;
	}
	APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
	return  ret;
}

s32 RIL_SOC_QMTSUB(char *topic)
{
	s32 ret, event;
	char strAT[64];

	Ql_sprintf(strAT, "AT+QMTSUB=0,1,\"%s\",2\r\n", topic);	
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
	if (RIL_AT_SUCCESS != ret){
		return ret;
	}	
	event = Ql_OS_WaitEvent(sys_event_id, EVENT_MQTT_SUB_RESP, 41000);
	if(event & EVENT_MQTT_SUB_RESP){
		return sub_result == 0 ? MQTT_SUB_OK : MQTT_SUB_ERR;
	}
	return  ret;
}

s32 RIL_SOC_QMTCLOSE(void)
{
	s32 ret, event;
	char strAT[64];
	Ql_sprintf(strAT, "AT+QMTCLOSE=0\r\n");	
	Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
	event = Ql_OS_WaitEvent(sys_event_id, EVENT_MQTT_CLOSE_RESP, 300);
	if(event & EVENT_MQTT_CLOSE_RESP){
		return close_result == 0 ? MQTT_SUCCESS : MQTT_FAIL;
	}
	return MQTT_SUCCESS;
}

s32 RIL_SOC_QMTSTAT(void)
{
	s32 ret, event;
	char strAT[64];

	Ql_sprintf(strAT, "AT+QMTCONN?\r\n");	
	ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_Handler, NULL, 300);
	if (RIL_AT_SUCCESS != ret){
		return ret;
	}
	event = Ql_OS_WaitEvent(sys_event_id, EVENT_MQTT_CONN_UCR, 300);
	if(event & EVENT_MQTT_CONN_UCR){
		return conn_state == 2 ? MQTT_CONN_OK : MQTT_CONN_ERR;
	}
	return  MQTT_CONN_ERR;
}

