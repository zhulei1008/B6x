#ifndef _CHIPTEST_H_
#define _CHIPTEST_H_

#include <stdint.h>

#define RF_TEST_CNT        0x64  //100

void chipTestStart(void);

void chipTestNext(void);

void chipTestCont(uint8_t rsp, uint8_t *payl);

#endif // _CHIPTEST_H_
