/**
 ****************************************************************************************
 *
 * @file leds.c
 *
 * @brief Demo of LED Display via soft-timer. *User should override it*
 *
 ****************************************************************************************
 */
/*
 * DEFINES
 ****************************************************************************************
 */
 #if (LED_PLAY)
#include <stdint.h>
#include "sftmr.h"
#include "gpio.h"
#include "leds.h"
#include "cfg.h"
 #if (DBG_LED)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif
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
    BT_LED,    // led0 BT
    POWER_LED, // led1 POWER
    CAP_LED,   // led2 CAP
};

#define IO_LEDS             (BIT(BT_LED) | BIT(POWER_LED) | BIT(CAP_LED)) // *same PA_LEDS*
#define NB_LEDS             sizeof(PA_LEDS)

/// set leds state, note polar (VCC-->LED-->IO) or (IO-->LED-->GND)
#define SET_LEDS_ON(bits)   GPIO_DAT_SET(bits)
#define SET_LEDS_OFF(bits)  GPIO_DAT_CLR(bits)

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
const led_item_t LED_CONT_ON_ITEM[] = {
    {LED(0)|LED(1)|LED(2), ON(0)|ON(1)|ON(2),  0},
};

/// blink twice mode
const led_item_t LED_HINT_BL_ITEM[] = {
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)},
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)},  
};



/// bt pairing
const led_item_t LED_BT_PAIRING_ITEM[] = {
    {LED(0)|LED(2), ON(0)|OFF(2),  _MS(100)},
    {LED(0)|LED(2), OFF(0)|OFF(2), _MS(100)},
};

/// bt pairing complete
const led_item_t LED_BT_OFF_ITEM[] = {
    {LED(0), OFF(0),  0},
};
const led_item_t LED_BT_ON_ITEM[] = {
    {LED(0), ON(0),  0},
};
/// bt connect back 
const led_item_t LED_BT_CONNECT_BACK_ITEM[] = {
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)},
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)}, 
};

/// cap on
const led_item_t LED_CAP_ON_ITEM[] = {
    {LED(2), ON(2),  0},
};

/// cap off
const led_item_t LED_CAP_OFF_ITEM[] = {
    {LED(2), OFF(2),  0},
};

/// bt delete pair
const led_item_t LED_BT_DELETE_PAIR_ITEM[] = {
    {LED(0),        ON(0),        _MS(50)},
    {LED(0),        OFF(0),       _MS(50)},
};


/// bt delete pair
const led_item_t LED_BATTLOW_ALARM_ITEM[] = {
    {LED(1),        ON(1),        _MS(250)},
    {LED(1),        OFF(1),       _MS(250)},
};

/// change channle
const led_item_t LED_BT_CHANGE_CHANNLE_ITEM[] = {
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)},
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)}, 
};

/// change os
const led_item_t LED_BT_CHANGE_OS_ITEM[] = {
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)},
    {LED(0),        ON(0),        _MS(100)},
    {LED(0),        OFF(0),       _MS(100)}, 
};

/// modes table
#define LED_MODE(rep, mode)    [mode]={mode##_ITEM, sizeof(mode##_ITEM)/sizeof(led_item_t), rep}

const led_mode_t LED_MODE_TAB[] = 
{
    //demo
    LED_MODE(1, LED_SLOW_BL),
    LED_MODE(1, LED_FAST_BL),
    LED_MODE(1, LED_BUSY_BL),
    LED_MODE(1, LED_CONT_ON),
    LED_MODE(0, LED_HINT_BL),
    
    //user
    LED_MODE(1, LED_BT_PAIRING),    
    LED_MODE(1, LED_BT_OFF),
    LED_MODE(0, LED_BT_CONNECT_BACK),
    LED_MODE(0, LED_CAP_ON),
    LED_MODE(0, LED_CAP_OFF),    
    LED_MODE(0, LED_BT_DELETE_PAIR),    
    LED_MODE(1, LED_BATTLOW_ALARM),    
    LED_MODE(0, LED_BT_CHANGE_CHANNLE),    
    LED_MODE(0, LED_BT_CHANGE_OS),
    LED_MODE(1, LED_BT_ON),
};
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/// gpio as output enable and set high.
static __forceinline void leds_io_init(void)
{
    GPIO_DAT_CLR(BIT(BT_LED));
    GPIO_DAT_CLR(BIT(CAP_LED));
    GPIO_DAT_SET(BIT(POWER_LED));
    
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
            {
                SET_LEDS_ON(BIT(PA_LEDS[n]));
                DEBUG("LEDON");
            }
                
            else
            {
                SET_LEDS_OFF(BIT(PA_LEDS[n]));
                DEBUG("LEDOFF");
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

void Leds_Play(uint8_t mode)
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

void Leds_Init(void)
{
    // init gpio
    leds_io_init();
    
    // init led_env
    led_env.tmrid = TMR_ID_NONE;
    led_env.cidx  = 0;
    led_env.mcurr = LED_MODE_MAX;
    led_env.mlast = LED_MODE_MAX;
}

uint8_t read_led_currunt_mode(void)
{
    return(led_env.mcurr);    
}

void hids_led_lock(uint8_t leds)
{
    //NUM_LOCK
    if (leds & 0x01)
    {

    }
    else
    {

    }
    
    //CAPS_LOCK
    if (leds & 0x02)
    {
        //CAP ON
        GPIO_DAT_SET(BIT(CAP_LED));
    }
    else
    {
        //CAP OFF    
        GPIO_DAT_CLR(BIT(CAP_LED));
    }
}
#endif //LED_PLAY
