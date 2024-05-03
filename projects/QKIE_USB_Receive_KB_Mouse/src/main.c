/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */
#include "regs.h"
#include "drvs.h"
#include "bledef.h"
#include "app.h"
#include "sysdbg.h"
#include "dbg.h"
#include "app_user.h"

/*
 * DEFINES
 ****************************************************************************************
 */
/*
 * FUNCTIONS
 ****************************************************************************************
 */
static void sysInit(void)
{
    rcc_sysclk_set(SYS_CLK);
    
    iwdt_disable();

    rcc_ble_en();
    
    rcc_adc_en();
    
    /***************************************/
    uint8_t fid = flashReadId() & 0xFF;

    if (fid == FSH_VID_BOYA)
    {
        rcc_fshclk_set(FSH_CLK_DPSC25);
        
        boya_flash_quad_mode();
        
        boya_enter_hpm();
    }
    else
    {
        rcc_fshclk_set(FSH_CLK_DPSC42);
        
        puya_enter_dual_read();        
    }
    /***************************************/
    
    // 2414.000MHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1C;
    
//    // sop8-3,
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1A;
    
//    // shenzhen,shao XOSC, 2478MHz, +14KHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x10;
    
    // QKIE, SOP8-1, 2440MHz, +4KHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x10;
    
    // SOP8-VDD12, 2440MHz, +2KHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x15;

    // Dongle,QFN20, Rx-2144MHz, +1KHz
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x22;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uint32_t store_mac = RD_32(FLASH_BASE + BT_MAC_STORE_OFFSET);

    if (store_mac != 0xFFFFFFFF)
    {
        write32p(ble_dev_addr.addr, store_mac);
    }

    #if (DBG_MODE)
    dbgInit();
    debug("----------Start(rsn:%X, clk:%d)...\r\n", rsn, rcc_sysclk_freq());
    #endif

    #if (CFG_HW_TIMER)
    sftmr_init();
    #endif

    app_init(rsn);
    rf_pa_set(0x0e);

    #if (CFG_USB)
    bootDelayMs(4);
    usbdInit();
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
        
        #if (CFG_USB)
        usbd_mouse_report();

        usbd_kb_report();
        
        usbd_consumer_report();
        #endif
        
        #if (CFG_HW_TIMER)
        sftmr_schedule();
        #endif
    }
}
