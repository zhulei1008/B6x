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
#include "drvs.h"
#include "regs.h"
#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

static void sysInit(void)
{    
    #if (FPGA_TEST)
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 2; // BB Must 16M
    #else
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 0; // BB Must 16M
    #endif
}

static void iwdtInit(void)
{
    IWDT->LOCK       = 0x1ACCE551;  // Unlock IWDT
    IWDT->CTRL.INTEN = 1;
    IWDT->CTRL.EN    = 1;

    NVIC_EnableIRQ(IWDT_IRQn);
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    iwdtInit();
}

#define TOG_IO1     BIT(6)
#define TOG_IO2     BIT(7)
#define TOG_IO_MASK (TOG_IO1 | TOG_IO2)
int main(void)
{
    sysInit();
    devInit();
    
//    iom_ctrl(6, IOM_DRV_LVL1 | IOM_PULLUP | IOM_SEL_GPIO);
    iom_ctrl(7, IOM_DRV_LVL1 | IOM_PULLUP | IOM_SEL_GPIO);
    
//    GPIO->DAT_CLR = TOG_IO1;
    GPIO->DAT_SET = TOG_IO2;
    GPIO->DIR_SET = TOG_IO_MASK;
    
    GLOBAL_INT_START();
    
    while(1)
    {
//        GPIO->DAT_TOG =  TOG_IO1;
    }
}

void IWDT_IRQHandler(void)
{
    IWDT->LOCK    = 0x1ACCE551;
    IWDT->INTCLR  = 1;
    IWDT->LOAD    = 0x2000;
    IWDT->LOCK    = 0;
    
    GPIO->DAT_SET = TOG_IO2;    
    GPIO->DAT_CLR = TOG_IO2;
}
