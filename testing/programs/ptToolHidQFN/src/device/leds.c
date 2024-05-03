/**
 ****************************************************************************************
 *
 * @file leds.c
 *
 * @brief Demo of LED Display via soft-timer. *User should override it*
 *
 ****************************************************************************************
 */

#if (LED_PLAY)

#include <stdint.h>
#include "sftmr.h"
#include "gpio.h"
#include "leds.h"


/*
 * DEFINES
 ****************************************************************************************
 */
 
#define LED(n)              (1 << (n)) // Bit of Led index
#define ON(n)               (1 << (n)) // Bit of On state
#define OFF(n)              (0 << (n)) // Bit of Off state
#if !defined(_MS)
#define _MS(n)              ((n) / 10) // Time in uint of 10ms
#endif

typedef struct {
    uint8_t           leds;  // iopad bits: LED0~7
    uint8_t           stat;  // state bits: ON or OFF
    uint16_t          time;  // hold times: _MS
} led_item_t;

typedef struct {
    const led_item_t* items;
    uint8_t           count;
    uint8_t           repeat;
} led_mode_t;

typedef struct
{
    uint8_t           tmrid; // soft-timer ID
    uint8_t           cidx;  // curr item index 
    uint8_t           mcurr; // curr led mode
    uint8_t           mlast; // last repeat mode
} led_env_t;

/// global variables
static led_env_t led_env;


/*
 * USER CUSTOMIZE
 ****************************************************************************************
 */

/// iopad of Leds, total number not excced 8. 
const uint8_t PA_LEDS[] = 
{
    PIN_OK_LED,  // led0   
    PIN_FAIL_LED,  // led1
    PIN_BUSY_LED,  // led2
};

#define IO_LEDS             (BIT(PIN_FAIL_LED) | BIT(PIN_BUSY_LED) | BIT(PIN_OK_LED)) // *same PA_LEDS*
#define NB_LEDS             sizeof(PA_LEDS)

/// set leds state, note polar (VCC-->LED-->IO) or (IO-->LED-->GND)
#define SET_LEDS_ON(bits)   GPIO_DAT_SET(bits)
#define SET_LEDS_OFF(bits)  GPIO_DAT_CLR(bits)

const led_item_t LED_READY_ITEM[] =
{
    {LED(2),                OFF(2),              _MS(0)},
};

const led_item_t LED_ERROR_ITEM[] =
{
    {LED(2) | LED(0) | LED(1), OFF(2) | OFF(0) | OFF(1), _MS(500)},
    {LED(1),               ON(1),                _MS(500)}
};

const led_item_t LED_BUSY_ITEM[] =
{
    {LED(2) | LED(0) | LED(1), ON(2) | OFF(0) | OFF(1), _MS(0)},
};

const led_item_t LED_OK_ITEM[] =
{
    {LED(2) | LED(0) | LED(1), OFF(2) | ON(0) | OFF(1),  _MS(0)},
};

const led_item_t LED_FAIL_ITEM[] =
{
    {LED(2) | LED(0) | LED(1), OFF(2) | OFF(0) | ON(1),  _MS(0)},
};

const led_item_t LED_OFF_ITEM[] =
{
    {LED(2) | LED(0) | LED(1), OFF(2) | OFF(0) | OFF(1), _MS(0)},
};

/// modes table
#define LED_MODE(rep, mode)    [mode]={mode##_ITEM, sizeof(mode##_ITEM)/sizeof(led_item_t), rep}

const led_mode_t LED_MODE_TAB[] = 
{
    LED_MODE(1, LED_READY),
    LED_MODE(1, LED_ERROR),

    LED_MODE(1, LED_BUSY),
    LED_MODE(1, LED_OK),
    LED_MODE(1, LED_FAIL),
    LED_MODE(1, LED_OFF),
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/// gpio as output enable
static __forceinline void leds_io_init(void)
{
    SET_LEDS_OFF(IO_LEDS);
    GPIO_DIR_SET(IO_LEDS);
}

/// set curidx state
static uint16_t leds_set_state(uint8_t patt, uint8_t idx)
{
    const led_item_t *item = &(LED_MODE_TAB[patt].items[idx]);
    uint8_t leds = item->leds;
    uint8_t stat = item->stat;
    
    #if (LED_GROUP)
    uint32_t iomsk = 0;
    uint32_t iosta = 0;
    
    for (uint8_t n = 0; n < NB_LEDS; n++)
    {
        if (leds & LED(n))
        {
            iomsk |= BIT(PA_LEDS[n]);
            iosta |= ((stat >> n) & 1UL) << PA_LEDS[n];
        }
    }
    
    // set together
    SET_LEDS_ON(iosta);
    SET_LEDS_OFF(iomsk ^ iosta);
    #else
    for (uint8_t n = 0; n < NB_LEDS; n++)
    {
        // set one by one
        if (leds & LED(n))
        {
            if (stat & ON(n))
                SET_LEDS_ON(BIT(PA_LEDS[n]));
            else
                SET_LEDS_OFF(BIT(PA_LEDS[n]));
        }
    }
    #endif
    
    return item->time;
}

static tmr_tk_t leds_timer_handler(tmr_id_t id)
{
    // update mode index
    led_env.cidx++;
    if (led_env.cidx >= LED_MODE_TAB[led_env.mcurr].count)
    {
        led_env.cidx = 0;
        
        if (!LED_MODE_TAB[led_env.mcurr].repeat)
        {
            if (led_env.mlast == LED_MODE_MAX)
            {
                led_env.tmrid = TMR_ID_NONE; // over to free
                return 0;
            }
            
            // recover last repeat-mode
            led_env.mcurr = led_env.mlast;
        }
    }

    // set leds state
    tmr_tk_t time = leds_set_state(led_env.mcurr, led_env.cidx);
    if (time == 0)
    {
        led_env.tmrid = TMR_ID_NONE; // over to free
        return 0;
    }
    
    return time;
}

void leds_play(uint8_t mode)
{
    if ((mode >= LED_MODE_MAX) || (mode == led_env.mcurr))
        return;
    
    // clear curr timer
    if (led_env.tmrid != TMR_ID_NONE)
    {
        sftmr_clear(led_env.tmrid);
        led_env.tmrid = TMR_ID_NONE;
    }
    
    // record last repeat-mode
    if ((led_env.mcurr != LED_MODE_MAX) && (LED_MODE_TAB[led_env.mcurr].repeat))
    {
        led_env.mlast = led_env.mcurr;
    }
    
    // update mode, start timer if need
    led_env.mcurr = mode;
    led_env.cidx  = 0;
    
    tmr_tk_t time = leds_set_state(led_env.mcurr, led_env.cidx);
    if (time)
    {
        led_env.tmrid = sftmr_start(time, leds_timer_handler);
    }
}

void leds_init(void)
{
    // init gpio
    leds_io_init();
    
    // init led_env
    led_env.tmrid = TMR_ID_NONE;
    led_env.cidx  = 0;
    led_env.mcurr = LED_MODE_MAX;
    led_env.mlast = LED_MODE_MAX;
}

#endif //LED_PLAY
