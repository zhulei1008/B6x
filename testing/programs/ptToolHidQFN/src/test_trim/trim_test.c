#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "trim_test.h"
//#include "ft_test.h"


#define LDO_TESTA 17

//MAX step: 8
//CMD_SYS_BDGP_FAL_ADJ(0x77) > 1.2V , CMD_SYS_BDGP_RIS_ADJ(0x76) < 1.2V
#define CORERUN_UPTRIM_MAX  0x20
#define CORERUN_LWTRIM_MIN  0x0

#define AONRUN_UPTRIM_MAX  0x10
#define AONRUN_LWTRIM_MIN  0x0

#define TRIM_VAILD              0xC8F5D5B6
#define TRIM_VAILD_LDO          0x0A

#define OTP_TRIM_OFFSET_ADDR         0x11E8     
//#define FLASH_TRIM_OFFSET_ADDR       0x18000FE4
//#define FLASH_TRIM_OFFSET_ADDR       0x00000FE0 // modify by door 2023.7.17, reason->coreldo voltage back to flash
//#define FLASH_TRIM_OFFSET_ADDR       0x00000FDC // modify by door 2023.7.17, reason->coreldo voltage back to flash
#define FLASH_TRIM_OFFSET_ADDR       0x00000F00 // modify by door 2023.7.17, reason->coreldo voltage back to flash
//#define FLASH_ER_OFFSET_ADDR         0x00000F00

//#define RAND_SEED_ADDR              0x20009000
#define CORELDO_VOLTAGE_ADDR        0x20009000

void CLK_EXTERMode(void)
{    
    // external mode clk configure
    AON->XOSC16M_CTRL.EN_LDO_SEL = 1; // EN_LDO_XOSC is controlled by EN_LDO_XOSC_REG
    AON->XOSC16M_CTRL.EN_LDO_XOSC_REG = 1; // EN_LDO_XOSC support power to DPLL  
    AON->PMU_CTRL.OSC_EN_RUN = 0;   
    APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 0;
    AON->BKHOLD_CTRL.XOSC_EST_MODE = 1; 
}

// bit[4] is retreated, initial value 0x0, actual value is 0x10
void ft_coreldo_init(void)
{
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 0;
    APBMISC->ANAMISC_CTRL.TESTA_CONN2XO_EN = 1; 
    APBMISC->ANAMISC_CTRL.BK_VBK_TEST_EN = 0;
    AON->PMU_ANA_CTRL.Word |= (1 << 20);     
    
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = 0;
    
    bootDelayMs(2);
}

// bit[3:2] is retreated, initial value 0x4, actual value is 0x8
void ft_aonldo_init(void)
{
    AON->PMU_ANA_CTRL.Word &=~ (1 << 20);    
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 0;
    APBMISC->ANAMISC_CTRL.TESTA_CONN2XO_EN = 1; 
    APBMISC->ANAMISC_CTRL.BK_VBK_TEST_EN = 1;
    
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = 0;
}

extern void sadc_calib(void);
uint16_t ft_rfbandgap_test(void)
{
    // coreldo replace aonldo
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 1;   
    
    uint16_t adc_data = 0, trim_val = 0x10;
    uint16_t max_rfbandgap_voleage = 0, min_rfbandgap_voleage = 0, rfbandgap_voleage = 0, coreldo_voltage = 0;

    // 1:VBG, 0: VDD
    SADC->SADC_ANA_CTRL.SADC_VREF_SEL = 1;
    // vref voltage :1.2v
    SADC->SADC_ANA_CTRL.SADC_VREF_TRIM = 0;

    coreldo_voltage = RD_32(CORELDO_VOLTAGE_ADDR);
#if DBG_MODE
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = 0x1B;
    coreldo_voltage = 1200;
#endif    
    max_rfbandgap_voleage = 1170 * 1024 / coreldo_voltage;
    min_rfbandgap_voleage = 1130 * 1024 / coreldo_voltage;
    rfbandgap_voleage = (max_rfbandgap_voleage + min_rfbandgap_voleage) >> 1;
    
    sadc_calib();

    sadc_conf(SADC_CR_DFLT);

    RF->RF_RSV |= 1 << 1; 
    
    for (uint8_t step = 0x10; step > 0; step >>= 1)
    {
        RF->ANA_TRIM.BG_RES_TRIM = trim_val;
        
        adc_data = sadc_read(15, 10);          
        
        if ((adc_data > min_rfbandgap_voleage) && (adc_data < max_rfbandgap_voleage))
            break;
        
        trim_val = trim_val + (step >> 1) - ((adc_data > rfbandgap_voleage) ? step : 0);
    }
    RF->RF_RSV &= ~(1UL << 1);
    
    return trim_val;
}

