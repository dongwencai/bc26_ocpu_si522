#include "ril.h"
#include "main.h"
#include "crc16.h"
#include "base64.h"
#include "ql_timer.h"
#include "ril_mqtt.h"
#include "device_info.h"
#include "ril_network.h"
#include "proc_mqtt_task.h"

struct{
	mqtt_state_t state;
	u32 mqtt_bh_tmr_id;
	u32 mqtt_status_query_tmr_id;
	u8 net_query_timeout;
	u8 connect_failed_cnt;
}mqtt;

struct{
	u16 checksum;
	bool history;
	uint8_t reserve;
	u8 context[250];
}record;

#define NET_QUERY_MAX_TIMES 120


int mqtt_send_data(u8 *data, u16 length)
{
	u16 en_len;
	char dst[256], *pdata;
	en_len = base64_encode(data, length, dst);
	pdata = Ql_MEM_Alloc(en_len + 1);
	Ql_strncpy(pdata, dst, en_len + 1);
	Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_IDLE, pdata);
}

static void net_state_query_timer(u32 timerId, void* param)
{
	s32 cgreg = 0;
	RIL_NW_GetEGPRSState(&cgreg);
	APP_DEBUG("<--Network State:cgreg=%d-->\r\n",cgreg);
	if((cgreg == NW_STAT_REGISTERED)||(cgreg == NW_STAT_REGISTERED_ROAMING)){
		Ql_Timer_Stop(mqtt.mqtt_status_query_tmr_id);
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_CONFIG, NULL);
		return;
	}

}

static void mqtt_bh_timer(u32 timerId, void* param)
{
	
	Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_NUM, NULL);

}

void proc_mqtt_init(void)
{
	mqtt.state = MQTT_IDLE;
	mqtt.connect_failed_cnt = 0x00;
	mqtt.mqtt_bh_tmr_id = 0x109;
	mqtt.mqtt_status_query_tmr_id = 0x108;	
	Ql_Timer_Register(mqtt.mqtt_bh_tmr_id, mqtt_bh_timer, NULL);
	Ql_Timer_Register(mqtt.mqtt_status_query_tmr_id, net_state_query_timer, NULL);
	Ql_SecureData_Read(2, &record, sizeof(record));
	if(record.history){
		if(record.checksum != crc16(&record.history, sizeof(record) - 2)){
			memset(&record, 0x00, sizeof(record));
			Ql_SecureData_Store(2, &record, sizeof(record));
		}else{
			mqtt.state = NET_QUERY_STATE;			
			mqtt.net_query_timeout = NET_QUERY_MAX_TIMES;
			Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, NET_QUERY_STATE, NULL);
			return;
		}
	}
	
	if(DEVICE_PARAM(bootnotification)){
		const char *fmt  = "bootNotification:type=%d,device_name=%s,version=%s";
		record.history = FALSE;	
		mqtt.state = NET_QUERY_STATE; 		
		mqtt.net_query_timeout = NET_QUERY_MAX_TIMES;
		APP_DEBUG("%s\t%d\r\n", __func__, __LINE__);
		Ql_sprintf(record.context, fmt, DEVICE_PARAM(type), DEVICE_PARAM(devicename),"v1.0.1");
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, NET_QUERY_STATE, NULL);
		Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_MQTT_LED_START, 0);
		return;
	}
	
	Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_CARD_ATACHE_E, 0);
}

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{    
  if(Ql_RIL_FindLine(line, len, "OK")){  
    return  RIL_ATRSP_SUCCESS;
  }else if (Ql_RIL_FindLine(line, len, "ERROR")){  
    return  RIL_ATRSP_FAILED;
  }else if (Ql_RIL_FindString(line, len, "+CME ERROR")){
    return  RIL_ATRSP_FAILED;
  }else if (Ql_RIL_FindString(line, len, "+CMS ERROR:")){
    return  RIL_ATRSP_FAILED;
  }else{

	}
  return RIL_ATRSP_CONTINUE; //continue wait
}

static void mqtt_idle_proc(char *pdata)
{
	ST_MSG msg;
//	char *pdata = NULL;
//	Ql_OS_GetMessage(&msg); 	
//	pdata = (char *)msg.param2;
	record.history = FALSE;
	strcpy(record.context, pdata);
//	Ql_MEM_Free(pdata);
	mqtt.state = NET_QUERY_STATE;
	mqtt.net_query_timeout = NET_QUERY_MAX_TIMES;
	Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, NET_QUERY_STATE, NULL);

	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_MQTT_LED_START, 0);
	return;
}

