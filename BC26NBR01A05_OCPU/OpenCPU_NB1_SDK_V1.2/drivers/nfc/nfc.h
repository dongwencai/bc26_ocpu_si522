#ifndef __NFC_H__
#define __NFC_H__
#include <stdint.h>

#define NFC_SUCCESS 0
#define NFC_FAIL 		1
#define NFC_ERR 		-1

#define NFC_CARD_INC	1
#define NFC_CARD_DEC 0

#define CARD_NO_LEN 9

typedef struct nfs_ops{
	int8_t (*init)();
	int8_t (*search)(uint8_t *puid);
	int8_t (*authent)(uint8_t *puid, uint8_t key_type, uint8_t *key,  uint8_t block_nr);
	int8_t (*block_write)(const void *pbuf, uint8_t block_nr);
	int8_t (*block_read)(void *pbuf, uint8_t block_nr);
	int8_t (*value_opt)(uint8_t *puid, uint8_t opt_code, uint32_t value, uint8_t block_nr);

	void *priv;
}nfc_ops_t;

#endif
