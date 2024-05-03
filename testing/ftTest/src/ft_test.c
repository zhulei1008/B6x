#include "ft_test.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "rf_test.h"
#include "ft_gpio.h"
#include "ft_trim.h"
#include "ft_lowpower.h"
#include "ft_clock.h"
#include "ft_bist.h"
#include "ft_digital.h"

volatile uint32_t gFtResult = 0;


static void ft_report_result(void)
{
    uart_putc(UART_PORT, gFtResult & 0xFF);
    uart_putc(UART_PORT, (gFtResult >> 8) & 0xFF);
    uart_putc(UART_PORT, (gFtResult >> 16) & 0xFF);
    uart_putc(UART_PORT, (gFtResult >> 24) & 0xFF);
}

void ft_result(uint32_t res)
{
    gFtResult = res;
}

void ft_proc(void)
{
    uint8_t ft_cmd = 0;

    ft_cmd = uart_getc(UART_PORT);
    
    if (ft_cmd != CMD_RESULT)
        gFtResult = 0; // clear 0   
    
#ifdef RF_TX_TEST
    // rf tx test
    if ((ft_cmd > 0x00) && (ft_cmd < 0x29))
    {
//        if (ft_cmd == 1)
//        {
//            ft_cmd = 4;
//        }
//        else if(ft_cmd == 9)
//        {
//            ft_cmd = 12;
//        }
//        else if(ft_cmd == 0x11)
//        {
//            ft_cmd = 20;
//        }
//        else if(ft_cmd == 0x19)
//        {
//            ft_cmd = 38; 
//        }
   
        rf_test(ft_cmd, 100);
    }

    // rf rx test
    if ((ft_cmd > 0x80) && (ft_cmd <  0xA9))
    {
//        if (ft_cmd == 0x81)
//        {
//            ft_cmd = 0x84;
//        }
//        else if(ft_cmd == 0x89)
//        {
//            ft_cmd = 0x8C;
//        }
//        else if(ft_cmd == 0x8B)
//        {
//            ft_cmd = 0x94;
//        }
//        else if(ft_cmd == 0x99)
//        {
//            ft_cmd = 0xA6; 
//        }
        
        rf_test(ft_cmd, 100);
    }
#endif    
     switch (ft_cmd)
    {
        
        case CMD_RESULT:
            ft_report_result();
            break;    

#ifdef CORELDO_TEST
        case CMD_CORELDO_INIT:
            ft_coreldo_init();
            break;		
		
        case CMD_CORELDO_RUN_RIS_ADJ:
        case CMD_CORELDO_RUN_FAL_ADJ:    
			ft_coreldo_trim_adj(ft_cmd);
            break;
				
        case CMD_CORELDO_DP_ADJ:
            
            break;		
#endif   
		
#ifdef AONLDO_TEST
        case CMD_AONLDO_INIT:
                ft_aonldo_init();
            break;		
		
        case CMD_AONLDO_RUN_RIS_ADJ:
		case CMD_AONLDO_RUN_FAL_ADJ:
            ft_aonldo_trim_adj(ft_cmd);
            break;
		
        case CMD_AONLDO_OFF_ADJ:
            
            break;		

#endif   		
		
#ifdef RC16M_TEST
        case CMD_RC16M_ADJ:
            clk_out(CLKOUT_HSI);
            rc16m_calib();
            break;
#endif         
	
#ifdef TRIM_DOWNLOAD_TEST
        case CMD_TRIM_LOAD:
            ft_trim_download_test();
            break;
#endif  		
		
        
/*GPIO TESET*/		
#ifdef GPIO_IN_OHEL_TEST      
        case CMD_GPIO_IN_OHEL:
            ft_gpio_in_oddhigh_evenlow_test();
            break;
#endif      
#ifdef GPIO_IN_OLEH_TEST        
        case CMD_GPIO_IN_OLEH:
            ft_gpio_in_oddlow_evenhigh_test();
            break;
#endif
#ifdef GPIO_OUT_OHEL_TEST        
        case CMD_GPIO_OUT_OHEL:
            ft_gpio_out_oddhigh_evenlow_test();
            break;
#endif
#ifdef GPIO_OUT_OLEH_TEST        
        case CMD_GPIO_OUT_OLEH:
            ft_gpio_out_oddlow_evenhigh_test();
            break;
#endif
#ifdef GPIO_PULL_OPED_TEST
        case CMD_GPIO_PULL_OPED:
            ft_gpio_oddpu_evenpd_test();
            break;
#endif
#ifdef GPIO_PULL_ODEP_TEST
        case CMD_GPIO_PULL_ODEP:
            ft_gpio_oddpd_evenpu_test();
            break;
#endif        
   		
#ifdef ADC_TEST
        case CMD_ADC_CHN_TEST:
            ft_adc_chn_test();
            break;
#endif 		
		
#ifdef MIC_TEST
        case CMD_ADC_MIC_TEST:
            ft_adc_mic_test();
            break;
#endif 		

#ifdef FLASH_TEST
        case CMD_FLASH_TEST:
            ft_flash_test();
            break;
#endif   		
		
#ifdef RC32K_TEST
        case CMD_RC32K_FREQ_TEST:
            clk_out(CLKOUT_LSI);
            break;
#endif         
		
#ifdef DPLL_TEST
        case CMD_DPLL_FREQ_TEST:            
             clk_out(CLKOUT_DPLL);
            break;
#endif 		
		
#ifdef USB_TEST
        case CMD_USB_TEST:
            
            break;
#endif 		

#ifdef DEEPSLEEP_CURRENT_TEST
        case CMD_DEEPSLEEP_CURRENT:
            ft_deepsleep_test();
            break;
#endif 		
		
#ifdef POWEROFF_CURRENT_TEST
        case CMD_POWEROFF_CURRENT:
            
            break;
#endif 		

#ifdef MEM_BIST_TEST
        case CMD_MEM_BIST:
            ft_memory_bist_test();
            break;
#endif		
		case TEST_CMD:
            uart_putc(UART_PORT, 0xCC);
            break;
        
        default:
            break;
    }    
}
