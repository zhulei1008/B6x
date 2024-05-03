/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */


/*
 * INCLUDES
 ****************************************************************************************
 */

//#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_test.h"

/*
 * DEFINES
 ****************************************************************************************
 */



void sysInit(void)
{
    // enable func clock
    RCC_AHBCLK_EN( AHB_SYSCFG_BIT | AHB_ADC_BIT | AHB_GPIOA_RST_BIT);
    RCC_APBCLK_EN( APB_MDM_BIT | APB_RF_BIT | APB_AON_BIT | APB_APBMISC_BIT | APB_UART1_BIT);    
}


void devInit(void)
{

}


void uartInit(void)
{    
    uart_init(UART_PORT, PIN_UART_TX, PIN_UART_RX);
    uart_conf(UART_PORT, BRR_921600, LCR_BITS_DFLT);
}

#define rom_flashInit ((void (*)(void))ROM_FLASHINIT)

int main(void)
{
    sysInit();
      
    rom_flashInit();
    
    uartInit();
	
    uart_putc(UART_PORT, 0x66);
    
    while(1)
    {
        ft_proc();
    }
}
