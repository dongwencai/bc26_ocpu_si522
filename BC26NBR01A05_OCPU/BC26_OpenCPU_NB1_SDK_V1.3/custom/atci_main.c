#include <stdio.h>
#include "atci_main.h"
#include "device_info.h"

#define LOGE(fmt,arg...)
#define LOGW(fmt,arg...)
#define LOGI(fmt,arg...)

static Enum_SerialPort *g_port;
atci_cmd_hdlr_table_t g_atci_cm4_general_hdlr_tables[ATCI_MAX_GNENERAL_TABLE_NUM] = {{0}};

atci_status_t atci_init_int(void);

uint32_t atci_uart_send_data(Enum_SerialPort *port, uint8_t *buf, uint32_t buf_len)
{
  uint32_t ret_len = 0;
	ret_len = Ql_UART_Write(*port, buf, buf_len);
  return ret_len;
}

/* ATCI main body related */
atci_status_t atci_local_init(void)
{
  atci_status_t ret =  ATCI_STATUS_ERROR;

  /* Init Queue */

  ret =  ATCI_STATUS_OK;
  LOGI("atci_local_init() success \r\n");
  return ret;
}

atci_status_t atci_init_int(void)
{
  atci_status_t ret = ATCI_STATUS_OK;

  LOGI("atci_init_int(), enter\r\n");
  if (at_command_init() == ATCI_STATUS_OK) {
    LOGI("atci_init_int() success \r\n");
  } else {
    ret = ATCI_STATUS_ERROR;
    LOGE("atci_init_int() fail \r\n");
  }
  return ret;
}

atci_status_t atci_init(Enum_SerialPort *    port)
{
  atci_status_t ret = ATCI_STATUS_ERROR;
	g_port = port;
  ret = atci_local_init();
  if (ret != ATCI_STATUS_OK) {
    return ret;
  }

  return atci_init_int();
}

uint32_t atci_port_send_data(Enum_SerialPort * port, uint8_t* buf, uint32_t buf_len)
{
  uint32_t data_len = 0;
  data_len = atci_uart_send_data(port, buf, buf_len);	
  return data_len;
}

atci_status_t atci_send_data_int(uint8_t* data, uint32_t data_len)
{
  atci_status_t ret = ATCI_STATUS_OK;
  uint32_t sent_len = 0;
	
  sent_len = atci_port_send_data(g_port, data, data_len);
  if (sent_len == 0){
    ret = ATCI_STATUS_ERROR;
    LOGE("atci_send_data_int() send data fail\r\n");
  }

  LOGI("atci_send_data_int() send data len:%d\r\n", sent_len);
  return ret;
}

atci_status_t atci_send_data(uint8_t* data, uint32_t data_len)
{
  uint32_t sent_len = 0;
	
  sent_len = atci_port_send_data(g_port, data, data_len);
  if (sent_len == 0){
    LOGE("atci_send_data() send data fail\r\n");
		return ATCI_STATUS_ERROR;
  }

	return ATCI_STATUS_OK;
}

