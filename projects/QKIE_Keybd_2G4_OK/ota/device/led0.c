/**
****************************************************************************************
*
* @file led.c
*
* @brief LED Mode Display.
*
****************************************************************************************
*/

#include <stdint.h>
#include "led.h"
#include "gpio.h"
#include "sftmr.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define LED(n)   (1 << (n))
#define ON(n)    (0 << (n)) // VCC->Led->IO
#define OFF(n)   (1 << (n))

#define LED_MODE(repeat, items) {items, sizeof(items)/sizeof(items[0]), repeat}

typedef struct
{
    uint8_t           gpio;  // gpio mask: led0~7
    uint8_t           value; // value mask
    uint16_t          time;  // time ms
} led_item_t;

typedef struct
{
    const led_item_t *items;
    uint8_t           count;
    uint8_t           repeat;
} led_mode_t;

typedef struct
{
    uint8_t           tmid;  // softTimer ID

    uint8_t           cidx;  // curr item index
    uint8_t           mcurr; // curr led mode
    uint8_t           mlast; // last repeat mode
} led_t;


/* User customize */
#if (ROLE_BURNER)
#define LED0_IO  PIN_BUSY_LED  //BLUE  LOW
#define LED1_IO  PIN_OK_LED    //GREEN HIGH
#define LED2_IO  PIN_FAIL_LED  //RED   HIGH

const led_item_t LED_READY_ITEM[] =
{
#if (SC1000_QCR)
    {LED(0),                OFF(0),              _MS(0)},
#else
    {LED(0),                ON(0),               _MS(200)},
    {LED(0),                OFF(0),              _MS(1000)}
#endif
};

const led_item_t LED_ERROR_ITEM[] =
{
    {LED(0) | LED(1) | LED(2), OFF(0) | OFF(1) | OFF(2), _MS(500)},
    {LED(2),               ON(2),                _MS(500)}
};

const led_item_t LED_BUSY_ITEM[] =
{
#if (SC1000_QCR)
    {LED(0) | LED(1) | LED(2), OFF(0) | OFF(1) | OFF(2), _MS(0)},
#else
    {LED(0) | LED(1) | LED(2), OFF(0) | OFF(1) | OFF(2), _MS(200)},
    {LED(0),               ON(0),                _MS(200)},
#endif
};

const led_item_t LED_OK_ITEM[] =
{
    {LED(0) | LED(1) | LED(2), OFF(0) | ON(1) | OFF(2),  _MS(0)},
};

const led_item_t LED_FAIL_ITEM[] =
{
    {LED(0) | LED(1) | LED(2), OFF(0) | OFF(1) | ON(2),  _MS(0)},
};

const led_item_t LED_OFF_ITEM[] =
{
    {LED(0) | LED(1) | LED(2), OFF(0) | OFF(1) | OFF(2), _MS(0)},
};
#endif

#if (ROLE_BATCH)
#define LED1_IO  PIN_OK_LED   //BLUE    HIGH
#define LED2_IO  PIN_FAIL_LED //RED HIGH

const led_item_t LED_READY_ITEM[] =
{
    {LED(1) | LED(2), ON(1) | OFF(2),  _MS(200) },
    {LED(1),        OFF(1),        _MS(1000)},
};

const led_item_t LED_ERROR_ITEM[] =
{
    {LED(1) | LED(2), OFF(1) | ON(2),  _MS(500)},
    {LED(2),        OFF(2),        _MS(500)},
};

const led_item_t LED_BUSY_ITEM[] =
{
    {LED(1) | LED(2), ON(1) | OFF(2),  _MS(200)},
    {LED(1),        OFF(1),        _MS(200)},
};

const led_item_t LED_OK_ITEM[] =
{
    {LED(1) | LED(2), ON(1) | OFF(2),  _MS(0)},
};

const led_item_t LED_FAIL_ITEM[] =
{
    {LED(1) | LED(2), OFF(1) | ON(2),  _MS(0)},
};

const led_item_t LED_OFF_ITEM[] =
{
    {LED(1) | LED(2), OFF(1) | OFF(2), _MS(0)},
};
#endif

const led_mode_t LED_MODE_TAB[] =
{
    LED_MODE(1, LED_READY_ITEM),
    LED_MODE(1, LED_ERROR_ITEM),

    LED_MODE(1, LED_BUSY_ITEM),
    LED_MODE(1, LED_OK_ITEM),
    LED_MODE(1, LED_FAIL_ITEM),
    LED_MODE(1, LED_OFF_ITEM),
};
/* end customize */


/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static led_t gLed;


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/* User customize */
static uint16_t ledSet(uint8_t patt, uint8_t idx)
{
    const led_item_t *item = &(LED_MODE_TAB[patt].items[idx]);

    // gpio output
#if (ROLE_BURNER)
    if ((item->gpio & LED(0)) == OFF(0))
    {
        if (item->value & LED(0))
            gpioDataSet(LED0_IO);
        else
            gpioDataClr(LED0_IO);
    }
#endif

    if (item->gpio & LED(1))
    {
        if ((item->value & LED(1)) == OFF(1))
            gpioDataClr(LED1_IO);
        else
            gpioDataSet(LED1_IO);
    }
    if (item->gpio & LED(2))
    {
        if ((item->value & LED(2)) == OFF(2))
            gpioDataClr(LED2_IO);
        else
            gpioDataSet(LED2_IO);
    }

    return item->time;
}
/* end customize */

static uint32_t ledTimer(uint8_t id)
{
    if (id != gLed.tmid)
        return 0;

    // update index
    gLed.cidx++;
    if (gLed.cidx >= LED_MODE_TAB[gLed.mcurr].count)
    {
        gLed.cidx = 0;

        if (!LED_MODE_TAB[gLed.mcurr].repeat)
        {
            if (gLed.mlast != LED_MODE_MAX)
            {
                gLed.mcurr = gLed.mlast;
            }
            else
            {
                return 0;
            }
        }
    }

    // set state
    return ledSet(gLed.mcurr, gLed.cidx);
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void ledPlay(uint8_t mode)
{
    uint16_t timeout;

    if ((mode >= LED_MODE_MAX) || (mode == gLed.mcurr))
        return;

    if (gLed.tmid != SFTMR_INVALID_ID)
    {
        sfTmrClear(gLed.tmid);
        gLed.tmid = SFTMR_INVALID_ID;
    }

    if ((gLed.mcurr != LED_MODE_MAX)
        && (LED_MODE_TAB[gLed.mcurr].repeat))
    {
        gLed.mlast = gLed.mcurr;
    }
    gLed.mcurr = mode;
    gLed.cidx  = 0;

    timeout = ledSet(mode, 0);
    if (timeout /*&& (LED_MODE_TAB[mode].count > 1)*/)
    {
        gLed.tmid = sfTmrStart(timeout, ledTimer);
    }
}

void ledInit(uint8_t mode)
{
    // gpio init
#if (ROLE_BURNER)
    gpioDataSet(LED0_IO);
    gpioDirSet(LED0_IO);
#endif

    gpioDataClr(LED1_IO);
    gpioDataClr(LED2_IO);

    gpioDirSet(LED1_IO);
    gpioDirSet(LED2_IO);

    // init gLed
    gLed.tmid  = SFTMR_INVALID_ID;
    gLed.mcurr = LED_MODE_MAX;
    gLed.mlast = LED_MODE_MAX;
    //#if (!SC1000_QCR)
    ledPlay(mode);
    //#endif
}
