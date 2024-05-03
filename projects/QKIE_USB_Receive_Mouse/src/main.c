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

    rcc_fshclk_set(FSH_CLK_DPSC42);

    puya_enter_dual_read();

    // 2414.000MHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1C;

//    // sop8-3,
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1A;

//    // shenzhen,shao XOSC, 2478MHz, +14KHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x10;

    // QKIE, SOP8-1, 2440MHz, +4KHz
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x10;
    
    // SOP8-VDD12, 2440MHz, +2KHz
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x15;

    // Dongle,QFN20, Rx-2144MHz, +1KHz
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x22;
}
uint16_t ft_rfbandgap_test(void)
{
    // coreldo replace aonldo
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 1;   
    
    uint16_t adc_data = 0, trim_val = 0x10;
    uint16_t max_rfbandgap_voleage = 0, min_rfbandgap_voleage = 0, rfbandgap_voleage = 0, coreldo_voltage = 0;

//    coreldo_voltage = RD_32(CORELDO_VOLTAGE_ADDR);
    coreldo_voltage = 1194;//1194;//get_trim_vdd12_voltage();//1200;
  
    max_rfbandgap_voleage = 1170 * 1024 / coreldo_voltage;
    min_rfbandgap_voleage = 1130 * 1024 / coreldo_voltage;
    rfbandgap_voleage = (max_rfbandgap_voleage + min_rfbandgap_voleage) >> 1;
    
//    sadc_calib();
    sadc_init(SADC_ANA_VREF_1V2);
    sadc_conf(SADC_CR_DFLT);

    RF->RF_RSV |= 1 << 1; 
    
    for (uint8_t step = 0x10; step > 0; step >>= 1)
    {
        RF->ANA_TRIM.BG_RES_TRIM = trim_val;
        
        adc_data = sadc_read(15, 10);          
        
        if ((adc_data > min_rfbandgap_voleage) && (adc_data < max_rfbandgap_voleage))
            break;
        
        trim_val = trim_val + (step >> 1) - ((adc_data > rfbandgap_voleage) ? step : 0);
          debug("step:%x\r\n",step);
    }
    RF->RF_RSV &= ~(1UL << 1);
   
    debug("trim_val:%x\r\n",trim_val);
    return trim_val;
}
static void devInit(void)
{
    uint16_t rsn = rstrsn();
    uint32_t store_mac = RD_32(FLASH_BASE + BT_MAC_STORE_OFFSET);
    
#if (DBG_MODE)
    dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);
    trace_init();
#endif
    #if(!FAST_DONGLE_MODE)
    if (store_mac != 0xFFFFFFFF)
    {
        write32p(ble_dev_addr.addr, store_mac);
        
        ble_dev_addr.addr[4]=0x20;
        ble_dev_addr.addr[5]=0xd2;
    }
    #endif
    debugHex(ble_dev_addr.addr,6);
    // Init BLE App
    app_init(rsn);
    //ft_rfbandgap_test();
    
    rf_pa_set(0x0C);
    
    #if (CFG_USB)
    bootDelayMs(4);
    usbdInit();
    #endif

//    sys_dbg_init();
//    mdm_dbg_sel(MDM_DBG1);
//    ble_dbg_sel(BLE_DBG_TXRX);
//    ioBleTxRx(2, 3);
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
        
        usbd_mouse_report();
    }
}
