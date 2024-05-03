/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */
#include "drvs.h"
#include "bledef.h"
#include "mouse.h"
#include "user_api.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

#if (CFG_SLEEP)
void timer_init(void)
{
#if (TIMER_CORR)
    RCC->APBCLK_EN_RUN.ADTMR1_CLKEN_RUN = 0;
    RCC->APBRST_CTRL.ADTMR1_RSTREQ      = 1;
    RCC->APBRST_CTRL.ADTMR1_RSTREQ      = 0;
    RCC->APBCLK_EN_RUN.ADTMR1_CLKEN_RUN = 1;

    // 10us cnt plus 1, cnt = 700 reload
    ADTMR1->PSC     = 159; // 10us
    ADTMR1->ARR     = 699;
    ADTMR1->CR1.OPM = 1;   // enable one-mode
    ADTMR1->EGR.UG  = 1;   // auto update arr and init counter, psc counter is cleared

    ADTMR1->ICR.UI  = 1;    // clear interrupt of update
    ADTMR1->CNT     = 0;   // the counter start from 0
    ADTMR1->CR1.CEN = 1;
#endif
}
extern uint8_t mouse_scroll(void);
static uint32_t scroll_check_wait_cnt = 0;
__SRAMFN static void sleep_proc(void)
{
    uint32_t slpdur = ble_slpdur_get();
    if(mouse_scroll())
    {
        scroll_check_wait_cnt=1000;
        mouse_proc();
        mouse_data_handle();
        return;
    }
    if(scroll_check_wait_cnt>0)
    {
        scroll_check_wait_cnt--;
        return;
    }
    //DEBUG("00");
    //return;
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
            
            //#if (CFG_SENSOR)
            mouse_proc();
            mouse_data_handle();
            //#endif
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
           // #if (CFG_SENSOR)
            mouse_proc();
            mouse_data_handle();
            //#endif
            //GPIO_DAT_CLR(GPIO06);
        }
    }
}
#endif //(CFG_SLEEP)

__SRAMFN void user_procedure(void)
{
#if (CFG_SLEEP)
    sleep_proc();
    
     //mouse_proc();
     //mouse_data_handle();
    #if (TIMER_CORR)
    if (ADTMR1->RIF.UI) // time ³¬Ê±
    {
        DEBUG("timeout\r\n");
        ADTMR1->ICR.UI   = 1;
        ADTMR1->CNT      = 0;
        ADTMR1->CR1.CEN  = 1;
        
        #if (CFG_SENSOR)
        mouse_proc();
        mouse_data_handle();
        #endif
    }
    #endif //TIMER_CORR
#endif //CFG_SLEEP
}
