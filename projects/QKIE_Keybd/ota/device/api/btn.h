#ifndef _BTN_H_
#define _BTN_H_

#include <stdint.h>

enum btn_event {
    /* User customize */
    BTN_IDLE,

#if (TRIG_BTN)
    BTN_PRESS,
    BTN_RELEASE,
#endif

    BTN_CLICK,
    BTN_DOUBLE,
    BTN_LONG,
    BTN_VERY_LONG,
    /* end customize */
    
    BTN_EVT_MAX
};

typedef void(*btn_func_t)(uint8_t id, uint8_t evt);

void btnInit(btn_func_t cb);

void btnScan(btn_func_t cb);

#endif  // _BTN_H_
