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
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "batt.h"
#include "dbg.h"
#include "keyboard.h"
#include "user_api.h"
#include "fcc.h"
#include "chipset.h"
#include "proto.h"
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
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);

    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    core_release_io_latch();
    
    zi_data_clr();
    
    #if (DBG_MODE)
    dbgInit();
    #endif //(UART_CMD)
    debug("Start(rsn:%X)...\r\n", rsn);

    Sftmr_Init();
       
    iospc_rstpin(true);
    iom_ctrl(PA_SWCLK,IOM_SEL_GPIO);
    iom_ctrl(PA_SWDIO,IOM_SEL_GPIO);
    Keyboard_Scan_Init();

    #if (LED_PLAY)
    Leds_Init();
    #endif //(LED_PLAY)
    if((rsn&RSN_POR12_CORE_BIT)==RSN_POR12_CORE_BIT)
    {
        //bootDelayMs(5000);        
        debug("wekup_poweroff\r\n");
        AON->BACKUP1    = 3;
    }
    #if (CFG_EMI_MODE)
    else if((rsn&RSN_POR12_BK_BIT)==RSN_POR12_BK_BIT)
    {
        debug("reset_poweron\r\n");
        EMI_Mode();
    }
    #endif
    Batt_Init();
    
    //keys_init();

    PairKey_Init();
    
    #if (PRF_OTAS)
    chipInit();
    #endif
    // Init BLE App
    app_init(rsn);
    
    memcpy(masterDongle_Addr, def_MasterDongle_Addr, 6);
}

int main(void)
{
    sysInit();
    devInit();

    Read_Bt_Mac_Ltk_Info();
    
    // Global Interrupt Enable
    GLOBAL_INT_START();
    
    // main loop
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();

        // SoftTimer Polling
        sftmr_schedule();
        
        // User's Procedure
        user_procedure();
        proto_schedule();
    }
}
