#ifndef __MQTT_MAIN_H__
#define __MQTT_MAIN_H__

typedef enum{
	MQTT_IDLE,
	NET_QUERY_STATE,
	MQTT_QUERY_STATE,
	MQTT_OPEN,
	MQTT_CONN,
	MQTT_PUB,
	MQTT_CLOSE,
	MQTT_CONFIG,
	MQTT_RESET,
	MQTT_NUM,
}mqtt_state_t;

//#define MQTT_SUCCESS 0
//#define MQTT_FAIL -1
extern void proc_mqtt_init(void);
extern void proc_mqtt_task(s32 taskId);
extern int mqtt_send_data(u8 *data, u16 length);

#endif