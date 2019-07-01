#include "ril.h"
#include "main.h"
#include "crc16.h"
#include "base64.h"
#include "ql_time.h"
#include "ql_timer.h"
#include "ril_mqtt.h"
#include "device_info.h"
#include "ril_network.h"
#include "proc_mqtt_task.h"

struct{
	char imei[32];
	u32 encode_times;
	mqtt_state_t state;
	u32 msg_in_ticks;
	u32 mqtt_encode_tmr_id;
	u32 mqtt_status_query_tmr_id;
	u8 net_query_timeout;
	u8 connect_failed_cnt;
}mqtt;

struct{	
	u16 checksum;
	bool history;
	uint8_t length;
	u32 mid;
	u32 delay_ticks;
	u8 context[244];
}record;

typedef union{
	u8 cid;
	u8 name[12];
}name_cid_t;

#define NET_QUERY_MAX_TIMES 120


int mqtt_send_data(u8 *data, u8 length)
{
	u16 en_len;
	char dst[256], *pdata;
	pdata = Ql_MEM_Alloc(length + 1);
	*pdata = length;
	Ql_memcmp(&pdata[1], data, length);
	Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_MSG_GET, pdata);
}

static void mqtt_start_proc(void)
{
	mqtt.net_query_timeout = NET_QUERY_MAX_TIMES;
	Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, NET_QUERY_STATE, NULL);
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_MQTT_LED_START, 0);

}
static void mqtt_stop_proc(void)
{
	Ql_OS_SendMessage(nfc_task_id ,MSG_ID_APP_TEST, USR_MSG_CARD_ATACHE_E, 0);
	Ql_OS_SendMessage(main_task_id ,MSG_ID_APP_TEST, USR_MSG_MQTT_LED_STOP, 0);
}

static void mqtt_reset_proc(bool reset)
{
	mqtt.connect_failed_cnt ++ ;

	if(mqtt.connect_failed_cnt <= 3 && !reset){
		RIL_SOC_QMTCLOSE();
		mqtt_start_proc();
	}else{
		record.delay_ticks += main_ticks_get() - mqtt.msg_in_ticks;
		if(!record.history){
			if(!Ql_strstr(record.context, "bootNotification")){
				record.history = TRUE;		
				record.checksum = crc16(&record.history, sizeof(record) - 2);
				Ql_SecureData_Store(2, &record, sizeof(record));
			}
		}else{
			if(record.delay_ticks > 1500){
				Ql_memset(&record, 0x00, sizeof(record));
				Ql_SecureData_Store(2, &record, sizeof(record));
			}
		}
		Ql_Reset(0);
		while(1);
	}

}

static void net_state_query_timer(u32 timerId, void* param)
{
	s32 cgreg = 0;

	RIL_NW_GetEGPRSState(&cgreg);
	APP_DEBUG("<--Network State:cgreg=%d-->\r\n",cgreg);
	if((cgreg == NW_STAT_REGISTERED)||(cgreg == NW_STAT_REGISTERED_ROAMING)){
		if(record.mid != 0){			
			Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_CONFIG, NULL);
		}else{
			Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_ENCODE_START, NULL);
		}
		Ql_Timer_Stop(mqtt.mqtt_status_query_tmr_id);
		return;
	}
	if(mqtt.net_query_timeout > 0){
		mqtt.net_query_timeout --;
	}else{
		static s32 send_reset_times = 0x00;
		send_reset_times ++;
		if(send_reset_times > 3){
			mqtt_reset_proc(TRUE);
		}else{
			mqtt_reset_proc(FALSE);
		}
	}
}
static uint8_t bcc(uint8_t *data, uint8_t len)
{
	uint8_t crc = 0x00;
	for(uint8_t i = 0; i < len; i ++){
		crc += data[i];
	}
	return crc;
}

static void mqtt_encode_timer(u32 timerId, void* param)
{
	if(record.mid == 0 || Ql_strlen(mqtt.imei) < 10){		
		ST_Time s;
		if(Ql_strlen(mqtt.imei) < 10){
			RIL_GetIMEI(mqtt.imei);
		}
		if(record.mid != 0){
			Ql_Timer_Stop(mqtt.mqtt_encode_tmr_id);
			return;
		}else{
			Ql_GetLocalTime(&s);
			s.timezone = 8;
			if(s.year > 2018){
				u32 u32second, delay_ticks;
				u8 buffer[256] = {0}, length, i;
				u64 u64second = Ql_Mktime(&s);
				
				u32second = (u64second & 0xffffffff) + 8 * 3600;
				Ql_memcpy(buffer, record.context, record.length);
				delay_ticks = record.delay_ticks + main_ticks_get() - mqtt.msg_in_ticks;
				u32second -=	delay_ticks / 5;record.context[0] = 0xff; record.context[1] = 0x02;
				record.mid = u32second;record.mid = record.mid ? record.mid : 1;
				
				if(record.length > 1){
					Ql_strncpy(&record.context[3], buffer, record.length);length =	record.length + 1;	
					record.context[length + 3] = bcc(&record.context[3], length - 1);
				}else{
					Ql_sprintf(&record.context[3], "%02d", buffer[0]);record.context[5] = ',';
					*(u32 *)&record.context[6] = u32second; length = record.length + 6;
					record.context[length + 3] = bcc(&record.context[3], length - 1);
				}
				record.context[2] = length;
				record.context[length + 4] = 0xff;
				APP_DEBUG("\r\nframe: ");
				for(i = 0; i < length + 5; i ++){
					APP_DEBUG("%x ", record.context[i]);
				}
				APP_DEBUG("\r\n");
				Ql_MKTime2CalendarTime(u32second, &s);
				Ql_Timer_Stop(mqtt.mqtt_encode_tmr_id);
				Ql_memcpy(buffer, record.context, length + 5);
				Ql_memset(record.context, 0x00, sizeof(record.context));
				record.length = base64_encode(buffer, length + 5, record.context);
				APP_DEBUG("%d-%d-%d %d:%d:%d\t%s\r\n", s.year, s.month, s.day, s.hour, s.minute, s.second, record.context);
				Ql_OS_SendMessageFromISR(mqtt_task_id, MSG_ID_APP_TEST, MQTT_CONFIG, NULL);
				Ql_Timer_Stop(mqtt.mqtt_encode_tmr_id);
				return;
			}
		}
		if(mqtt.encode_times){
			mqtt.encode_times --;
		}else{
			mqtt_reset_proc(FALSE);
			Ql_Timer_Stop(mqtt.mqtt_encode_tmr_id);
		}
	}
}

