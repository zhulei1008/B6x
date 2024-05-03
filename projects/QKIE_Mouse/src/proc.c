/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "mouse.h"
#include "uartRb.h"
#include "prf_bass.h"
#include "hid_desc.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

uint16_t core_sleep_rc16m(uint16_t wkup_en);
/*
 * DEFINES
 ****************************************************************************************
 */

#if (CFG_SLEEP)
void timer_init(void)
{
#if (0)
    RCC->APBCLK_EN_RUN._CLKEN_RUN = 0;
    RCC->APBRST_CTRL._RSTREQ      = 1;
    RCC->APBRST_CTRL._RSTREQ      = 0;
    RCC->APBCLK_EN_RUN._CLKEN_RUN = 1;

    // 10us cnt plus 1, cnt = 700 reload
    ->PSC     = 159; // 10us
    ->ARR     = 699;
    ->CNT     = 0;   // the counter start from 0
    ->CR1.OPM = 1;   // enable one-mode
    ->EGR.UG  = 1;   // auto update arr and init counter, psc counter is cleared
                          //    ->CR1.CEN  = 1;
    ->ICR.UI = 1;    // clear interrupt of update
#endif
#if (1)
    RCC->APBCLK_EN_RUN.ATMR_CLKEN_RUN = 0;
    RCC->APBRST_CTRL.ATMR_RSTREQ      = 1;
    RCC->APBRST_CTRL.ATMR_RSTREQ      = 0;
    RCC->APBCLK_EN_RUN.ATMR_CLKEN_RUN = 1;

    // 10us cnt plus 1, cnt = 700 reload
    ATMR->PSC     = 159; // 10us
    ATMR->ARR     = 699;
    ATMR->CR1.OPM = 1;   // enable one-mode
    ATMR->EGR.UG  = 1;   // auto update arr and init counter, psc counter is cleared

    ATMR->ICR.UI  = 1;    // clear interrupt of update
    ATMR->CNT     = 0;   // the counter start from 0
    ATMR->CR1.CEN = 1;
#endif
}

__SRAMFN static void sleep_proc(void)
{
    uint32_t slpdur = ble_slpdur_get();

    // sleep > 100ms
//    if (slpdur > 3200)
//    {
//        // Core enter poweroff mode120
//        if (ble_sleep(360, slpdur) == BLE_IN_SLEEP)
//        {
//            if(mouse_proc())
//            {
//              //pwroff_io_sw(PAD_KSCAN_MSK, PAD_KSCAN_MSK);
//                core_pwroff(CFG_WKUP_BLE_EN | WKUP_IO_LATCH_N_BIT);
//            }
//            else
//            {
//                mouse_data_handle();
//            }
//        }
//    }
    // sleep > 6.25ms
//    else 
        if (slpdur > 200)
    {
        APBMISC->BLE_DP_TWCORE = 66;
        APBMISC->AON_PMU_CTRL.CLK_STB_TIME = 31;
        
        // Core enter deepsleep mode
        if (ble_sleep(64, slpdur) == BLE_IN_SLEEP)
        {
            core_sleep(CFG_WKUP_BLE_EN);
            #if (CFG_FLASH_DXIP)
            puya_enter_dual_read();
            #endif
            
            #if (CFG_SENSOR)
            mouse_proc();
            mouse_data_handle();
            #endif
        }
    }
    // sleep > 2.5ms
    else if (slpdur > 80)
    {
        APBMISC->BLE_DP_TWCORE = 14;
        APBMISC->AON_PMU_CTRL.CLK_STB_TIME = 1;
        
        // Core enter deepsleep mode
        if (ble_sleep(12, slpdur) == BLE_IN_SLEEP)
        {
            core_sleep_rc16m(CFG_WKUP_BLE_EN);
            puya_enter_dual_read();
            
            //GPIO_DAT_SET(GPIO06);
            #if (CFG_SENSOR)
            mouse_proc();
            mouse_data_handle();
            #endif
            //GPIO_DAT_CLR(GPIO06);
        }
    }
}
#endif //(CFG_SLEEP)

__SRAMFN void user_procedure(void)
{
#if (CFG_SLEEP)
    sleep_proc();
    
    #if (TIMER_CORR)
    if (ATMR->RIF.UI) // time ³¬Ê±
    {
        DEBUG("timeout\r\n");
        ATMR->ICR.UI   = 1;
        ATMR->CNT      = 0;
        ATMR->CR1.CEN  = 1;
        
        #if (CFG_SENSOR)
        mouse_proc();
        mouse_data_handle();
        #endif
    }
    #endif //TIMER_CORR
#endif //CFG_SLEEP
}
