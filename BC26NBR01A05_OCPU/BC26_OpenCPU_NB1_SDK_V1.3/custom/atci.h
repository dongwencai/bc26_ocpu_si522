/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef ATCI_H
#define ATCI_H
#include "stdint.h"
#include <Ql_type.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define AT_LINE_LENGTH 256
#define ATCI_UART_TX_FIFO_BUFFER_SIZE      (256)

typedef enum {
    ATCI_STATUS_REGISTRATION_FAILURE = -2,   /**< Failed to register the AT command handler table. */
    ATCI_STATUS_ERROR = -1,                  /**< An error occurred during the function call. */
    ATCI_STATUS_OK = 0                       /**< No error occurred during the function call. */
} atci_status_t;

typedef enum {
    ATCI_CMD_MODE_READ,        /**< Read mode command, such as "AT+CMD?". */
    ATCI_CMD_MODE_ACTIVE,      /**< Active mode command, such as "AT+CMD". */
    ATCI_CMD_MODE_EXECUTION,   /**< Execute mode command, such as "AT+CMD=<op>". */
    ATCI_CMD_MODE_TESTING,     /**< Test mode command, such as "AT+CMD=?". */
    ATCI_CMD_MODE_INVALID      /**< The input command doesn't belong to any of the four types. */
} atci_cmd_mode_t;

typedef enum {

    ATCI_RESPONSE_FLAG_AUTO_APPEND_LF_CR = 0x00000002,    /**< Auto append "\r\n" at the end of the response string. */
    ATCI_RESPONSE_FLAG_URC_FORMAT = 0x00000010,           /**< The URC notification flag. */
    ATCI_RESPONSE_FLAG_QUOTED_WITH_LF_CR = 0x00000020,    /**< Auto append "\r\n" at the begining and end of the response string. */
    ATCI_RESPONSE_FLAG_APPEND_OK = 0x00000040,            /**< Auto append "OK\r\n" at the end of the response string. */
    ATCI_RESPONSE_FLAG_APPEND_ERROR = 0x00000080          /**< Auto append "ERROR\r\n" at the end of the response string. */            
} atci_response_flag_t;

typedef struct {
    uint8_t  response_buf[ATCI_UART_TX_FIFO_BUFFER_SIZE]; /**< The response data buffer. */
    uint16_t response_len;                                /**< The actual data length of response_buf. */
    uint32_t response_flag;                               /**< For more information, please refer to #atci_response_flag_t. */
} atci_response_t;

typedef struct {
    char             *string_ptr;    /**< The input data buffer. */
    uint32_t         string_len;     /**< The response data buffer. */
    uint32_t         name_len;       /**< AT command name length. For example, in "AT+EXAMPLE=1,2,3", name_len = 10 (without symbol "=") */ 
    uint32_t         parse_pos;      /**< The length after detecting the AT command mode. */
    atci_cmd_mode_t mode;            /**< For more information, please refer to #atci_cmd_mode_t. */

} atci_parse_cmd_param_t;

typedef atci_status_t (*at_cmd_hdlr_fp) (atci_parse_cmd_param_t *parse_cmd);

typedef struct{
	char *cmd;
	char *param[6];
	uint8_t param_cnt;
	char context[256];
}atci_param_t;

typedef struct {
    char           *command_head;    /**< AT command string. */
		char 					*command_usage;
    at_cmd_hdlr_fp command_hdlr;     /**< The command handler, please refer to #at_cmd_hdlr_fp. */
    uint32_t       hash_value1;      /**< Use hash value 1 in the AT command string to accelerate search for the command handler. */
    uint32_t       hash_value2;      /**< Use hash value 2 in the AT command string to accelerate search for the command handler.*/

} atci_cmd_hdlr_item_t;

typedef struct {
    atci_cmd_hdlr_item_t *item_table;       /**< For more information, please refer to #atci_cmd_hdlr_item_t. */
    uint32_t              item_table_size;  /**< The command item size in the item table. */

} atci_cmd_hdlr_table_t;

#define ATCI_SET_OUTPUT_PARAM_STRING( s, ptr, len, flag)	    \
    {                                                           \
        memcpy((void*)s->response_buf, (uint8_t*)ptr, len);       \
        s->response_len = (uint32_t)(len);                        \
        s->response_flag = (uint32_t)(flag);                      \
    }

//extern atci_param_t *atci_cmd_param_parse(char *param);
extern bool atci_param_parse(atci_param_t *pb, char *param, char *delimit);

extern atci_status_t  atci_register_handler(atci_cmd_hdlr_item_t *table, int32_t hdlr_number);

extern atci_status_t  atci_send_response(atci_response_t *response);


#ifdef __cplusplus
}
#endif


#endif
