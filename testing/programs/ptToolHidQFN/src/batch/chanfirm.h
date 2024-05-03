#ifndef _CHANFIRM_H_
#define _CHANFIRM_H_

#include <stdint.h>

void chanSyncStart(void);

void chanSyncNext(uint8_t status);

void chanBurnStart(uint8_t chans, uint8_t mode);

void chanBurnNext(uint8_t status);

void chanBurnCont(uint8_t rsp, uint8_t status);

void chanRunStart(uint8_t mode);

void chanRunNext(uint8_t status);

void chanRunState(uint8_t state);

#endif // _CHANS_H_
