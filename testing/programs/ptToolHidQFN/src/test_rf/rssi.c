#include "hyreg.h"
#include "rcc.h"
#include "rssi.h"

static void rssi_adc_calibration(void)
{
    // enable ADC
//    SADC->SADC_ANA_CTRL.SADC_EN   = 1;
    
    // clear flag
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    // enable calibration mode
    SADC->CTRL.SADC_CALIB_MODE    = 1;
    
    // start calibration
    SADC->CTRL.SADC_SOC           = 1;
    
    // wait calibration done is 1
    while (! SADC->STCTRL.SADC_AUX_FLG);
    
    // clear flag
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    for (uint8_t i = 0; i < 8; i++)
    {
        SADC->SADC_CALIB_DATSEL = i;
    }
    
    // disable calibration mode
    SADC->CTRL.SADC_CALIB_MODE    = 0;
}

void rssi_sadc_init(void)
{
    uint32_t clk_div = rccGetSysClk()/16000000UL;
    
    __NOP();__NOP();
    __NOP();__NOP();

    // Clock
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 0;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 1;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 0;
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 1;
    
    // enable ADC
    SADC->SADC_ANA_CTRL.SADC_EN          = 1;
    
    SADC->CTRL.SADC_AUX_CLK_DIV          = (clk_div - 1); // must 16M.
    
    SADC->SADC_ANA_CTRL.SADC_IBSEL_CMP   = 3;
    SADC->SADC_ANA_CTRL.SADC_IBSEL_VCM   = 3;
    SADC->SADC_ANA_CTRL.SADC_IBSEL_VREF  = 3;
    SADC->SADC_ANA_CTRL.SADC_IBSEL_BUF   = 3;
    SADC->SADC_ANA_CTRL.SADC_VREF_SEL    = 0;
    SADC->SADC_ANA_CTRL.SADC_CALCAP_SEL  = 1;
    SADC->SADC_ANA_CTRL.SADC_INBUF_BYPSS = 1;
    
    SADC->CTRL.SADC_CLK_PH               = 1; // conversion at posedge of clk
    
    rssi_adc_calibration();

    SADC->CTRL.SADC_CONV_MODE            = 0; // single sample
    SADC->CTRL.SADC_SAMP_MOD             = 1; // RSSI sample mode
    SADC->CTRL.SADC_RSSI_SAMP_DLY        = 0;

    SADC->CH_CTRL.SADC_CH_SET0           = 15; // RF
    
    RF->RF_RSV &= ~(0x0F << 0);
    RF->RF_RSV |= (0x01 << 2); // RSSI
}

uint8_t rssi_get(void)
{
    RF->RSSI_CTRL.SW_RSSI_REQ = 1;
    // wait rssi_ready is 1
    while (! RF->RSSI_CTRL.SW_RSSI_READY);
    RF->RSSI_CTRL.SW_RSSI_REQ = 0;
    
//    RF->RSSI_CTRL.AGC_LNA_GAIN
//    RF->RSSI_CTRL.AGC_BPF_GAIN_ADJ
//    RF->RSSI_CTRL.AGC_MIXL_GAIN_CTL
//    RF->RSSI_CTRL.AGC_MIXH_GAIN_CTL

    return RF->RSSI_CTRL.RSSI_VAL;
}