#include "proto.h"
uint16_t ft_coreldo_trim_adj(uint8_t cmd)
{
    static uint16_t upSysTrimRange = CORERUN_UPTRIM_MAX;
    static uint16_t dnSysTrimRange = CORERUN_LWTRIM_MIN;
	uint16_t trimValue =  AON->BKHOLD_CTRL.CORELDO_TRIM_RUN;
	
//    trimValue = (trimValue & 0x10) ? trimValue & 0x0f : trimValue | 0x10;
    trimValue ^= 0x10;

    if (cmd == CMD_CORELDO_RUN_RIS_ADJ)
    {   
        dnSysTrimRange =  trimValue;  
        trimValue = (dnSysTrimRange + upSysTrimRange) >> 1;   
    }
    else if (cmd == CMD_CORELDO_RUN_FAL_ADJ)// && (trimValue != 0))
    {  
        upSysTrimRange =  (trimValue == 0 ? upSysTrimRange : trimValue);
        trimValue = (dnSysTrimRange + upSysTrimRange) >> 1;   
    }	
    else if (cmd == CMD_CORELDO_RUN_ADD)
    {
        trimValue += 1;
    }
    // The highest bit displacement
//    trimValue = (trimValue & 0x10) ? trimValue & 0x0f : trimValue | 0x10;
    trimValue ^= 0x10;
    
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = trimValue;
    APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP = trimValue;
    // test cmd  

   return trimValue;  
}


uint16_t ft_coreldo_dp_trim_adj(uint8_t cmd)
{
	uint16_t trimValue =  AON->BKHOLD_CTRL.CORELDO_TRIM_RUN;
	
//    trimValue = (trimValue & 0x10) ? trimValue & 0x0f : trimValue | 0x10;    
    trimValue ^= 0x10;
    
    if (cmd == CMD_CORELDO_DP_REDUCE)
        trimValue -= 1;
    else if (cmd == CMD_CORELDO_DP_ADD)
        trimValue += 1;
    else if (cmd == CMD_CORELDO_RECOVER)
    {
        trimValue ^= 0x10;
        
        AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP;
        APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP = trimValue;
        // test cmd
        return trimValue;  
    }
    
//    trimValue = (trimValue & 0x10) ? trimValue & 0x0f : trimValue | 0x10;
    trimValue ^= 0x10;
    
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = trimValue;
    // test cmd  
    return trimValue;           
}

void ft_coreldo_record(uint16_t val)
{
//    uint8_t coreldo_val[2];
    // 
//    coreldo_val[0] = uart_getc(UART_PORT);
//    // 
//    coreldo_val[1] = uart_getc(UART_PORT);
    // coreldo write to memory
    WR_32(CORELDO_VOLTAGE_ADDR, val/*(coreldo_val[1] << 8) | coreldo_val[0]*/);
//    FT_RET(T_OK, CMD_CORELDO_RUN_RECORD, 0);
}

uint16_t ft_aonldo_trim_adj(uint8_t cmd)
{
	uint16_t trimValue = AON->BKHOLD_CTRL.AONLDO_TRIM_RUN;
    trimValue ^= 0xC;
    

    if (cmd == CMD_AONLDO_RUN_FAL_ADJ)
        trimValue -= 1;
    else if (cmd == CMD_AONLDO_RUN_RIS_ADJ)
        trimValue += 1;    
    else if (cmd == CMD_AONLDO_RUN_ADD)
        trimValue += 1;

    trimValue ^= 0xC;
    
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = trimValue;
    APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF = trimValue;
    // test cmd  
    return trimValue;       
}

uint16_t ft_aonldo_off_trim_adj(uint8_t cmd)
{
    uint16_t trimValue = AON->BKHOLD_CTRL.AONLDO_TRIM_RUN;

    trimValue ^= 0xC;    

    if (cmd == CMD_AONLDO_OFF_REDUCE)
        trimValue -= 1;
    else if (cmd == CMD_AONLDO_OFF_ADD)
        trimValue += 1;
    else if (cmd == CMD_AONLDO_RECOVER)
    {
        trimValue ^= 0xC;
        
        AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF;  
        APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF = trimValue;
        
        // test cmd  
        return trimValue;  
    }    
   
    trimValue ^= 0xC;    
    
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = trimValue;
    // test cmd  
    return trimValue;    
}


//void flash_wr_protect(uint8_t sta)
//{
//    // Ð´±£»¤0x000-0xFFF (4KB)
//    // write en singal
//    fshc_en_cmd(FSH_CMD_WR_EN);
//    // write state enable for write flash state
//    fshc_en_cmd(FSH_CMD_WR_STA_EN);
//    // send write sta cmd
//    fshc_wr_sta(FSH_CMD_WR_STA, 1, sta);
//}

