/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "batt.h"
#include "mouse.h"
#include "uartRb.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */
extern void user_procedure(void);
extern void read_bt_mac_ltk_info(void);
extern void timer_init(void);
extern void mouse_data_clear(void);

static void sysInit(void)
{
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
    
#if (CFG_FLASH_DXIP)
    puya_enter_dual_read();
#endif
    
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x12;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    core_release_io_latch();
    
    #if (DBG_MODE)
    dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);
    #endif
    
    #if (CFG_SENSOR)
    mouse_io_init();
    #endif  
    if (!RSN_IS_BLE_WKUP(rsn))
    {
        mouse_data_clear();
        #if (LED_PLAY)
        sftmr_init();
        leds_init();
        leds_play(LED_POWERON);
        #endif //(LED_PLAY)
        read_bt_mac_ltk_info();
        gpio_set_hiz(CHANNEL_SLECT_PIN);
        #if (CFG_SENSOR)
        mouse_sensor_init();
        #endif
        batt_init();
    }
    else
    {
        #if (LED_PLAY)
        sftmr_init();
        #endif //(LED_PLAY)
    }

    #if (CFG_SLEEP && CFG_POWEROFF)
    #if (TIMER_CORR)
    timer_init();
    #endif
    #endif

    // Init BLE App
    app_init(rsn);
}

int main(void)
{
    sysInit();
    devInit();
    
    // Global Interrupt Enable
    GLOBAL_INT_START();
    
    // main loop
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();

        #if (LED_PLAY)
        // SoftTimer Polling
        sftmr_schedule();
        #endif //(LED_PLAY)
		
        // User's Procedure
        user_procedure();
    }
}
