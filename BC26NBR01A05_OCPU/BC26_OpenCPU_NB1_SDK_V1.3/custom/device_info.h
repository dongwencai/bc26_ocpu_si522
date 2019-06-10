#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__

#include <stdint.h>
#include <stddef.h>
#include <ql_type.h>

typedef struct {
 uint16_t checksum;
 uint8_t clientid[16];
 uint16_t qos;
 uint16_t mode;
 uint16_t bhinterval;
 uint16_t volume;
 uint8_t serverip[20];
 uint16_t serverport;
 uint8_t username[16];
 uint8_t password[16];
 uint16_t version;
 uint16_t sendtimeout;
 uint16_t sendrepeatcnt;
 uint16_t keepalive;
 uint16_t session;
 uint16_t retain;
 uint8_t publishtopic[16];
 uint8_t subscribe[16];
 uint8_t log_item[64];
 uint16_t type;
 uint8_t devicename[32];
 uint16_t timeout;
 uint8_t firmware_ver[16];
 uint8_t bootnotification;
 uint8_t reserve[17];
}device_info_t;

typedef struct{
	uint8_t mode;
}tt_runtime_t;

extern tt_runtime_t tt_rt;
extern device_info_t device;

#define DEVICEINFO_ADDRESS 				0x801F800
#define DEVICE_PARAM(member) (device.member)
#define TTRT_PARAM(member) (tt_rt.member)

extern void device_info_load(void);
extern void device_info_set(uint8_t offset, uint8_t *data, uint8_t len);
#endif

