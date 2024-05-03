#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>

enum screen{
    SCRN_INIT,
    SCRN_INFO,
    SCRN_BUSY,
    SCRN_DONE,
    SCRN_DETAIL
};

void dispInit(void);

void dispInfo(void);

void dispBusy(void);

void dispDone(void);

void dispDetail(void);

#endif

