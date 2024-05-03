/**
 ****************************************************************************************
 *
 * @file leds.h
 *
 * @brief Header file - Separate LED Display Module
 *
 ****************************************************************************************
 */

#ifndef _LEDS_H_
#define _LEDS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// mode of leds_play *User should override it*
enum led_mode
{
	LED_READY,
	LED_ERROR,    
    LED_BUSY,
	LED_OK,
	LED_FAIL,
    LED_OFF,
    
    // add more...
    
    LED_MODE_MAX
};


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

#if (LED_PLAY)
/// Init iopad and env
void leds_init(void);

/// Dispaly 'mode' @see enum led_mode
void leds_play(uint8_t mode);
#else
/// Disable via empty marco
#define leds_init()
#define leds_play(mode)
#endif


#endif // _LEDS_H_
