#ifndef _FT_TEST_H_
#define _FT_TEST_H_

#include <stdint.h>


#define T_OK                    1
#define T_FAIL                  0


// FT result: byte0 - byte1 - byte2 - byte3
//            OK/FAIL CMD     VAL0    VAL1
#define FT_RET(ok,cmd,val)              ft_result(((uint32_t)val<<16) | ((uint32_t)cmd<<8) | ok)


#define LDO_TESTA 17


/***************CMD*******************/
enum ft_cmd
{
    // 0x01 - 0x28 For RF Tx Freq
    // 0x81 - 0xA8 For RF Rx Freq   
    CMD_RF_TX           ,
    CMD_RF_RX           ,
	/*configure reg, through ldo_testa measure coreldo and aonldo voleage*/
    CMD_CORELDO_INIT    		= 0x31, // test ok
    CMD_CORELDO_RUN_RIS_ADJ     = 0x32, // test ok
    CMD_CORELDO_RUN_FAL_ADJ     = 0x33, // test ok
    CMD_CORELDO_DP_ADJ          = 0x34, // test ok
    CMD_AONLDO_INIT             = 0x35, // test ok
    CMD_AONLDO_RUN_RIS_ADJ      = 0x36, // test ok
    CMD_AONLDO_RUN_FAL_ADJ      = 0x37, // test ok
    CMD_AONLDO_OFF_ADJ          = 0x38, // test ok   
	/**********************************************************************/
	/*use boot rom rc16mAdj function to self-calibrate*/
    CMD_RC16M_ADJ               = 0x39, // tesk ok
	/*put trim value write to flash*/
    CMD_TRIM_LOAD               = 0x3a, // tesk ok
    // gpio_test cmd
    /*INPUT TESET*/	
	CMD_GPIO_IN_OHEL      		= 0x3b,  // INPUT_ODD      // tesk ok
	CMD_GPIO_IN_OLEH      		= 0x3c,  // INPUT_EVEN     // tesk ok
    /*OUTPUT TESET*/			
	CMD_GPIO_OUT_OHEL     		= 0x3d,  // ODD_HIGH AND EVNE_LOW  // tesk ok
	CMD_GPIO_OUT_OLEH     		= 0x3e,  // ODD_LOW AND EVNE_HIGH  // tesk ok
    /*Pullup/Pulldown TESET*/	 
	CMD_GPIO_PULL_OPED    		= 0x3f,  // ODD_UP  AND EVNE_DOWN  // tesk ok
	CMD_GPIO_PULL_ODEP    		= 0x40,  // ODD_DOWN  AND EVNE_UP  // tesk ok
    /*ININT TESET*/			
//	CMD_GPIO_INL          		= 0x11,  // ININT LOW DRIVER
//	CMD_GPIO_INH          		= 0x12,  // ININT HIGH DRIVER    
				
    CMD_ADC_CHN_TEST      		= 0x43,
    CMD_ADC_MIC_TEST      		= 0x44,
    CMD_FLASH_TEST        		= 0x45, // tesk ok
    CMD_RC32K_FREQ_TEST   		= 0x46, // test ok
    CMD_DPLL_FREQ_TEST    		= 0x47, // test ok
    //CMD_USB_TEST          		= 0x18,
    CMD_DEEPSLEEP_CURRENT   	= 0x49, // tesk ok
    //CMD_POWEROFF_CURRENT    	= 0x4a,
    CMD_MEM_BIST          		= 0x4b,  // rom and sram  // tesk ok    
    
    TEST_CMD                    = 0xcc,
    
    CMD_RESULT              = 0xdd,  
    CMD_MAX                 = 0xFF,
};



void ft_proc(void);
void ft_result(uint32_t res);


#endif

