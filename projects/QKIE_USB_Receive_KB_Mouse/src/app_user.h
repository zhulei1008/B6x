#ifndef APP_USER_H_
#define APP_USER_H_

#include "gap.h"
#include "app_actv.h"

#ifndef SCAN_NUM_MAX
#define SCAN_NUM_MAX 2
#endif

enum icon_type
{
    ICON_KB = 0xC1,
    ICON_MS = 0xC2,
};
enum os_type
{
    WINDOWS = 0,
    MAC_IOS = 1,
};
struct bd_addr_icon
{
    struct gap_bdaddr bt_addr;
    uint8_t icon_type;
};

/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */
extern volatile bool g_usbd_ok;
extern volatile uint8_t g_cid_kb;
extern volatile uint8_t g_kb_led;
extern volatile uint8_t g_os_type;
extern volatile uint8_t  g_os_type_back ;
extern volatile uint8_t g_conn_icon_type[SCAN_NUM_MAX];

extern struct bd_addr_icon user_scan_addr_list[SCAN_NUM_MAX];
extern struct bd_addr_icon user_scan_addr_list_store[SCAN_NUM_MAX];
extern uint8_t g_empty[4];
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void usbdInit(void);

void usbd_wakeup(void);

void init_timer_stop(void);

void init_timer_start(void);

uint8_t usbd_mouse_report(void);

uint8_t usbd_kb_report(void);

uint8_t usbd_consumer_report(void);

uint8_t *get_mouse_pkt(void);

uint8_t *get_kb_pkt(void);

uint8_t *get_consumer_pkt(void);

#endif // APP_USER_H_