static void net_query_proc(void)
{
	s32 cgreg = 0;
	RIL_NW_GetEGPRSState(&cgreg);
	APP_DEBUG("<--Network State:cgreg=%d-->\r\n",cgreg);
	if((cgreg == NW_STAT_REGISTERED)||(cgreg == NW_STAT_REGISTERED_ROAMING)){
		mqtt.state = MQTT_CONFIG;
		return;
	}
	if((-- mqtt.net_query_timeout) == 0){
		mqtt.connect_failed_cnt = 4;
		mqtt.state = MQTT_RESET;
	}
	Ql_Sleep(1000);
}
static void mqtt_config_proc(void)
{
	ST_Mqtt_Config_t config;
	config.version = DEVICE_PARAM(version);
	config.session = DEVICE_PARAM(session);
	config.keepalive = DEVICE_PARAM(keepalive);
	config.timeout = DEVICE_PARAM(sendtimeout);
	config.repeat_times = DEVICE_PARAM(sendrepeatcnt);
	RIL_SOC_QMTCFG(&config);	
	Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_QUERY_STATE, NULL);
	mqtt.state = MQTT_QUERY_STATE;
}
static void mqtt_query_state_proc(void)
{
	APP_DEBUG("MQTT query state...\r\n");
	if(RIL_SOC_QMTSTAT() == MQTT_CONN_OK){
		mqtt.state = MQTT_PUB;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_PUB, NULL);
	}else{
		
		mqtt.state = MQTT_OPEN;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_OPEN, NULL);
	}
}
static void mqtt_open_proc(void)
{
	if(RIL_SOC_QMTOPEN(DEVICE_PARAM(serverip), DEVICE_PARAM(serverport)) == MQTT_OPEN_OK){
		mqtt.state = MQTT_CONN;		
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_CONN, NULL);
		mqtt.connect_failed_cnt = 0x00;
	}else{
		mqtt.state = MQTT_RESET;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_RESET, NULL);
	}
}
static void mqtt_connect_proc(void)
{
	char imei[32] = {0};
	RIL_GetIMEI(imei);
	if(RIL_SOC_QMTCONN(imei, DEVICE_PARAM(username), DEVICE_PARAM(password)) != MQTT_CONN_OK){
		mqtt.state = MQTT_RESET;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_RESET, NULL);
	}else{
		mqtt.state = MQTT_PUB;		
		mqtt.connect_failed_cnt = 0x00;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_PUB, NULL);
	}
}
static void mqtt_publish_proc()
{
	if(RIL_SOC_QMTPUB(DEVICE_PARAM(publishtopic), 2, 1, record.context) != MQTT_PUB_OK){
		mqtt.state = MQTT_RESET;
		
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_RESET, NULL);
	}else{
		if(Ql_strstr(record.context, "bootNotification")){
			uint8_t data = 0;
			device_info_set(offsetof(device_info_t, bootnotification), &data, 1);
			APP_DEBUG("%s\t%d\r\n", __func__, DEVICE_PARAM(bootnotification));
		}else
			{
			if(record.history){
				memset(&record, 0x00, sizeof(record));
				Ql_SecureData_Store(2, &record, sizeof(record));
			}
		}
		mqtt.state = MQTT_CLOSE;
		
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_CLOSE, NULL);
	}
}

static void mqtt_reset_proc(void)
{
	mqtt.connect_failed_cnt ++ ;

	if(mqtt.connect_failed_cnt <= 3){
		RIL_SOC_QMTCLOSE();
		mqtt.state = NET_QUERY_STATE;
		mqtt.net_query_timeout = NET_QUERY_MAX_TIMES;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, NET_QUERY_STATE, NULL);
	}else{
		if(!record.history){
			record.history = TRUE;
			record.checksum = crc16(&record.history, sizeof(record) - 2);
			Ql_SecureData_Store(2, &record, sizeof(record));
		}
		Ql_Reset(0);
	}

}

static void mqtt_close_proc(void)
{
	RIL_SOC_QMTCLOSE();
	mqtt.state = MQTT_IDLE;
	Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_CARD_ATACHE_E, 0);
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_MQTT_LED_STOP, 0);
}

void proc_mqtt_task(s32 taskId)
{
	ST_MSG msg;

	Ql_OS_WaitEvent(sys_event_id, EVENT_RUNNING, 5000);
	proc_mqtt_init();
	Ql_Timer_Start(mqtt.mqtt_bh_tmr_id, 3000, TRUE);

	while(1){
		Ql_OS_GetMessage(&msg); 	
		APP_DEBUG("%s\t%d\t%x\r\n", __func__, __LINE__, msg.param1);
		switch(msg.param1){
			case MQTT_IDLE:
				mqtt_idle_proc((char *)msg.param2);
				break;
			case NET_QUERY_STATE:
				Ql_Timer_Start(mqtt.mqtt_status_query_tmr_id, 1000, TRUE);
				break;
			case MQTT_CONFIG:
				mqtt_config_proc();
				break;
			case MQTT_QUERY_STATE:
				mqtt_query_state_proc();
				break;
			case MQTT_OPEN:
				mqtt_open_proc();
				break;
			case MQTT_CONN:
				mqtt_connect_proc();
				break;
			case MQTT_PUB:
				mqtt_publish_proc();
				break;
			case MQTT_CLOSE:
				mqtt_close_proc();
				break;
			case MQTT_RESET:
				mqtt_reset_proc();
				break;
		}		
	}
}
//
//switch(mqtt.state){
//	case MQTT_IDLE:
//		mqtt_idle_proc();
//		break;
//	case NET_QUERY_STATE:
//		net_query_proc();
//		break;
//	case MQTT_CONFIG:
//		mqtt_config_proc();
//		break;
//	case MQTT_QUERY_STATE:
//		mqtt_query_state_proc();
//		break;
//	case MQTT_OPEN:
//		mqtt_open_proc();
//		break;
//	case MQTT_CONN:
//		mqtt_connect_proc();
//		break;
//	case MQTT_PUB:
//		mqtt_publish_proc();
//		break;
//	case MQTT_CLOSE:
//		mqtt_close_proc();
//		break;
//	case MQTT_RESET:
//		mqtt_reset_proc();
//		break;
//}

