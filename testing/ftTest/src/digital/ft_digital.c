#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_digital.h"
#include "ft_test.h"

#define ARR_SIZE            0x40       //TEST WRITE READ A PAGE SIZE
#define FLASH_TEST_ADDR_OFFSET         0x8000      //TEST WRITE READ OFFSET ADDRESS
#define FLASH_TEST_ADDR_OFFSET_1       0x10000      //TEST WRITE READ OFFSET ADDRESS

#define ADC_CHANNEL0  0
//1.206V  1.2/3.3*1024 = 372
#define ADC_UP_LIMIT_V12    380
#define ADC_DOWN_LIMIT_V12  360

#define ADC_MICBIAS   2
#define ADC_MICIN     3
void ft_adc_chn_test(void)
{
    uint16_t adc_data = 0;
	        
	GPIO_DIR_CLR(1 << ADC_CHANNEL0);
    
	iom_ctrl(ADC_CHANNEL0, IOM_ANALOG);	
	
    sadc_init();
    
    sadc_conf(SADC_CR_DFLT);
    
//    for (uint8_t i = 0; i < 10; i++)
//    {
    adc_data = sadc_read(ADC_CHANNEL0 + 5, 10);
//        adc_data_sum += adc_data;
//    }
//    adc_data = adc_data_sum / 10;
    // test cmd
	uart_putc(UART_PORT, adc_data);
    uart_putc(UART_PORT, adc_data >> 8);
    
	if ((adc_data > ADC_UP_LIMIT_V12) || (adc_data < ADC_DOWN_LIMIT_V12))
    {
		FT_RET(T_FAIL, CMD_ADC_CHN_TEST, 0);
        return ;
	}
    
	FT_RET(T_OK, CMD_ADC_CHN_TEST, 0);	
}


void ft_adc_mic_test(void)
{
	GPIO_DIR_CLR(1 << ADC_MICBIAS | 1 << ADC_MICIN);
	CSC->CSC_PIO[ADC_MICBIAS].Word = 0;   
	iom_ctrl(ADC_MICIN, IOM_ANALOG);
	
    sadc_init();
    
//    SADC->CTRL.SADC_SAMP_MOD    |= SADC_CR_SAMP_PCM;     
//    SADC->CTRL.Word |= SADC_CR_CLKPH_POSEDGE; 
//    SADC->CTRL.SADC_HPF_COEF = 3;    
//    SADC->CTRL.SADC_AUX_CLK_DIV = 0; 
//    SADC->CTRL.SADC_CONV_MODE = 0; 
    
    sadc_conf(SADC_CR_CLKPH_POSEDGE | SADC_CR_SAMP_PCM | (3 << SADC_CR_HPF_COEF_LSB) | \
                (0 << SADC_CR_CLK_DIV_LSB) | SADC_CR_CONV_CONTINUE);
    
    // bias voltage adjust
//    SADC->MIC_CTRL.MIC_MODE_SEL = 0;
//    SADC->MIC_CTRL.MIC_VREF_SEL = 1;
//    // mic power enable
//    SADC->MIC_CTRL.MIC_PD = 0;    
//    SADC->MIC_CTRL.PGA_BIAS_SEL = 1;
    // pga enable
//    SADC->MIC_CTRL.PGA_EN = 1;  
    SADC->MIC_CTRL.Word = SADC_MIC_DFLT;
    // gain 39dB
    //SADC->MIC_CTRL.PGA_VOL = 0;
   
    // adc sample channel select MICIN
    SADC->CH_CTRL.SADC_CH_SET0  = 4;
    
    SADC->DC_OFFSET = 0x200;
        
    // clear last flag.
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;

    // start conversion.   
    SADC->CTRL.SADC_SOC = 1;    

    for (uint16_t i = 0; i < 4; i++)
    {
        uart_putc(UART_PORT, (SADC->PCM_DAT) >> 8);
        uart_putc(UART_PORT, SADC->PCM_DAT);
    }
}




void ft_usb_test(void)
{

}

#define FLASH_WR_DATA 0x33a4d8

void ft_flash_test(void)
{	
	uint16_t i = 0;
    uint32_t wrBuff[ARR_SIZE] = {0};
    uint32_t rdBuff[ARR_SIZE] = {0};
	

	for (i = 0; i < ARR_SIZE; i++)
	{
		wrBuff[i] = FLASH_WR_DATA * i;	
	}	
	 // single test
    fshc_erase(FLASH_TEST_ADDR_OFFSET, FSH_CMD_ER_PAGE);

    fshc_read(FLASH_TEST_ADDR_OFFSET, rdBuff, ARR_SIZE, FSH_CMD_RD);    
	// compare
	for (i = 0; i < ARR_SIZE; i++)
	{
		if(0xFFFFFFFF != rdBuff[i])
        {
            break;
        }
	}    
    
    fshc_write(FLASH_TEST_ADDR_OFFSET, wrBuff, ARR_SIZE, FSH_CMD_WR);

    fshc_read(FLASH_TEST_ADDR_OFFSET, rdBuff, ARR_SIZE, FSH_CMD_RD);  
	// compare
	for (i = 0; i < ARR_SIZE; i++)
	{
		if(wrBuff[i] != rdBuff[i])
        {
            break;
        }
	}	
	// single mode erase, write, read flash
    fshc_erase(FLASH_TEST_ADDR_OFFSET, FSH_CMD_ER_PAGE);

    
    // double test
    fshc_erase(FLASH_TEST_ADDR_OFFSET_1, FSH_CMD_ER_PAGE);

    fshc_read(FLASH_TEST_ADDR_OFFSET_1, rdBuff, ARR_SIZE, FSH_CMD_DLRD);    
	// compare
	for (i = 0; i < ARR_SIZE; i++)
	{
		if(0xFFFFFFFF != rdBuff[i])
        {
            break;
        }
	}    
    
    fshc_write(FLASH_TEST_ADDR_OFFSET_1, wrBuff, ARR_SIZE, FSH_CMD_DLWR);

    fshc_read(FLASH_TEST_ADDR_OFFSET_1, rdBuff, ARR_SIZE, FSH_CMD_DLRD);  
	// compare
	for (i = 0; i < ARR_SIZE; i++)
	{
		if(wrBuff[i] != rdBuff[i])
        {
            break;
        }
	}	
	// double mode erase, write, read flash
    fshc_erase(FLASH_TEST_ADDR_OFFSET_1, FSH_CMD_ER_PAGE);
	
	FT_RET( (i == ARR_SIZE ? T_OK : T_FAIL), CMD_FLASH_TEST, 0);
}

