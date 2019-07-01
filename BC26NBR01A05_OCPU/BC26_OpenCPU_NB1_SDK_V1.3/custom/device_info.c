#include "crc16.h"
#include "main.h"
#include <ql_stdlib.h>
#include "ql_system.h"
#include "device_info.h"

device_info_t device;
tt_runtime_t tt_rt = {0};

const device_info_t default_param = {
	.checksum = 0x0000,
	.clientid = {0},
	.qos = 0x00,
	.mode = 0,
	.volume = 10,
	.serverip = "47.92.121.156",
	.serverport = 1883,
	.username = "huaxi_cc",
	.password = "Usi0195[]",
	.version = 3,
	.sendtimeout = 10,
	.sendrepeatcnt = 3,
	.keepalive = 60,
	.session = 0,
	.retain = 0,
	.publishtopic = "server",
	.subscribe = {0},
	.log_item = {0},
	.type = 3,
	.devicename = {0},
	.timeout = 180,
	.bootnotification = 1,
	.reserve = {0},
};

void device_info_store(void)
{
	s32 ret;
	device_info_t di;
	uint16_t checksum = 0x00;
	checksum = crc16((uint8_t *)&device.clientid, sizeof(device_info_t) - 2);
	device.checksum = checksum;
	Ql_SecureData_Read(1, &di, sizeof(device_info_t));

	if(device.checksum != di.checksum){
		ret = Ql_SecureData_Store(1, (uint8_t *)&device, sizeof(device_info_t));
		if(di.checksum != crc16((uint8_t *)&di.clientid, sizeof(device_info_t) - 2)){
			ret = Ql_SecureData_Store(1, (uint8_t *)&device, sizeof(device_info_t));
		}
	}
}

void device_info_set(uint8_t offset, uint8_t *data, uint8_t len)
{
	if(offset != offsetof(device_info_t, bootnotification)){
		device.bootnotification = 1;
	}

	Ql_memcpy((uint8_t *)&device + offset, data, len);

	device_info_store();
}

void device_info_load()
{ 
	device_info_t di;
	uint16_t checksum = 0x00;
	Ql_SecureData_Read(1, &di, sizeof(device_info_t));
	checksum = crc16((uint8_t *)&di.clientid, sizeof(device_info_t) - 2);
	
	if(di.checksum == checksum && 	Ql_strlen(di.serverip) && Ql_strlen(di.publishtopic)){
		Ql_memcpy(&device, &di, sizeof(device_info_t));
		APP_DEBUG("%s\t%d\r\n", __func__, device.bootnotification);
	}else{
	APP_DEBUG("Load default config\r\n");
		Ql_memcpy(&device, &default_param, sizeof(device_info_t));
		device_info_store();	
	}
}