#define SADC_AFLG_WAIT()   dowl( while (!(SADC->STCTRL.SADC_AUX_FLG)); )

//uint32_t sadc_rand_num(void)
//{
//    uint32_t random_num = 0;
//    uint32_t sadc_bak[5] = {0};
//    
//    if (SADC->SADC_ANA_CTRL.Word & SADC_EN_BIT)
//    {
//        sadc_bak[0]  = SADC->SADC_ANA_CTRL.Word;
//        sadc_bak[1]  = SADC->CTRL.Word;
//        sadc_bak[2]  = SADC->SADC_ANA_CTRL.Word;
//        sadc_bak[3]  = SADC->AUTO_SW_CTRL.Word;
//        sadc_bak[4]  = SADC->CH_CTRL.Word;
//    }
//    
//    // .EN_BG
//    RF->ANA_EN_CTRL.Word     = 0x00070000;
//    // bit0:rf_temp
//    RF->RF_RSV               = 0x0000B801;
//    
//    SADC->SADC_ANA_CTRL.Word = 0x0011B6D9;
//    SADC->CTRL.Word          = 0x09FB0010;
//    SADC->CH_CTRL.Word       = 0x0F;
//    
//    for (uint8_t cnt = 0; cnt < 32; cnt++)
//    {
//        SADC->CTRL.SADC_AUX_CLK_DIV = cnt;
//        
//        SADC->SADC_ANA_CTRL.Word    = ((cnt & 0x07) << SADC_IBSEL_CMP_LSB) | ((cnt & 0x07) << SADC_IBSEL_VCM_LSB)
//                                      | ((cnt & 0x07) << SADC_IBSEL_VREF_LSB) | ((cnt & 0x07) << SADC_IBSEL_BUF_LSB)
//                                      | ((cnt & 0x03) << SADC_CALCAP_LSB) | SADC_INBUF_BYPSS_BIT | SADC_VREF_VBG | SADC_EN_BIT;
//        // start conversion
//        SADC->CTRL.SADC_SOC         = 1;
//        
//        SADC_AFLG_WAIT();
//        
//        // clear flag
//        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
//        
//        random_num |= ((((SADC->AUX_ST.Word) & 0x01)) << (cnt));
//    }
//    
//    RF->RF_RSV             = 0x0000B800;

//    if (sadc_bak[0] & SADC_EN_BIT)
//    {
//        SADC->SADC_ANA_CTRL.Word = sadc_bak[2];
//        SADC->AUTO_SW_CTRL.Word  = sadc_bak[3];
//        SADC->CH_CTRL.Word       = sadc_bak[4];
//        SADC->SADC_ANA_CTRL.Word = sadc_bak[0];
//        SADC->CTRL.Word          = sadc_bak[1];
//        
//        sadc_calib();
//    }
//    
//    return random_num;
//}

