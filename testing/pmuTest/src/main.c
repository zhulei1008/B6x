/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "regs.h"
#include "dbg.h"
#include "deepsleep.h"
#include "poweroff.h"
/*
 * DEFINES
 ****************************************************************************************
 */

void sysInit(void)
{    
    #if (FPGA_TEST)
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 2; // BB Must 16M
    #else
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 0; // BB Must 16M
    #endif
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    //dbgInit();
    //debug("Start(rsn:0x%X)...\r\n", rsn);
    
    AON->PMU_WKUP_CTRL.IO_LATCH_N     = 1;
//    AON->PMU_WKUP_CTRL.BLE_LATCH_N    = 1;
    
    rc32k_conf(RCLK_DPLL, 0x0F);
    rc32k_calib();
    
    //core_pmuset(CFG_PMU_DFLT_CNTL);    
    
}

#define BB_BASE 0x50000000

void bleWkepSet(uint32_t time, uint32_t twosc, uint32_t twrm)
{
    // set bb deepsleep time
    (*(volatile uint32_t *)(BB_BASE+0x34))  = time; 
    //OSC en before module leave low-power mode
    (*(volatile uint32_t *)(BB_BASE+0x3C)) |= twosc <<10 ; 
    // radio en before module leave low-power mode
    (*(volatile uint32_t *)(BB_BASE+0x3C)) |= twrm ; 
    //OSC_SLEEP_EN enable
    (*(volatile uint32_t *)(BB_BASE+0x30)) |= 0x01 ; 
    //RADIO_SLEEP_EN disable
    (*(volatile uint32_t *)(BB_BASE+0x30)) &=~ 0x02 ; 
    //DEEP_SLEEP_ON    
    (*(volatile uint32_t *)(BB_BASE+0x30)) |= 0x04 ; 
    //deepsleep correct enable    
    (*(volatile uint32_t *)(BB_BASE+0x30)) |= 0x08 ; 
    // delay 150us
    btmr_delay(16, 150);
    AON->PMU_WKUP_CTRL.BLE_WKUP_SEL = 1;
}


void pmuPowerSet(uint8_t ldo_dp, uint8_t aon_pwr_dp, uint8_t osc_dp, uint8_t hsi_dp, uint16_t clk_stb_time)
{
    uint32_t tmp_word, aon_pmu_ctrl;
    
    aon_pmu_ctrl = APBMISC->AON_PMU_CTRL.Word;
    aon_pmu_ctrl &= 0xFFC0F0FF;
    
    tmp_word = (ldo_dp << APBMISC_LDO_LP_SEL_DP_POS) | (aon_pwr_dp << APBMISC_AON_PWR_SEL_DP_POS) | \
                (osc_dp << APBMISC_OSC_EN_DP_POS) | (hsi_dp << APBMISC_HSI_EN_DP_POS) | \
                (clk_stb_time << APBMISC_CLK_STB_TIME_LSB);
    
    APBMISC->AON_PMU_CTRL.Word = aon_pmu_ctrl | tmp_word;
}


#define WKUP_IO_MASK 0x80

#if 1
#define DEEPSLEEP
#endif

