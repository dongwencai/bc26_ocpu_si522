#ifndef BASE64_H
#define BASE64_H
#include <stdint.h>

#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))

/*
 * out is null-terminated encode string.
 * return values is out length, exclusive terminating `\0'
 */

extern uint32_t base64_encode(char *src, uint32_t len, char *dst);

/*
 * return values is out length
 */
extern uint32_t base64_decode(char *src, uint32_t len, uint8_t *dst);

#endif /* BASE64_H */
