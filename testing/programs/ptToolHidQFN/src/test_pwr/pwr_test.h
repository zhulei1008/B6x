#ifndef _PWR_TEST_H_
#define _PWR_TEST_H_

#include <stdint.h>

#define PIN_VBG_1P2V  0
//#define ADC_CHANNEL2  4
#define ADC_CHANNEL8  28

enum CURR_TEST
{
    CURR_SLEEP,
    CURR_WORK,
};

void pwr_adcInit(void);

uint16_t pwr_adcGet(void);

void pwr_enterMode(uint8_t mode);

#endif // _PWR_TEST_H_
