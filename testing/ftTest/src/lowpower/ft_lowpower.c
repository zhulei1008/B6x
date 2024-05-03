#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_lowpower.h"
#include "ft_test.h"


#define WKUP_IO 14


void pmuClkPowerSet(uint8_t ldo_dp, uint8_t aon_pwr_dp, uint8_t osc_dp, uint8_t hsi_dp, uint8_t clk_stb_time)
{
    uint32_t tmp_word, aon_pmu_ctrl;
    
    aon_pmu_ctrl = APBMISC->AON_PMU_CTRL.Word;
    aon_pmu_ctrl &= 0xFFC0F0FF;
    tmp_word = (ldo_dp << APBMISC_LDO_LP_SEL_DP_POS | aon_pwr_dp << APBMISC_AON_PWR_SEL_DP_POS | \
                osc_dp << APBMISC_OSC_EN_DP_POS | hsi_dp << APBMISC_HSI_EN_DP_POS | \
                clk_stb_time << APBMISC_CLK_STB_TIME_LSB);
    APBMISC->AON_PMU_CTRL.Word = tmp_word | aon_pmu_ctrl;
}


void ioWakeupSet(uint32_t padWord, uint8_t trig_edge)
{
    uint32_t tmp;
    
    for (uint8_t i = 0; i < 20; i++)
    {
        if ((padWord >> i) & 0x01)
        {
            CSC->CSC_PIO[i].IE = 1;
            CSC->CSC_PIO[i].FSEL = IOM_SEL_GPIO;
            CSC->CSC_PIO[i].PUPDCTRL = trig_edge;
            GPIO->DIR_CLR = 1 << i;
        }
    }
    if (trig_edge == IOM_PULLUP)
    {
        APBMISC->PIOA_WKUP_POS = padWord;
        tmp = APBMISC->PIOA_WKUP_NEG;
        APBMISC->PIOA_WKUP_NEG = tmp & (~padWord);
    }
    else if (trig_edge == IOM_PULLDOWN)
    {
        APBMISC->PIOA_WKUP_NEG = padWord;
        tmp = APBMISC->PIOA_WKUP_POS;
        APBMISC->PIOA_WKUP_POS = tmp & (~padWord);
    }
    
    AON->PMU_WKUP_CTRL.IO_WKUP_EN = 1;
}




void ioState(void)
{
	for (uint16_t i = 0; i < 20; i++)
    {
        if  (i == 14)
            continue;
        iom_ctrl(i, IOM_SEL_GPIO | IOM_PULLDOWN);
    }
}


void AON_PMU_IRQHandler(void)
{
    APBMISC->AON_PMU_CTRL.AON_PMU_INT_CLR = 1;
    APBMISC->AON_PMU_CTRL.WKUP_ST_CLR = 1;
}


extern void uartInit(void);
void ft_deepsleep_test(void)
{
    RCC->AHBRST_CTRL.ADC_RSTREQ = 1;
    RCC->AHBRST_CTRL.ADC_RSTREQ = 0;
    
	__enable_irq();
    NVIC_EnableIRQ(AON_PMU_IRQn);	

	// rf power disable
    ioState();   
    
    pmuClkPowerSet(1, 1, 0, 0, 0x1f);

    RCC->CLK_EN_ST.IWDT_FCLK_DIS_DP = 1;
    APBMISC->DPLL_CTRL.DPLL2_EN = 0;
    
    RF->ANA_EN_CTRL.EN_BG = 0;
    RF->ANA_PWR_CTRL.CLK_EN_PLL = 0;
    
    ioWakeupSet(1 << WKUP_IO, 2);
    
//	flashDeInit();

    SCB->SCR |= 1 << 2 ;
    __WFI ();

    RF->ANA_EN_CTRL.EN_BG = 1;
    RF->ANA_PWR_CTRL.CLK_EN_PLL = 1;               
    uartInit();
    uart_putc(UART_PORT, 0x88);
}


void ft_poweroff_test(void)
{

}
