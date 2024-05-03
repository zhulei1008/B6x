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
#include "cfg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// mode of Leds_Play *User should override it*
enum led_mode
{
	//demo
    LED_SLOW_BL,  // slow blink
    LED_FAST_BL,  // fast blink
    LED_BUSY_BL,  // busy blink, more fast
    LED_CONT_ON,  // continued ON
    LED_HINT_BL,  // twice blink as hint
    
    //user
	LED_BT_PAIRING,
	LED_BT_OFF,  
	LED_BT_CONNECT_BACK,	
	LED_CAP_ON,	
	LED_CAP_OFF,
	LED_BT_DELETE_PAIR,
	LED_BATTLOW_ALARM,
	LED_BT_CHANGE_CHANNLE,
	LED_BT_CHANGE_OS,	
	LED_BT_ON,
    LED_MODE_MAX
};
/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
#if (LED_PLAY)
/// Init iopad and env
void Leds_Init(void);

/// Dispaly 'mode' @see enum led_mode
void Leds_Play(uint8_t mode);

uint8_t read_led_currunt_mode(void);

/**
 ****************************************************************************************
 * @brief Show LED Lock of Keyboard Output, User Implement! (__weak func)
 *
 * @param[in] leds  Bits of Led_Lock(b0:num,b1:caps,b2:scroll)
 ****************************************************************************************
 */
void hids_led_lock(uint8_t leds);
#else
/// Disable via empty marco
#define Leds_Init()
#define Leds_Play(mode)

#endif


#endif // _LEDS_H_
