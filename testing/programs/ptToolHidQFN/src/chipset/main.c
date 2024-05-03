#include "b6x.h"
#include "drvs.h"
#include "regs.h"


#include "sftmr.h"
#include "proto.h"
#include "chipset.h"

extern void rf_reg_init(void);
extern void CLK_EXTERMode(void);

static void sysInit(void)
{
    #if (SOP)
    RCC->CFG.SYSCLK_SW = 1; 
    CLK_EXTERMode();
    RCC->CFG.SYSCLK_SW = 2; 
    #endif    
    
    rcc_fshclk_set(FSH_CLK_DPSC42);

//    CACHE->CCR.Word = 0;
//    CACHE->CIR.INV_ALL = 1;
//    puya_enter_dual_read();
//    fshc_quad_mode(1UL << 9);   
//    puya_enter_hpm();
    
    iwdt_disable();
    
    // enable func clock
    RCC_AHBCLK_EN( AHB_SYSCFG_BIT | AHB_ADC_BIT | AHB_GPIOA_RST_BIT);
    RCC_APBCLK_EN( APB_MDM_BIT | APB_RF_BIT | APB_AON_BIT | APB_APBMISC_BIT | APB_UART1_BIT);    
    rcc_ble_en();
    rcc_adc_en();
    
    // rf reg init
    rf_reg_init(); 
    
    // xo16m cap trim
    // .XOSC16M_LP = 0
    APBMISC->XOSC16M_CTRL.Word = (0x06 << APBMISC_XOSC16M_CAP_TR_LSB) | 0x00014880;
    
    #if (UART1_BAUD_2M)
    rcc_sysclk_set(SYS_CLK_64M);
    #endif
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    sftmr_init();

    chipInit();
    
//    trace_init();
    GPIO_DAT_CLR(GPIO13);
    GPIO_DIR_SET(GPIO13);
}

int main(void)
{
    sysInit();

    devInit();

    __enable_irq();
    
    while (1)
    {
        proto_schedule();
    }
}
