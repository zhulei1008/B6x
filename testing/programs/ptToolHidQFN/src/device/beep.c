/**
****************************************************************************************
*
* @file beep.c
*
* @brief Buzzer.
*
****************************************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include "beep.h"
#include "sftmr.h"
#include "reg_gpio.h"
#include "reg_iopad.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define BIT_BEEP       (0x01UL << PIN_BEEP)

#define BEEP_ON()      GPIOA->DIRSET = BIT_BEEP
#define BEEP_OFF()     GPIOA->DIRCLR = BIT_BEEP

typedef struct
{
    uint8_t    tone;
    uint8_t    tidx;
} beep_t;


/* User customize */
// Tone[] = {cnt, tm_on, [tm_off, tm_on]*}
const uint8_t Tone_pwr[] = {1, _MS(50)};
const uint8_t Tone_btn[] = {1, _MS(20)};
const uint8_t Tone_ok[] = {1, _MS(100)};
const uint8_t Tone_fail[] = {3, _MS(50), _MS(50), _MS(50)};

const uint8_t *const TONE_TAB[] =
{
    Tone_pwr,
    Tone_btn,
    Tone_ok,
    Tone_fail,
};

/* end customize */


/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static beep_t gBeep;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static uint32_t beepTimer(uint8_t id)
{
    if (gBeep.tone == TONE_OFF)
        return 0;

    const uint8_t *ptone = TONE_TAB[gBeep.tone - 1];

    // finish to off
    if (gBeep.tidx >= ptone[0])
    {
        BEEP_OFF();
        gBeep.tone = TONE_OFF;
        return 0;
    }

    // update index, set state
    gBeep.tidx++;
    if (gBeep.tidx % 2)
        BEEP_ON();
    else
        BEEP_OFF();
    return ptone[gBeep.tidx];
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void beepTone(uint8_t tone)
{
    if ((tone == TONE_OFF) || (tone >= TONE_MAX))
    {
        BEEP_OFF();
        gBeep.tone = TONE_OFF;
        return;
    }

    if (gBeep.tone != TONE_OFF)
        return; // on busy

    const uint8_t *ptone = TONE_TAB[tone];

    if (sfTmrStart(ptone[1], beepTimer) != SFTMR_INVALID_ID)
    {
        gBeep.tone = tone;
        gBeep.tidx = 1;
        BEEP_ON();
    }
}

void beepInit(void)
{
    IOMCTL->PIOA[PIN_BEEP].Word  = 0;
    GPIOA->SET    = BIT_BEEP;
    GPIOA->DIRCLR = BIT_BEEP;
}
