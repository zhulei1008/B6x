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
#include "sysdbg.h"
#include "dbg.h"
#include "rf_afe.h"
#include "uart_cmd.h"

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
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT | APB_UART1_BIT | APB_RF_BIT);
    RCC_AHBCLK_EN(AHB_SYSCFG_BIT);
    
    rcc_adc_en();
    
    iospc_rstpin(false);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    sysdbg_anatst(ATEN_AT0_AT1, 0);
    // AT1 PA19
//    iospc_rstpin(true);
    
    rf_debug_test_init();
}

int main(void)
{
    sysInit();
    devInit();
    
    userUartParse();
}
