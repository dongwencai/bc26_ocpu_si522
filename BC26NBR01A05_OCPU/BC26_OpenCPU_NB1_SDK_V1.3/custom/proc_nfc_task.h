#ifndef __PROC_NFC_TASK_H__
#define __PROC_NFC_TASK_H__

typedef enum {
	CARD_INIT,
	CARD_IDLE,
	CARD_PSM,
	CARD_READ,
	CARD_SEARCH,
	CARD_AUTHENT,
}card_step_t;

#endif
