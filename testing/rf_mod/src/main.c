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
#include "regs.h"
#include "uart.h"
#include "uart_cmd.h"
#include "rf_test.h"
#include "iopad.h"
#include "uartRb.h"
#include "sysdbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */
void user_procedure(void);

static void sysInit(void)
{
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT | APB_UART1_BIT | APB_RF_BIT);
    RCC_AHBCLK_EN(AHB_SYSCFG_BIT);
    
    rcc_adc_en();
    
    iospc_rstpin(false);
    
    AON->PMU_ANA_CTRL.ANA_RESV |= 4;
    
    // QFN48-2, 16000007.96M
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x19;
    
    // QFN48-6, 16000003.96M
//    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x20;
}

static void devInit(void)
{
    #if (USE_UART1)
    uart1Rb_Init();
    #endif
    
    #if (USE_UART2)
    uart2Rb_Init();
    #endif
 
    rf_2g4_init();
        
    NVIC_EnableIRQ(UART1_IRQn);
    NVIC_EnableIRQ(BLE_IRQn);
    
    sys_dbg_init();
    rf_dbg_sel(RF_DBG_TX);
}

int main(void)
{
    sysInit();
    devInit();

    // Interrupt Enable
    GLOBAL_INT_START();
    
    while (1)
    {
        user_procedure();
    }
}