#include "..\src\trim.h"
bool ft_trim_download_test(void)
{    
    uint32_t readBuff[TRIM_VAL_NUM] = {0};
    uint32_t i, values[64] = {[0 ... 63] = 0xFFFFFFFF}; // 
    TRIM_Typedef *trim = (TRIM_Typedef *)(values+1); 
    
    values[0] = RD_32(CORELDO_VOLTAGE_ADDR);
    // LDO_TESTA disable
    APBMISC->ANAMISC_CTRL.BK_VBK_TEST_EN = 0;        
//    CSC->CSC_PIO[LDO_TESTA].Word = 0;
	AON->BKHOLD_CTRL.PIOA19_FUNC_SEL = 1;    
    
    // VAL04    
	//trim->VAL04.BK_BOD_TRIM         = AON->PMU_ANA_CTRL.BK_BOD_TRIM;
    //trim->VAL04.LDO_LVD_SEL         = AON->PMU_ANA_CTRL.LDO_LVD_SEL;
    trim->VAL04.CORELDO_TRIM_RUN    = AON->BKHOLD_CTRL.CORELDO_TRIM_RUN;
    trim->VAL04.CORELDO_TRIM_DP     = APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP;
    trim->VAL04.AONLDO_TRIM_RUN     = AON->BKHOLD_CTRL.AONLDO_TRIM_RUN;
    trim->VAL04.AONLDO_TRIM_OFF     = APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF;
    trim->VAL04.VOLTAGE_TRIM_VALID  = TRIM_VAILD_LDO;
    
    // VAL03
//	trim->VAL03.RC16M_FREQ_TRIM     = APBMISC->RC16M_FREQ_TRIM;
//    uint16_t rc16m_freq_MHz         = APBMISC->RC16M_CNT_CTRL.RC16M_WIN_CNT/1000;
//    uint16_t rc16m_freq_KHz         = (APBMISC->RC16M_CNT_CTRL.RC16M_WIN_CNT%1000) / 10;
//    trim->VAL03.RC16M_FREQ_VALUE    = (rc16m_freq_MHz << 7) | rc16m_freq_KHz;
//	trim->VAL03.RC16M_FREQ_TRIM_VAILD = TRIM_VAILD_LDO;

//    trim->VAL03.LDO_XOSC_TR         = APBMISC->XOSC16M_CTRL.LDO_XOSC_TR;
//    trim->VAL03.LDO_XOSC_TR_VAILD   = TRIM_VAILD_LDO;
    
    // VAL02
	//trim->VAL02.MIC_TRIM = SADC->MIC_CTRL.MIC_TRIM;
    trim->VAL02.BG_RES_TRIM = RF->ANA_TRIM.BG_RES_TRIM;

    // VAL01
    // Add. 2023.08.25 --- wq.
    trim->VAL01.RANDOM_SEED = sadc_rand_num();//RD_32(RAND_SEED_ADDR);
    
    // VAL00
//    trim->VAL00.XOSC16M_CAP_TRIM = APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR;
//    trim->VAL00.XOSC16M_CAP_TRIM_VAILD = TRIM_VAILD_LDO;

    // VAL05
    trim->VAL05.TRIM_VALID_FLAG = 0xC8F5D5B6;

//    flash_wr_protect(0x00);
	//flashErase(0x18000f00, PAGE_ERASE_MODE, 0);
    //fshc_erase(FLASH_ER_ADDR, );
    fshc_erase(FLASH_TRIM_OFFSET_ADDR, FSH_CMD_ER_PAGE);

	//flashWriteData(0x18000FE4, TRIM_VAL_NUM, values, SPI_WRITE_MODE, 0);
    fshc_write(FLASH_TRIM_OFFSET_ADDR, values, 64/*TRIM_VAL_NUM*/, FSH_CMD_WR);

//    bootDelayMs(10);
    //flashReadData(0x18000FE4, TRIM_VAL_NUM, readBuff, 0);
    fshc_read(FLASH_TRIM_OFFSET_ADDR, readBuff, TRIM_VAL_NUM, FSH_CMD_RD);

//    flash_wr_protect(0x64);

#if (DBG_MODE)
    for (i = 0; i < TRIM_VAL_NUM; i++)
    {
        uart_putc(0, readBuff[i] & 0xff);
        uart_putc(0, (readBuff[i] >> 8) & 0xff);
        uart_putc(0, (readBuff[i] >> 16) & 0xff);
        uart_putc(0, (readBuff[i] >> 24) & 0xff);
    }
#endif
    
	for (i = 0; i < TRIM_VAL_NUM; i++)
	{
		if (values[i] != readBuff[i] )
		{
			//FT_RET(T_FAIL, CMD_TRIM_LOAD, 0);
			return false;
		}
	}
	
//    FT_RET(T_OK, CMD_TRIM_LOAD, 0);
    return true;    
}

uint32_t ldo_trim_test(uint8_t ft_cmd, uint16_t val)
{
    uint32_t result = 0;
    
    switch (ft_cmd)
    {
        case CMD_CORELDO_INIT:
        {
            ft_coreldo_init();        
        } break;		
		
        case CMD_CORELDO_RUN_RIS_ADJ:
        case CMD_CORELDO_RUN_FAL_ADJ:    
        case CMD_CORELDO_RUN_ADD: 
        {
			result = ft_coreldo_trim_adj(ft_cmd);        
        } break;				
        
        case CMD_CORELDO_DP_ADD:
        case CMD_CORELDO_DP_REDUCE:    
        case CMD_CORELDO_RECOVER:
        {
            result = ft_coreldo_dp_trim_adj(ft_cmd);        
        } break;		

        case CMD_CORELDO_RUN_RECORD: 
        {
            ft_coreldo_record(val);        
        } break;

        case CMD_AONLDO_INIT:
        {
            ft_aonldo_init();        
        } break;		
		
        case CMD_AONLDO_RUN_RIS_ADJ:
		case CMD_AONLDO_RUN_FAL_ADJ:
        {
            result = ft_aonldo_trim_adj(ft_cmd);        
        } break;
        
        case CMD_AONLDO_OFF_ADD:
        case CMD_AONLDO_OFF_REDUCE:
        case CMD_AONLDO_RECOVER: 
        {
            result = ft_aonldo_off_trim_adj(ft_cmd);        
        } break;

        case CMD_RFBANDGAP_TEST:
        {
            ft_rfbandgap_test();        
        } break;
        
        case CMD_TRIM_LOAD:
        {
            ft_trim_download_test();        
        } break;        
    }
    
    return result;
}
