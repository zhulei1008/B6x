#include "pwr_test.h"
#include "rcc.h"
#include "adc.h"
#include "iopad.h"

#if (CFG_TEST)

// NEED ADD 10uF to ADC_PIN
void pwr_adcInit(void)
{
    // pin 18 configure to analog mode:VBG_1P2V
    //    ioAnaCtrl(PIN_VBG_1P2V, 1);
    ioAnaCtrl(ADC_CHANNEL8, 1);

    adc_param_t parm =
    {
        .clkDiv     = ADC_CLK_DIV64,
        .clkPhase   = ADC_CLK_CONV_AT_POSEDGE,
        .inputMode  = ADC_SINGLE_ENDED_INPUT,
        .vrefSel    = ADC_VREF_SEL_VDD,
        .capSel     = ADC_CAP_CAL_LVL1,
        .vcmSel     = ADC_COMMON_MODE_CUR_SEL3,
        .cmpSel     = ADC_COMP_CUR_SEL3,
        .convMode   = ADC_SINGLE_CONVER_MODE,
        .inbufByps  = ADC_INBUF_ON,
    };

    adcDeviceParametersSet(&parm);

    adcCtrl(1);
}

// V = (dataSum/0xFFF)*VD33, VD33 = .vrefSel
uint16_t pwr_adcGet(void)
{
    uint16_t dataSum = 0, adcData = 0;

    for (uint8_t i = 0; i < 0x10; i++)
    {
        adcData = adcRead(ADC_CHN_AIN8);
        dataSum += adcData;
    }

    dataSum >>= 4;

    return dataSum;
}

void pwr_enterMode(uint8_t mode)
{

}

#endif //(CFG_TEST)