int main(void)
{
    uint32_t i, wkup_pad, tmp_word;
    
    sysInit();
    devInit();
    // coreldo adjust to 1.2
    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = 0x1b;    
    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = 0x0;

#if 0     // rc32k adjust
    iospc_clkout(CLK_OUT_LSI);    
    rc32k_conf(RCLK_DPLL, RCAL_CYCLES(0x1f));
    rc32k_calib();
#endif
    
    //APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 0;

 
//    while(1);
    bootDelayMs(5000);
    
//    AON->PMU_ANA_CTRL.Word &=~ (1 << 18);// 0xFFFBFFFF ;
//    AON->PMU_ANA_CTRL.ANA_RESV |= 2;
//    iom_ctrl(17, IOM_ANALOG);


    //AON->PMU_CTRL.CORELDO_EN_RUN = 1;
    // close dpll en
    APBMISC->DPLL_CTRL.Word = 0;
    AON->PMU_ANA_CTRL.LDO_LVD_EN = 0;
    AON->PMU_ANA_CTRL.LDO_BOD_EN = 0;

////    pwroff_io_sw(0xfffff, 0xfffff);
////    core_sleep(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);      
   


#ifdef DEEPSLEEP
//    APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP = 0x14; 
//    APBMISC->LDO_UD_CTRL.CORELDO_TRIM_STEP = 7;   

    AON->PMU_ANA_CTRL.Word &=~ (1 << 18);// 0xFFFBFFFF ;    
    pmuPowerSet(1, 0, 0, 0, 0x1f);
    deepsleepTest(); 

#else
    APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF = 0x4; 
    APBMISC->LDO_UD_CTRL.AONLDO_UD_STEP = 7;
    AON->PMU_ANA_CTRL.Word &=~ (1 << 18);// 0xFFFBFFFF ;    
    poweroffTest();

#endif
        
//    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 0;
//    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN = 0xf;
    while(1)
    {
#if 0        
        uint8_t cmd = uart_getc(0);
        //debug("cmd:%x, pmu_wkup_ctrl:%x, %d\r\n", cmd, AON->PMU_WKUP_CTRL.Word, AON->PMU_CTRL.POWEROFF_LOG);
        switch (cmd)
        {
            case 0x1 :
                AON->BKHOLD_CTRL.CORELDO_TRIM_RUN -= 1;
                debug("CORELDO_TRIM_RUN = %d\r\n", AON->BKHOLD_CTRL.CORELDO_TRIM_RUN);
            break;
            
            case 0x2 :
                AON->BKHOLD_CTRL.AONLDO_TRIM_RUN += 1;
                debug("AONLDO_TRIM_RUN = %d\r\n", AON->BKHOLD_CTRL.AONLDO_TRIM_RUN);
            break;
            
            ///////// DEEPSLEEP /////////
            case 0x11:
            {
                for (i = 0; i < 20; i++)
                {
                    if ((i == 6) || (i == 7))
                        continue;
                    //wkup_pad = 1 << i;
                    // rise wkup
                    pwroff_io_sw(0xfff9f, 0xfff9f);
                    core_sleep(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);                
                }
                
            } break;
            
            case 0x22:
            {
                for (i = 0; i < 20; i++)
                {
                    if ((i == 6) || (i == 7))
                        continue;
                    //wkup_pad = 1 << i;
                    // fall wkup
                    pwroff_io_sw(0xfff9f, 0);
                    core_sleep(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);                
                }
            } break;
            
            case 0x33: // ble wakeup
                bleWkepSet(0x10000, 0x1000, 0x100);
                core_sleep(CFG_WKUP_BLE_EN);
            break;
 
            case 0x44: // rtc wakeup
                rtc_conf(true);
                rtc_alarm_set(5000);
                core_sleep(CFG_WKUP_RTC_EN);
            break; 

            
            ///////// POWEROFF /////////
            case 0x66:
            {
                
                for (i = 0; i < 20; i++)
                {
                    //wkup_pad = 1 << i;
                    // rise wkup
                    pwroff_io_sw(0xfff9f, 0xfff9f);
                    core_pwroff(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);                
                }
                
            } break;
            
            case 0x77:
            {
                for (i = 0; i < 20; i++)
                {
                    wkup_pad = 1 << i;
                    // fall wkup
                    pwroff_io_sw(0xfff9f, 0);
                    core_pwroff(WKUP_PWIO_EN_BIT/* | WKUP_IO_LATCH_N_BIT*/);                
                }
            } break;
            
            case 0x88: // ble wakeup
                bleWkepSet(0x10000, 0x1000, 0x100);                
                core_pwroff(CFG_WKUP_BLE_EN);
            break;
 
            case 0x99: // rtc wakeup
                rtc_conf(true);
                rtc_alarm_set(5000);            
                core_pwroff(CFG_WKUP_RTC_EN);
            break;
            
            default:
            {
            } break;
        };
#endif        
    }
    
}
