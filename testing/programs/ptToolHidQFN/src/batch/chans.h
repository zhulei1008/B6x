#ifndef _CHANS_H_
#define _CHANS_H_

#include <stdint.h>

#define CHANS_CNT          (8)

void chansInit(void);

void chansSelect(uint8_t idx);

void chansReset(void);

uint8_t chansDetect(void);

#endif // _CHANS_H_
