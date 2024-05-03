//@ hyIC Software Timer

#ifndef _SFTMR_H_
#define _SFTMR_H_

#include <stdint.h>

#define _MS(n)             ((n) / 10) // sftmr in 10ms

#define SFTMR_MAX_ID       (8)
#define SFTMR_INVALID_ID   0xFF

#define SFTMR_MSK_TIMEOUT  ((1 << 16)-1)
#define SFTMR_MAX_TIMEOUT  (SFTMR_MSK_TIMEOUT >> 1)

/// Timer callback function, return next timeout(>0) or stop(0).
typedef uint32_t(*tmr_cb_t)(uint8_t id);

uint32_t currTickCnt(void);

void sfTmrInit(void);

void sfTmrPolling(void);

/// auto find idle timer and start, return id 
uint8_t sfTmrStart(uint16_t to, tmr_cb_t cb);

void sfTmrClear(uint8_t id);

#endif // _SFTMR_H_
