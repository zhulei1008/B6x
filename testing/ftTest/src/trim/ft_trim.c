#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_trim.h"
#include "ft_test.h"

//MAX step: 8
//CMD_SYS_BDGP_FAL_ADJ(0x77) > 1.2V , CMD_SYS_BDGP_RIS_ADJ(0x76) < 1.2V
#define CORERUN_UPTRIM_MAX  0x10
#define CORERUN_LWTRIM_MIN  0x0

#define AONRUN_UPTRIM_MAX  0x8
#define AONRUN_LWTRIM_MIN  0x0

#define TRIM_VAILD              0xC8F5D5B6
#define TRIM_VAILD_LDO          0x0A

#define OTP_TRIM_OFFSET_ADDR         0x11E8     
#define FLASH_TRIM_OFFSET_ADDR       0x00000FE4
#define FLASH_ER_OFFSET_ADDR         0x00000F00


// bit[4] is retreated, initial value 0x0, actual value is 0x10
void ft_coreldo_init(void)
{
	// configure to use ldo_testa measure coreldo 
	AON->PMU_ANA_CTRL.ANA_RESV = 1 << 4;
    APBMISC->ANAMISC_CTRL.TESTA_CONN2XO_EN = 0;
	iom_ctrl(LDO_TESTA, IOM_ANALOG);
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = 0;
}

// bit[3:2] is retreated, initial value 0x4, actual value is 0x8
void ft_aonldo_init(void)
{
	// configure to use ldo_testa measure aonldo 
	AON->PMU_ANA_CTRL.ANA_RESV &=~ (1 << 4);
	APBMISC->ANAMISC_CTRL.BK_VBK_TEST_EN = 1;
	//iom_ctrl(LDO_TESTA, IOM_ANALOG);  
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = 0x0;
}

void ft_coreldo_trim_adj(uint8_t cmd)
{
     static uint16_t upSysTrimRange = CORERUN_UPTRIM_MAX;
    static uint16_t dnSysTrimRange = CORERUN_LWTRIM_MIN;
	uint16_t trimValue =  AON->BKHOLD_CTRL.CORELDO_TRIM_RUN;
	
    trimValue = (trimValue & 0x10) ? trimValue & 0x0f : trimValue | 0x10;

    if (cmd == CMD_CORELDO_RUN_RIS_ADJ)
    {   
        //trimValue = (upSysTrimRange + trimValue) >> 1; 
        dnSysTrimRange =  trimValue;  
        trimValue = (dnSysTrimRange + upSysTrimRange) >> 1;   
    }
    else if (cmd == CMD_CORELDO_RUN_FAL_ADJ)// && (trimValue != 0))
    {  
        //upSysTrimRange = trimValue;
        //trimValue = (upSysTrimRange + dnSysTrimRange) >> 1;
        //dnSysTrimRange = trimValue;
        upSysTrimRange =  (trimValue == 0 ? upSysTrimRange : trimValue);
        trimValue = (dnSysTrimRange + upSysTrimRange) >> 1;   
    }	
    // The highest bit displacement
    trimValue = (trimValue & 0x10) ? trimValue & 0x0f : trimValue | 0x10;
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = trimValue;
    
    // test cmd
    uart_putc(UART_PORT, trimValue);    
}


