#ifndef APP_USER_H_
#define APP_USER_H_

#include <stdint.h>

/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */
extern volatile uint16_t pkt_sidx, pkt_eidx;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

void usbdInit(void);

void usbd_wakeup(void);

void init_timer_start(void);

void init_timer_stop(void);

uint8_t usbd_mouse_report(void);

uint8_t *get_mouse_pkt(void);

#endif // APP_USER_H_
