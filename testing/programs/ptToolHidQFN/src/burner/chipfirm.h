#ifndef _CHIPFIRM_H_
#define _CHIPFIRM_H_

#include <stdint.h>

void gpioControlInit(void);
void gpioControlClear(void);

void chipLoadStart(uint8_t action);
void chipLoadSync(uint8_t code);
void chipLoadCont(uint8_t rsp, uint8_t status);

void chipBurnStart(void);
void chipBurnCont(uint8_t rsp, uint8_t status);

#endif // _CHIPFIRM_H_
