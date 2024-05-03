#ifndef _BEEP_H_
#define _BEEP_H_

#include <stdint.h>

enum {
    TONE_OFF,
    /* User customize */
    TONE_PWR,
    TONE_BTN,
    TONE_OK,
    TONE_FAIL,
    /* end customize */
    
    TONE_MAX
};

void beepInit(void);

void beepTone(uint8_t tone);

#endif  // _BEEP_H_
