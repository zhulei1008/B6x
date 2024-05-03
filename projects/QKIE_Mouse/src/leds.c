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
const uint8_t PA_LEDS[] = 
{
    _BT_LED,  // led0
    _24G_LED,  // led1
    _BATT_LOW_LED,  // led2
};
#define BtLed   LED(0)
#define B24GLed    LED(1)
#define BattLed LED(2)

#define BtLedOn ON(0)
#define BtLedOff OFF(0)

#define B24GLedOn ON(1)
#define B24GLedOff OFF(1)

#define BattLedOn ON(2)
#define BattLedOff OFF(2)
/// iopad of Leds, total number not excced 8. 
//const uint8_t PA_LEDS[] = 
//{
//    PA08,  // led0
//    PA09,  // led1
//    PA10,  // led2
//};

//#define IO_LEDS             (BIT(PA08) | BIT(PA09) | BIT(PA10)) // *same PA_LEDS*
#define IO_LEDS             (BIT(_BT_LED) | BIT(_24G_LED) | BIT(_BATT_LOW_LED)) // *same PA_LEDS*
#define NB_LEDS             sizeof(PA_LEDS)

/// set leds state, note polar (VCC-->LED-->IO) or (IO-->LED-->GND)
#define SET_LEDS_ON(bits)   GPIO_DAT_SET(bits)
#define SET_LEDS_OFF(bits)  GPIO_DAT_CLR(bits)
///poweron mode
const led_item_t LED_POWERON_ITEM[]={
			{BtLed|BattLed|B24GLed,BtLedOn|BattLedOn|B24GLedOn,_MS(100)},
			{BtLed|BattLed|B24GLed,BtLedOff|BattLedOff|B24GLedOff,_MS(2)}

};
///connect back blink mode
const led_item_t LED_CONNECT_BACK_ITEM[]={
			{BattLed|BtLed|B24GLed,BtLedOn|B24GLedOn|BattLedOff,_MS(100)},
			{BtLed|B24GLed,BtLedOff|B24GLedOff,_MS(100)},
			{BtLed|B24GLed,BtLedOn|B24GLedOn,_MS(100)},
			{BtLed|B24GLed,BtLedOff|B24GLedOff,_MS(100)},
			{BtLed|B24GLed,BtLedOn|B24GLedOn,_MS(100)},
			{BtLed|B24GLed,BtLedOff|B24GLedOff,_MS(100)},

};
///paring blink mode
const led_item_t LED_PARING_ITEM[]={
			{BattLed|BtLed|B24GLed,BtLedOn|B24GLedOn|BattLedOff,_MS(200)},
			{BtLed|B24GLed,BtLedOff|B24GLedOff,_MS(200)},


};
///low batt blink mode
const led_item_t LED_LOW_BATT_ITEM[]={
			{BattLed|BtLed|B24GLed,BtLedOff|B24GLedOff|BattLedOff,_MS(1000)},
			{BattLed,BattLedOff,_MS(1000)},


};
/// slow blink mode
const led_item_t LED_SLOW_BL_ITEM[] = {
    {LED(0)|LED(1)|LED(2), ON(0)|OFF(1)|ON(2),  _MS(200)},
    {LED(0)|LED(2),        OFF(0)|OFF(2),       _MS(2000)}    
};

/// fast blink mode
const led_item_t LED_FAST_BL_ITEM[] = {
    {LED(0)|LED(1)|LED(2), ON(0)|OFF(1)|OFF(2), _MS(250)},
    {LED(0)|LED(2),        OFF(0)|ON(2),        _MS(250)},    
};

/// busy blink mode, more fast
const led_item_t LED_BUSY_BL_ITEM[] = {
    {LED(0)|LED(1)|LED(2), OFF(0)|ON(1)|OFF(2),   _MS(50)},
    {LED(0)|LED(1)|LED(2), OFF(0)|OFF(1)|OFF(2),  _MS(50)},
};

/// continued ON mode
//const led_item_t LED_CONT_ON_ITEM[] = {
//    {LED(0)|LED(1)|LED(2), ON(0)|ON(1)|ON(2),  0},
//};
const led_item_t LED_CONT_ON_ITEM[] = {
   {BtLed|B24GLed,BtLedOff|B24GLedOff,0}
};
/// blink twice mode
const led_item_t LED_HINT_BL_ITEM[] = {
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)},
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)}  
};

/// modes table
#define LED_MODE(rep, mode)    [mode]={mode##_ITEM, sizeof(mode##_ITEM)/sizeof(led_item_t), rep}

const led_mode_t LED_MODE_TAB[] = 
{
    LED_MODE(1, LED_SLOW_BL),
    LED_MODE(1, LED_FAST_BL),
    LED_MODE(1, LED_BUSY_BL),
    LED_MODE(1, LED_CONT_ON),
    LED_MODE(0, LED_HINT_BL),
		LED_MODE(0, LED_POWERON),
		LED_MODE(0, LED_CONNECT_BACK),
		LED_MODE(1, LED_PARING),
		LED_MODE(1, LED_LOW_BATT),
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
extern uint8_t work_mode;
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
//    for (uint8_t n = 0; n < NB_LEDS; n++)
//    {
//        // set one by one
//        if (leds & LED(n))
//        {
//            if (stat & ON(n))
//                SET_LEDS_ON(BIT(PA_LEDS[n]));
//            else
//                SET_LEDS_OFF(BIT(PA_LEDS[n]));
//        }
//    }
    if(work_mode==BT_MODE)
    {
        if (leds & LED(0))
        {
            if (stat & ON(0))
                SET_LEDS_ON(BIT(PA_LEDS[0]));
            else
                SET_LEDS_OFF(BIT(PA_LEDS[0]));
        }
        if (leds & LED(2))
        {
            if (stat & ON(2))
                SET_LEDS_ON(BIT(PA_LEDS[2]));
            else
                SET_LEDS_OFF(BIT(PA_LEDS[2]));
        }
    }
    else
    {
        for (uint8_t n = 1; n < NB_LEDS; n++)
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