void proc_mqtt_init(void)
{
	mqtt.msg_in_ticks = 0x00;
	mqtt.connect_failed_cnt = 0x00;	
	mqtt.mqtt_encode_tmr_id = 0x109;
	mqtt.mqtt_status_query_tmr_id = 0x108;	
	if(sys_event_id == 0){
		sys_event_id = Ql_OS_CreateEvent();
	}
	Ql_OS_WaitEvent(sys_event_id, EVENT_RUNNING, 5000);
	
	Ql_Timer_Register(mqtt.mqtt_encode_tmr_id, mqtt_encode_timer, NULL);		
	Ql_Timer_Register(mqtt.mqtt_status_query_tmr_id, net_state_query_timer, NULL);		
	
	Ql_SecureData_Read(2, &record, sizeof(record));
	if(record.history){
		if(record.checksum == crc16(&record.history, sizeof(record) - 2)){			
			mqtt_start_proc();
		}else{		
			memset(&record, 0x00, sizeof(record));
			Ql_SecureData_Store(2, &record, sizeof(record));
			return;
		}
	}
	
	if(DEVICE_PARAM(bootnotification)){
		const char *fmt  = "bootNotification:type=%d,mode=%d,device_name=%s,version=%s";
		record.mid = 0x01;
		record.history = FALSE;	
		Ql_sprintf(record.context, fmt, DEVICE_PARAM(type), DEVICE_PARAM(mode), DEVICE_PARAM(devicename),"v1.0.1");
		mqtt_start_proc();
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

static void mqtt_msg_proc(char *pdata)
{
	record.mid = 0x00;
	record.history = FALSE;
	Ql_memset(record.context, 0x00, sizeof(record.context));
	record.length = pdata[0];
	mqtt.msg_in_ticks = main_ticks_get();
	Ql_memcpy(record.context, &pdata[1], record.length);
	mqtt_start_proc();
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
		mqtt_reset_proc(TRUE);
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
	if(RIL_SOC_QMTOPEN(DEVICE_PARAM(serverip), DEVICE_PARAM(serverport)) != MQTT_OPEN_OK){
		mqtt_reset_proc(FALSE);
	}else{
		mqtt.state = MQTT_CONN; 	
		mqtt.connect_failed_cnt = 0x00; 
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_CONN, NULL);
	}
}
static void mqtt_connect_proc(void)
{
	char imei[32] = {0};
	RIL_GetIMEI(imei);
	if(RIL_SOC_QMTCONN(imei, DEVICE_PARAM(username), DEVICE_PARAM(password)) != MQTT_CONN_OK){
		mqtt_reset_proc(FALSE);
	}else{
		mqtt.state = MQTT_PUB;		
		mqtt.connect_failed_cnt = 0x00;
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_PUB, NULL);
	}
}
static void mqtt_publish_proc()
{
	char topic[64];
	if(Ql_strlen(DEVICE_PARAM(publishtopic)) == 0){
		Ql_sprintf(topic, "%s/%d/%d", "server", DEVICE_PARAM(type), record.mid);
	}else{
		Ql_sprintf(topic, "%s/%d/%d", DEVICE_PARAM(publishtopic), DEVICE_PARAM(type), record.mid);
	}
	if(RIL_SOC_QMTPUB(topic, 0, 0, record.context) != MQTT_PUB_OK){
		mqtt_reset_proc(FALSE);
	}else{
		if(Ql_strstr(record.context, "bootNotification")){
			uint8_t data = 0;
			device_info_set(offsetof(device_info_t, bootnotification), &data, 1);
		}else
			{
			if(record.history){
				Ql_memset(&record, 0x00, sizeof(record));
				Ql_SecureData_Store(2, &record, sizeof(record));
			}
		}
		RIL_SOC_QMTCLOSE();
		Ql_OS_SendMessage(mqtt_task_id, MSG_ID_APP_TEST, MQTT_STOP, NULL);
	}
}

void proc_mqtt_task(s32 taskId)
{
	ST_MSG msg;
	proc_mqtt_init();
	while(1){
		Ql_OS_GetMessage(&msg); 	
		APP_DEBUG("%s\t%d\t%x\r\n", __func__, __LINE__, msg.param1);
		switch(msg.param1){
			case MQTT_STOP:
				mqtt_stop_proc();
				break;
			case MQTT_MSG_GET:
				mqtt_msg_proc((char *)msg.param2);
				Ql_MEM_Free((char *)msg.param2);
				break;
			case NET_QUERY_STATE:
				Ql_Timer_Start(mqtt.mqtt_status_query_tmr_id, 1000, TRUE);
				break;
			case MQTT_ENCODE_START:				
				mqtt.encode_times = 60;
				Ql_Timer_Start(mqtt.mqtt_encode_tmr_id, 500, TRUE);
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
		}		
	}
}
//
//switch(mqtt.state){
//	case MQTT_MSG_GET:
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

