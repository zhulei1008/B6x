/**
****************************************************************************************
*
* @file btn.c
*
* @brief Button Scan via soft timer.
*
****************************************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "btn.h"
#include "reg_gpio.h"
#include "reg_iopad.h"
#include "sftmr.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/* User customize */
#if (ROLE_BATCH)
#define BTN_CNT   2
#define BTN0_IO   PIN_START
#define BTN1_IO   PIN_OLED_PG
#define BTN_ALL   ((1<<BTN0_IO) | (1<<BTN1_IO))
#else //(ROLE_BURNER)
#define BTN_CNT   1
#define BTN0_IO   PIN_START
#define BTN_ALL   (1<<BTN0_IO)
#endif

//#define BTN_MSK   (BTN_ALL >> BTN0_IO)
#define BTN_MSK   ((1<<BTN_CNT) - 1)

#define SCAN_INV  _MS(20)

/* end customize */

//#define BTN(n)    (1 << (BTN_IO[n] - BTN0_IO))
#define BTN(n)    (1 << (n))

#define TCNT_DCLK (_MS( 200) / SCAN_INV)
#define TCNT_LONG (_MS(1000) / SCAN_INV)
#define VERY_LONG (_MS(3000) / SCAN_INV)

typedef struct
{
    uint16_t tcnt : 12;
    uint16_t event: 4;
} btn_sta_t;

typedef struct
{
    btn_func_t func;    // event call
    uint8_t    tmid;    // softTimer ID
    uint8_t    level;   // gpio val
    uint8_t    keys;    // real keys
    uint8_t    trig;    // trigger
    btn_sta_t  state[BTN_CNT];
} btn_t;

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static btn_t gBtn;


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/* User customize */
static uint8_t btnGetLvl(void)
{
    //return (uint8_t)(((GPIOA->PIN & BTN_ALL) >> BTN0_IO) ^ BTN_MSK);
    uint8_t lvl = ((GPIOA->PIN >> BTN0_IO) & 0x01);

#if (ROLE_BATCH)
    lvl |= (((GPIOA->PIN >> BTN1_IO) & 0x01) << 1);
#endif
    return (lvl ^ BTN_MSK);
}

static void btnOnEvt(void)
{
    for (uint8_t n = 0; n < BTN_CNT; n++)
    {
        if (gBtn.keys & BTN(n)) // key press
        {
            if (gBtn.trig & BTN(n)) // on trig
            {
#if (TRIG_BTN)
                gBtn.func(n, BTN_PRESS);
#endif
                if (gBtn.state[n].event == BTN_CLICK)
                {
                    if (gBtn.state[n].tcnt < TCNT_DCLK)
                    {
                        gBtn.state[n].event = BTN_DOUBLE;
                    }
                }
                else
                {
                    gBtn.state[n].event = BTN_CLICK;
                }
                gBtn.state[n].tcnt = 0;
            }
            else
            {
                if (gBtn.state[n].event != BTN_IDLE)
                {
                    gBtn.state[n].tcnt++;
                    if (gBtn.state[n].tcnt == TCNT_LONG)
                    {
                        gBtn.state[n].event = BTN_LONG;
                        gBtn.func(n, BTN_LONG);
                    }
                    else if (gBtn.state[n].tcnt >= VERY_LONG)
                    {
                        gBtn.state[n].event = BTN_IDLE;
                        gBtn.func(n, BTN_VERY_LONG);
                    }
                }
            }
        }
        else // key release
        {
            if (gBtn.trig & BTN(n)) // on trig
            {
#if (TRIG_BTN)
                gBtn.func(n, BTN_RELEASE);
#endif
                if (gBtn.state[n].event == BTN_DOUBLE)
                {
                    gBtn.state[n].event = BTN_IDLE;
                    gBtn.func(n, BTN_DOUBLE);
                }
                gBtn.state[n].tcnt = 0;
            }
            else
            {
                if ((gBtn.state[n].event == BTN_CLICK) && (++gBtn.state[n].tcnt >= TCNT_DCLK))
                {
                    gBtn.state[n].event = BTN_IDLE;
                    gBtn.func(n, BTN_CLICK);
                }
            }
        }
    }
}
/* end customize */

static uint32_t btnTimer(uint8_t id)
{
    uint8_t level, keys;

    if (id != gBtn.tmid)
        return 0;

    // debounce
    level  = btnGetLvl(); // gpio value
    keys = (level & gBtn.level) | ((level ^ gBtn.level) & gBtn.keys); // real value

    // trigger
    gBtn.trig  = keys ^ gBtn.keys;
    gBtn.keys  = keys;
    gBtn.level = level;

    // events
    btnOnEvt();

    return SCAN_INV;
}


/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void btnScan(btn_func_t func)
{
    if (gBtn.tmid != SFTMR_INVALID_ID)
    {
        // Unregister
        sfTmrClear(gBtn.tmid);

        memset(&gBtn, 0, sizeof(btn_t));
        gBtn.tmid = SFTMR_INVALID_ID;
        //gBtn.func = NULL;
    }

    if (func)
    {
        gBtn.func = func;
        gBtn.tmid = sfTmrStart(SCAN_INV, btnTimer);
    }
}

void btnInit(btn_func_t func)
{
    // gpio init
    IOMCTL->PIOA[BTN0_IO].Word = 0x00000300; // Input&PullUp
#if (ROLE_BATCH)
    IOMCTL->PIOA[BTN1_IO].Word = 0x00000300; // Input&PullUp
#endif
    GPIOA->DIRCLR = BTN_ALL;

    // init gBtn
    memset(&gBtn, 0, sizeof(btn_t));
    gBtn.tmid = SFTMR_INVALID_ID;

    if (func)
    {
        gBtn.func = func;
        gBtn.tmid = sfTmrStart(SCAN_INV, btnTimer);
    }
}
