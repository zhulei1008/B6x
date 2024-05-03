/**
 ****************************************************************************************
 *
 * @file sftmr.h
 *
 * @brief Demo of Soft Timer Module. *User should override it*
 *
 ****************************************************************************************
 */
#include <stdbool.h>
#include "string.h"
#include "b6x.h"
#include "sftmr.h"
/*
 * DEFINITIONS
 ****************************************************************************************
 */
 #if (DBG_SFT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/// Number of Soft-Timer Instances
#if !defined(SFTMR_NUM)
#define SFTMR_NUM               (4)
#endif

#if (SFTMR_NUM > 15)
    #error "Number of Soft-Timer instances exceed 'field' bits(@see struct sftmr_env_tag)"
#endif

/// Timer Source: SysTick INT,  INT
#define TMS_SysTick             0
#define TMS_CTMR                1

#if !defined(SFTMR_SRC)
#define SFTMR_SRC               (TMS_CTMR)
#endif

/// Tick add operation
#define TMR_TICK_ADD(tk1, tk2)  (((tk1) + (tk2)) & SFTMR_TICKS_MSK)
/// Tick timeout arrived
#define TMR_TICK_OUT(now, out)  ((tmr_tk_t)((now) - (out)) <= SFTMR_DELAY_MAX)

/// Timer ID valid
#define TMR_ID_VALID(tmid)      (((tmid) > 0) && ((tmid) <= SFTMR_NUM))
/// Timer TK range
#define TMR_TK_RANGE(delay)     ((delay) = ((delay) > SFTMR_DELAY_MAX) ? SFTMR_DELAY_MAX : (delay))

/// Timer interrupt flag bit, MSB of 'sftmr_env.field'
#define TMR_SRC_IFLG_BIT        (1 << 15)

/// Soft-Timer environment structure
typedef struct sftmr_env_tag
{
    // timer tick of source
    volatile tmr_tk_t ticks;
    // timer field of each instance
    uint16_t field;
    
    // timer tables of func and time
    tmr_cb_t func[SFTMR_NUM];
    tmr_tk_t time[SFTMR_NUM];
} sftmr_env_t;

/// global variables
static struct sftmr_env_tag sftmr_env;
/*
 * TIMER DRIVER
 ****************************************************************************************
 */
#if ((SFTMR_SRC == TMS_CTMR) || (SFTMR_SRC == TMS_SysTick))

/// Interrupt Mode Timer
static __forceinline void _timer_irq(void)
{
    ++sftmr_env.ticks;
    sftmr_env.field |= TMR_SRC_IFLG_BIT;
}

static __forceinline bool _timer_arise(void)
{
    if (sftmr_env.field & TMR_SRC_IFLG_BIT)
    {
        sftmr_env.field &= ~TMR_SRC_IFLG_BIT;
        return true;
    }
    return false;
}

/// Timer Source - CTMR
#if (SFTMR_SRC == TMS_CTMR)
#include "timer.h"

/// CTMR prescaler and Auto-reload value
#if !defined(TMS_CTMR_PSC)
    #define TMS_CTMR_PSC        (159) // 159=(16MHz/100000 - 1), 10us
#endif
#if !defined(TMS_CTMR_ARR)
    #define TMS_CTMR_ARR        (999) // 999=(1000 - 1), 1000x10us=10ms
#endif

//  configure as auto-reload and update interrupt
static __forceinline void _timer_init(void)
{
    ctmr_init(TMS_CTMR_PSC, TMS_CTMR_ARR);
    ctmr_ctrl(TMR_PERIOD_MODE, TMR_IR_UI_BIT);
    
    NVIC_EnableIRQ(CTMR_IRQn);
}

void CTMR_IRQHandler(void)
{
    // Disable UI Interrupt
   CTMR->IDR.Word = TMR_IR_UI_BIT; 
    
    if (CTMR->RIF.Word & TMR_IR_UI_BIT)
    {
        // Clear Interrupt Flag
        CTMR->ICR.Word = TMR_IR_UI_BIT; 
        
        _timer_irq();
    }
    
    // Enable UI Interrupt
    CTMR->IER.Word = TMR_IR_UI_BIT;
}
#endif

