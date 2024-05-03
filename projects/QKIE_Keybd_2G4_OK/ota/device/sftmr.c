#include "hyb5x.h"
#include "sys.h"
#include "sftmr.h"

#define USE_    0 // 0-SYSTICK, 1-

static volatile bool gTickFlag = false;
static volatile uint32_t gTickCount = 0;


uint32_t currTickCnt(void)
{
    return gTickCount;
}

static bool IsTickFlag(void)
{
    if (gTickFlag)
    {
        gTickFlag = false;
        return true;
    }

    return false;
}

#if (USE_)
#include "rcc.h"
#include "reg_tim.h"

void _IRQHandler(void)
{
    // Each  tick Interrupt is 10ms
    ->IDR.UI = 1;  // Disable UI Interrupt

    if (->RIF.UI)  // Judeg Interrupt Flag
    {
        gTickCount++;
        gTickFlag = true;
    }

    ->ICR.UI = 1; //Clear Interrupt Flag
    ->IER.UI = 1; // Enable UI Interrupt
}

static void SFTMR_INIT(void)
{
    gTickFlag = 0;
    gTickCount = 0;

    rccAPB1ClockCtrl(RCC_APB1_, 1);
    ->CR1.CEN = 1;
    ->PSC = ((sysGetClock() / 100000) - 1); // 10us
    ->ARR = 999;
    ->CNT = 0; // the counter start from 0
    ->CR1.OPM = 0; // disable one-mode
    ->CR1.ARPE = 1; //arr register is buffered
    ->EGR.UG = 1; //auto update arr and init counter, psc counter is cleared
    ->CR1.CEN = 1;
    ->ICR.UI = 1;//clear interrupt of update
    ->IER.UI = 1;

    NVIC_EnableIRQ(_IRQn);
}

#else //USE_SYSTICK

#define TICK_MS(n)   (16000*n)   // 16MHz Clock
#define SFTMR_MS     TICK_MS(10) // sftmr init

void SysTick_Handler(void)
{
    gTickFlag = 1;
    gTickCount++;
}

static void SFTMR_INIT(void)
{
    gTickFlag = 0;
    gTickCount = 0;

    SysTick_Config(SFTMR_MS);
}
#endif //(USE_)


typedef struct
{
    tmr_cb_t func;  // callback func
    uint32_t time;  // timecnt value
} sftmr_t;

// global timer array
static sftmr_t gSfTimer[SFTMR_MAX_ID];

void sfTmrInit(void)
{
    SFTMR_INIT();

    // init array
    for (uint8_t id = 0; id < SFTMR_MAX_ID; id++)
    {
        gSfTimer[id].func = NULL;
    }
}

void sfTmrPolling(void)
{
    if (!IsTickFlag())
        return;

    uint32_t now = currTickCnt();

    for (uint8_t id = 0; id < SFTMR_MAX_ID; id++)
    {
        if ((gSfTimer[id].func != NULL)
            && ((uint32_t)(now - gSfTimer[id].time) <= SFTMR_MAX_TIMEOUT))
        {
            uint32_t timeout = gSfTimer[id].func(id);
            if (timeout)
            {
                // peroid mode, reload timeout
                gSfTimer[id].time = now + timeout;
            }
            else
            {
                // single mode, stop timer
                gSfTimer[id].func = NULL;
            }
        }
    }
}

static uint8_t sfTmrGetIdle(void)
{
    uint8_t id;

    for (id = 0; id < SFTMR_MAX_ID; id++)
    {
        if (gSfTimer[id].func == NULL)
            return id; // find first idle!
    }

    // none idle timer
    return SFTMR_INVALID_ID;
}

uint8_t sfTmrStart(uint16_t to, tmr_cb_t cb)
{
    uint8_t id;

    if (cb == NULL)
    {
        // cb MUST NOT BE NULL
        return SFTMR_INVALID_ID;
    }

    if (to > SFTMR_MAX_TIMEOUT)
        to = SFTMR_MAX_TIMEOUT;

    id = sfTmrGetIdle();
    if (id != SFTMR_INVALID_ID)
    {
        // set timer
        gSfTimer[id].func = cb;
        gSfTimer[id].time = currTickCnt() + (uint32_t)to;
    }

    return id;
}

void sfTmrClear(uint8_t id)
{
    if (id < SFTMR_MAX_ID)
    {
        gSfTimer[id].func = NULL;
    }
}