void ft_aonldo_trim_adj(uint8_t cmd)
{
    static uint16_t upSysTrimRange = AONRUN_UPTRIM_MAX;
    static uint16_t dnSysTrimRange = AONRUN_LWTRIM_MIN;
	uint16_t trimValue = AON->BKHOLD_CTRL.AONLDO_TRIM_RUN;
	
    //trimValue = (trimValue & 0x0c) ? trimValue & 0x03 : trimValue | 0x0c;
    if ((trimValue & 0x8) && (trimValue & 0x4))
    {
        trimValue = trimValue & 0x03;
    }
    else if (trimValue & 0x8)
    {
        trimValue = ((trimValue & 0x7) | 0x4);
    }
    else if (trimValue & 0x04)
    {
        trimValue = ((trimValue & 0xb) | 0x8);
    }
    else
    {
        trimValue = trimValue | 0x0c;
    }
    

    if (cmd == CMD_AONLDO_RUN_RIS_ADJ)
    {   
        //trimValue = (upSysTrimRange + trimValue) >> 1; 
        dnSysTrimRange =  trimValue;  
        trimValue = (dnSysTrimRange + upSysTrimRange) >> 1;   
    }
    else if (cmd == CMD_AONLDO_RUN_FAL_ADJ)// && (trimValue != 0))
    {  
        //upSysTrimRange = trimValue;
        //trimValue = (upSysTrimRange + dnSysTrimRange) >> 1;
        //dnSysTrimRange = trimValue;
        upSysTrimRange =  (trimValue == 0 ? upSysTrimRange : trimValue);
        trimValue = (dnSysTrimRange + upSysTrimRange) >> 1;   
    }	
    // The highest bit displacement
    //trimValue = (trimValue & 0x0c) ? trimValue & 0x03 : trimValue | 0x0c;
    if ((trimValue & 0x8) && (trimValue & 0x4))
    {
        trimValue = trimValue & 0x03;
    }
    else if (trimValue & 0x8)
    {
        trimValue = ((trimValue & 0x7) | 0x4);
    }
    else if (trimValue & 0x04)
    {
        trimValue = ((trimValue & 0xb) | 0x8);
    }
    else
    {
        trimValue = trimValue | 0x0c;
    }    
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = trimValue;
    // test cmd
    uart_putc(UART_PORT, trimValue);   
}


void ft_trim_download_test(void)
{    
    uint32_t readBuff[TRIM_VAL_NUM] = {0};
    uint32_t i, values[TRIM_VAL_NUM]; // 
    TRIM_Typedef *trim = (TRIM_Typedef *)values; 
    
    // LDO_TESTA disable
    APBMISC->ANAMISC_CTRL.BK_VBK_TEST_EN = 0;       
    CSC->CSC_PIO[LDO_TESTA].Word = 0;
	AON->BKHOLD_CTRL.PIOA19_FUNC_SEL = 1;    
    
    // VAL04    
	trim->VAL04.BK_BOD_TRIM = AON->PMU_ANA_CTRL.BK_BOD_TRIM;
    trim->VAL04.LDO_LVD_SEL = AON->PMU_ANA_CTRL.LDO_LVD_SEL;
    trim->VAL04.CORELDO_TRIM_RUN = AON->BKHOLD_CTRL.CORELDO_TRIM_RUN;
    trim->VAL04.CORELDO_TRIM_DP = APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP;
    trim->VAL04.AONLDO_TRIM_RUN = AON->BKHOLD_CTRL.AONLDO_TRIM_RUN;
    trim->VAL04.AONLDO_TRIM_OFF = APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF;
    trim->VAL04.VOLTAGE_TRIM_VALID = TRIM_VAILD_LDO;
    
    // VAL03
	trim->VAL03.RC16M_FREQ_TRIM = APBMISC->RC16M_FREQ_TRIM;
	trim->VAL03.RC16M_FREQ_TRIM_VAILD = 0x0A;
    
    // VAL02
	//trim->VAL02.MIC_TRIM = SADC->MIC_CTRL.MIC_TRIM;
    // VAL01
    
    // VAL00
    
    // VAL05
    trim->VAL05.TRIM_VALID_FLAG = 0xC8F5D5B6;    
    
	//flashErase(0x18000f00, PAGE_ERASE_MODE, 0);
    //fshc_erase(FLASH_ER_ADDR, );
    fshc_erase(FLASH_ER_OFFSET_ADDR, FSH_CMD_ER_PAGE);

	//flashWriteData(0x18000FE4, TRIM_VAL_NUM, values, SPI_WRITE_MODE, 0);
    fshc_write(FLASH_TRIM_OFFSET_ADDR, values, TRIM_VAL_NUM, FSH_CMD_WR);

    //flashReadData(0x18000FE4, TRIM_VAL_NUM, readBuff, 0);
    fshc_read(FLASH_TRIM_OFFSET_ADDR, readBuff, TRIM_VAL_NUM, FSH_CMD_RD);

    uart_putc(0, readBuff[4] & 0xff);
    uart_putc(0, (readBuff[4] >> 8) & 0xff);
    uart_putc(0, (readBuff[4] >> 16) & 0xff);
    uart_putc(0, (readBuff[4] >> 24) & 0xff);

	for (i = 0; i < TRIM_VAL_NUM; i++)
	{
		if (values[i] != readBuff[i] )
		{
			FT_RET(T_FAIL, CMD_TRIM_LOAD, 0);
			return;
		}
	}
	
    FT_RET(T_OK, CMD_TRIM_LOAD, 0);     
}

