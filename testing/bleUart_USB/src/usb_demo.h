#ifndef _USB_DEMO_H_
#define _USB_DEMO_H_

#include <stdint.h>

void usbdInit(void);

void usbdTest(void);

void hid_wakeup(void);
uint8_t hid_keybd_send_report(uint8_t code);
uint8_t hid_mouse_send_report(int8_t x);

#endif // _USB_DEMO_H_
