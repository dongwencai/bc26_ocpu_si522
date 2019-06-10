#ifndef __RIL_MQTT_H__
#define __RIL_MQTT_H__

typedef enum{
	MQTT_FAIL = -1,
	MQTT_SUCCESS,
	MQTT_OPEN_OK,
	MQTT_OPEN_ERR,
	MQTT_CONN_OK,
	MQTT_CONN_ERR,
	MQTT_PUB_OK,
	MQTT_PUB_ERR,
	MQTT_SUB_OK,
	MQTT_SUB_ERR,
	MQTT_STATE_OK,
	MQTT_STATE_ERR,
}mqtt_status_t;


typedef struct{
	bool session;
	u16 keepalive;
	u8 timeout;
	u8 repeat_times;
	u8 version;
}ST_Mqtt_Config_t;

typedef struct{
	char user[32];
	char passwd[32];
}ST_Mqtt_Connect_t;

extern s32 RIL_SOC_QMTCLOSE(void);
extern s32 RIL_SOC_QMTOPEN(char *host, u16 port);
extern s32 RIL_SOC_QMTCFG(ST_Mqtt_Config_t *cfg);
extern s32 RIL_SOC_QMTPUB(char *topic, u8 qos, u16 msgid, char *text);
extern s32 RIL_SOC_QMTCONN(char *clientid, char *user, char *passwd);

extern void RIL_SOC_QMTOPEN_SET_RES(s32 result);
#endif
