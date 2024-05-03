/**
 ****************************************************************************************
 *
 * @file keys.c
 *
 * @brief keys operation.
 *
 ****************************************************************************************
 */

#include "hyb6x.h"
#include "app.h"
#include "reg_csc.h"
#include "reg_gpio.h"
#include "prf_hogpd.h"
#include "hidkey.h"

#if (DBG_KEYS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<KEYS>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

#define LED1    (1 << 11)
#define LED2    (1 << 10)
#define LEDS    (LED2 | LED1)

#define BTN1    (1 << 12)
#define BTN2    (1 << 14)
#define BTN3    (1 << 15)
#define BTNS    (BTN1 | BTN2 | BTN3)

/// Reports Description @see hid_report_map
enum report_desc
{
    // report id
    REP_NONE         = 0,
    REP_KB           = 1,
    REP_MOUSE        = 2,
    
    // report index and length
    REP_IDX_KB       = 0, // IDX_KB_INPUT
    REP_LEN_KB       = 8, // 8-bytes
    
    REP_IDX_MOUSE    = 2, // IDX_MOUSE_INPUT
    REP_LEN_MOUSE    = 6, // 6-bytes
};


/*
 * FUNCTIONS
 ****************************************************************************************
 */

void hid_leds(uint8_t leds)
{
    if (leds & 0x02/*CAPS_LOCK*/)
        GPIO->DAT_CLR = LED2;
    else
        GPIO->DAT_SET = LED2;
}

uint8_t hid_keybd_send_report(uint8_t code)
{
    uint8_t ret = 0;
    
    #if (HID_KEYBOARD)
    uint8_t kyebd_report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //A
    
    if (code != 0)
    {
        kyebd_report[0] = KEY_BIT_LALT;
        kyebd_report[2] = KEY_PAD_0;
        kyebd_report[3] = KEY_PAD_1;
        kyebd_report[4] = KEY_PAD_2;
        kyebd_report[5] = KEY_PAD_8;
    }
    else
    {
        kyebd_report[2] = code;
    }
    
    ret = hogpd_send_report(app_env.curidx, REP_IDX_KB, 8, kyebd_report);

    #endif
    
    return ret;
}

struct mouse_rep
{
    uint8_t btns;
    int16_t xoft;
    int16_t yoft;
    int8_t  wheel;
} __attribute__((packed));

uint8_t hid_mouse_send_report(int8_t x)
{
    uint8_t ret = 0;
    
    #if (HID_MOUSE)
    struct mouse_rep mouse_report;
    
    mouse_report.btns = 0;
    mouse_report.xoft = x;
    mouse_report.yoft = 0;
    mouse_report.wheel = 0;
    
    ret = hogpd_send_report(app_env.curidx, REP_IDX_MOUSE, sizeof(mouse_report), (const uint8_t *)&mouse_report);
    #endif
    
    return ret;
}

void keys_init(void)
{
    GPIO->DAT_CLR = BTNS;
    GPIO->DIR_CLR = BTNS;
    CSC->CSC_PIO[12].IE = 1;
    CSC->CSC_PIO[12].PUPDCTRL = 2;
    CSC->CSC_PIO[14].IE = 1;
    CSC->CSC_PIO[14].PUPDCTRL = 2;
    CSC->CSC_PIO[15].IE = 1;
    CSC->CSC_PIO[15].PUPDCTRL = 2;
    
    GPIO->DAT_SET = LEDS;
    GPIO->DIR_SET = LEDS;
}

void keys_scan(void)
{
    static uint16_t btn_lvl = BTNS;
    uint8_t ret = 0;
    uint16_t value = (GPIO->PIN) & BTNS;
    uint16_t chng = btn_lvl ^ value;
    btn_lvl = value;
    
    if (chng) {
        uint8_t code = 0;

        if ((chng & BTN1) && ((value & BTN1) == 0)) {
            code = 40;//HID_KEY_ENTER;
        }
        if ((chng & BTN2) && ((value & BTN2) == 0)) {
            code = 82;//HID_KEY_UP;
        }
        if ((chng & BTN3) && ((value & BTN3) == 0)) {
            code = 81;//HID_KEY_DOWN;
        }
        
        DEBUG("keys(val:%X,chng:%X,code:%d)\r\n", btn_lvl, chng, code);
        
        if (app_state_get() >= APP_CONNECTED)
        {
            ret = hid_keybd_send_report(code);
            if (ret == 0) {
                DEBUG("keys Fail(sta:%d)\r\n", ret);
            }
            
            if (code) {
                GPIO->DAT_CLR = LED1;
                hid_mouse_send_report(10);
            }
            else {
                GPIO->DAT_SET = LED1;
                hid_mouse_send_report(-10);
            }
        }
    }
}