/// Timer Source - SysTick
#if (SFTMR_SRC == TMS_SysTick)

#if !defined(TMS_SysTick_ARR)
    #define TMS_SysTick_ARR     (16 * 10000) // 10ms(16MHz CLK)
#endif

// SysTick configure
static __forceinline void _timer_init(void)
{
    SysTick_Config(TMS_SysTick_ARR);
}

void SysTick_Handler(void)
{
    _timer_irq();
}
#endif

#else // Other Timer

static __forceinline void _timer_init(void)
{
    // todo
}

static __forceinline bool _timer_arise(void)
{
    // todo
    return false;
}

#endif


/*
 * EXPORT FUNCTIONS
 ****************************************************************************************
 */

/// Init timer source
void Sftmr_Init(void)
{
    // clear env
    sftmr_env.ticks = 0;
    sftmr_env.field = 0;
    
    // init timer
    _timer_init();
}

/// Schedule timer event of callback.
void sftmr_schedule(void)
{
    if (!_timer_arise())
        return;
			//DEBUG("sft1");
    if (sftmr_env.field)
    {
        tmr_tk_t now = sftmr_env.ticks;
        
        for (tmr_id_t idx = 0; idx < SFTMR_NUM; idx++)
        {
            if (sftmr_env.field & (1 << idx))
            {
                // Call func, if time has arrived
                if ((sftmr_env.func[idx] != NULL) && (TMR_TICK_OUT(now, sftmr_env.time[idx])))
                {
                    tmr_tk_t delay = (sftmr_env.func[idx])(idx + 1);
                    
                    if (delay > 0)
                    {
                        // continue mode, reload
                        sftmr_env.time[idx] = TMR_TICK_ADD(now, delay);
                    }
                    else
                    {
                        // single mode, stop
                        sftmr_env.field    &= ~(1 << idx);
                        //sftmr_env.func[idx] = NULL;
                    }
                }
            }
        }
    }
}

/// Find free/unused timer
static __forceinline tmr_id_t find_free_tmr(void)
{
    for (tmr_id_t idx = 0; idx < SFTMR_NUM; idx++)
    {
        if (((sftmr_env.field & (1 << idx)) == 0) /*&& (sftmr_env.func[idx] == NULL)*/)
        {
            return idx + 1; // found timer
        }
    }

    return 0;
}

/// Start timer, callback 'func' after 'delay' ticks post.
tmr_id_t sftmr_start(tmr_tk_t delay, tmr_cb_t func)
{
    tmr_id_t tmr_id = find_free_tmr();
    
    if (func && tmr_id)
    {
        tmr_id_t idx = tmr_id - 1;

        TMR_TK_RANGE(delay);
        
        // set delay time, enable timer
        sftmr_env.func[idx] = func;
        sftmr_env.time[idx] = TMR_TICK_ADD(sftmr_env.ticks, delay);
        sftmr_env.field    |= (1 << idx);
    }

    return tmr_id;
}

/// Clear/Free 'tmr_id' timer instance
void sftmr_clear(tmr_id_t tmr_id)
{
    if (TMR_ID_VALID(tmr_id))
    {
        tmr_id_t idx = tmr_id - 1;
        
        sftmr_env.field    &= ~(1 << idx);
        sftmr_env.func[idx] = NULL;
    }
}

/// Get current ticks
tmr_tk_t sftmr_tick(void)
{
    return sftmr_env.ticks;
}

/// Blocking to wait 'delay' ticks arrived
void sftmr_wait(tmr_tk_t delay)
{
	tmr_tk_t time = TMR_TICK_ADD(sftmr_env.ticks, delay);	
	
    // Wait time arrived, should keep schedule
	while (!TMR_TICK_OUT(sftmr_env.ticks, time))
    {
        sftmr_schedule();
    }
}
