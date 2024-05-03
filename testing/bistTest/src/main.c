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

#define TOG_IO1     BIT(6)
#define TOG_IO2     BIT(7)
#define TOG_IO_MASK (TOG_IO1 | TOG_IO2)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{    
    // Todo config, if need
    
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
}

int main(void)
{
    sysInit();
    devInit();

    while(1)
    {
        uint8_t cmd = uart_getc(0);
        debug("cmd:%x, bist_st:%x, signature_rpt:%x\r\n", cmd, SYSCFG->BIST_STATUS, SYSCFG->ROM_SIGNATURE_RPT);
        switch (cmd)
        {
            case 0x66:
            {
                SYSCFG->ROM_SIGNATURE_TARGET = 0x31323334;//0x72AADA64;
                SYSCFG->BIST_MODE            = 0x01;
            } break;

            default:
            {
                 SYSCFG->BIST_STATUS_CLR = 1;
            } break;
        }
    }
}
