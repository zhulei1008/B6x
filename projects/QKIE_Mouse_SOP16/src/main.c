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
#include "user_api.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

uint16_t g_rst_rsn;
volatile uint32_t g_io_sta;
//poweroffÇ°:80286
//×ó¼ü»½ÐÑ:  00286
//ÖÐ¼ü»½ÐÑ:  80086
//ÓÒ¼ü»½ÐÑ:  80206
/*
 * FUNCTIONS
 ****************************************************************************************
 */
static void sysInit(void)
{
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
    
#if (CFG_FLASH_DXIP)
    puya_enter_dual_read();
#endif
    
#if (CFG_SOP16_VDD12)
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1B;
#else
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;
#endif
}
extern void EMI_Mode(void);

static void devInit(void)
{
    g_rst_rsn = rstrsn();
    
    g_io_sta = GPIO_PIN_GET();
    
    core_release_io_latch();
    
    // BTN_L/R/M
    g_io_sta = ((((g_io_sta >> PAD_KSCAN_R2) << 0) | ((g_io_sta >> PAD_KSCAN_R0)  << 1) | ((g_io_sta >> PAD_KSCAN_R1) << 2)) & 0x07) ^ ROW_PAD_MSK;
    
    #if (DBG_MODE)
    dbgInit();
    debug("Start(rsn:%X, %s, EM_END:%X)...\r\n", g_rst_rsn, __TIME__, ble_exch_size());
    ble_build_info();
    debug("0---io:%x, %x...\r\n", g_io_sta, GPIO_PIN_GET());
    #endif

    #if (CFG_SENSOR)
    #if(!CFG_QFN32)
    iospc_rstpin(true);
    iom_ctrl(PAD_SENSOR_CLK,IOM_SEL_GPIO);
	iom_ctrl(PAD_SENSOR_DIO,IOM_SEL_GPIO);
    iom_ctrl(CHANNEL_SLECT_PIN,IOM_SEL_GPIO);
    #endif
    mouse_io_init();
    #endif
   // wakeup_io_sw(PAD_WHEEL_MSK, PAD_WHEEL_MSK);
    if (!RSN_IS_BLE_WKUP(g_rst_rsn))
    {
        mouse_data_clear();
        #if (CFG_EMI_MODE)
        if((g_rst_rsn&RSN_POR12_BK_BIT)==RSN_POR12_BK_BIT)
        {
            debug("reset_poweron\r\n");
            EMI_Mode();
        }
        #endif
        #if (LED_PLAY)
        sftmr_init();
        leds_init();
        leds_play(LED_POWERON);
        #endif //(LED_PLAY)
       // batt_scan_init();
        read_bt_mac_ltk_info(g_rst_rsn);
        
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
    app_init(g_rst_rsn);
    
#if ((CFG_IO_OBSERVE) && !(DBG_MODE))
    GPIO->DIR_SET = GPIO06 | GPIO07;
    iom_ctrl(PA06, IOM_SEL_GPIO);
    iom_ctrl(PA07, IOM_SEL_GPIO);
#endif
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
