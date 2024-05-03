
#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>

enum {
    /* User customize */
	LED_READY,
	LED_ERROR,    
    LED_BUSY,
	LED_OK,
	LED_FAIL,
    LED_OFF,
    /* end customize */

    LED_MODE_MAX
};

void ledInit(uint8_t mode);

void ledPlay(uint8_t mode);

#endif  // _LED_H_