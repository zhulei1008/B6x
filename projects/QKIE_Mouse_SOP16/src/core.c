/**
 ****************************************************************************************
 *
 * @file core.c
 *
 * @brief Core APIs of Reset/Sleep/Poweroff...
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "regs.h"

#include "core.h"
#include "gpio.h"
#include "rtc.h"
#include "rcc.h"

#if (DBG_CORE)
#include "dbg.h"
#define DEBUG(format, ...)      //debug("<CORE>" format "\r\n", ##__VA_ARGS__)
#define DBG_TIM_PIO_H()         //(GPIO->DAT_SET = 1UL << PAD_TIM_PIO)
#define DBG_TIM_PIO_L()         //(GPIO->DAT_CLR = 1UL << PAD_TIM_PIO)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#define DBG_TIM_PIO_H()
#define DBG_TIM_PIO_L()
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

// BLE Register field definitions
#define BLE_DEEPSLCNTL_ADDR     0x50000030
#define BLE_DEEPSL_ON_BIT       ((uint32_t)0x00000004)
#define BLE_DEEPSL_STA_MSK      ((uint32_t)0x00008004)

#define BLE_DEEPSLWKUP_ADDR     0x50000034
#define BLE_DEEPSLTIME_MASK     ((uint32_t)0xFFFFFFFF)
#define BLE_DEEPSLTIME_LSB      0
#define BLE_DEEPSLTIME_WIDTH    ((uint32_t)0x00000020)

#define BLE_DEEPSLSTAT_ADDR     0x50000038
#define BLE_DEEPSLDUR_MASK      ((uint32_t)0xFFFFFFFF)
#define BLE_DEEPSLDUR_LSB       0
#define BLE_DEEPSLDUR_WIDTH     ((uint32_t)0x00000020)

#define BLE_ENBPRESET_ADDR      0x5000003C
#define BLE_TWEXT_MASK          ((uint32_t)0xFFE00000)
#define BLE_TWEXT_LSB           21
#define BLE_TWEXT_WIDTH         ((uint32_t)0x0000000B)
#define BLE_TWOSC_MASK          ((uint32_t)0x001FFC00)
#define BLE_TWOSC_LSB           10
#define BLE_TWOSC_WIDTH         ((uint32_t)0x0000000B)
#define BLE_TWRM_MASK           ((uint32_t)0x000003FF)
#define BLE_TWRM_LSB            0
#define BLE_TWRM_WIDTH          ((uint32_t)0x0000000A)

#define BLE_IS_DP_ON()          ( RD_32(BLE_DEEPSLCNTL_ADDR) & BLE_DEEPSL_ON_BIT )
#define BLE_IS_DP_STA()         ((RD_32(BLE_DEEPSLCNTL_ADDR) & BLE_DEEPSL_STA_MSK) == BLE_DEEPSL_STA_MSK)
#define BLE_IS_DP_VALID()       ((APBMISC->BLE_LP_CTRL.BLE_DP_VALID) || (RD_32(BLE_DEEPSLSTAT_ADDR) <= 2)) /*(RD_32(BLE_DEEPSLWKUP_ADDR) - RD_32(BLE_DEEPSLSTAT_ADDR) > 160)*/

#define PMU_ANA_CTRL_DPLL_LDO_POS   18
#define PMU_ANA_CTRL_DPLL_LDO_BIT   ((uint32_t)0x00040000)
#define PMU_ANA_CTRL_LDO_LVD_EN_BIT ((uint32_t)0x00000400)

#define BKUP0_IO_LATCH_BIT          ((uint32_t)0x00008000)

#define LEN_VECTOR  38
#define SRAM_VECTOR 0x20003000

/*
 * UTILS FUNCTIONS
 ****************************************************************************************
 */

#if (ROM_UNUSED)
#define DELAY_TIMER           (1)    // 0-SysTick, 1-BTMR(Base Timer)
#define CR1_ONE_MODE          (0x0C) // one-time mode .URS=1(bit2), .OPM=1(bit3)
#define boot_delayUs(us)      _delay(16, us)

static __inline void _delay(uint16_t tpsc, uint16_t tcnt)
{
#if (DELAY_TIMER)
    // config Params
    BTMR->CR1.Word = BTMR_CR1_MODE;
    BTMR->PSC = tpsc - 1;
    BTMR->ARR = tcnt - 1;
    BTMR->CNT = 0;
    BTMR->EGR = 1;
    
    // enable CEN, wait Time-Reach
    BTMR->CR1.CEN = 1;
    while(!BTMR->RIF && BTMR->CR1.CEN);
    BTMR->ICR = 1;
    
    // clear Mode
    BTMR->CR1.Word = 0;
#else
    uint32_t temp;

    SysTick->LOAD = tcnt * tpsc - 1; 
    SysTick->VAL  = 0x00;                
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    
    do
    {
        temp = SysTick->CTRL;
    } while((temp & SysTick_CTRL_ENABLE_Msk) && !(temp & SysTick_CTRL_COUNTFLAG_Msk));
    
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL   = 0x00;
#endif
}

__attribute__((section("ram_func")))
void deepsleep(void)
{
    volatile uint32_t reg_sclk, reg_dpll;

    /************************************************************************
     **************             Disable CLK & PWR              **************
     ************************************************************************/
    // wait flash cache idle, fix by 6vp 20220915
    while(SYSCFG->ACC_CCR_BUSY);

    // close dpll, First switch sysclk if dpll used.
    reg_sclk = RCC->CFG.Word;      // .SYSCLK_SW; bit[3:0]
    if ((reg_sclk & 0x0F) == 4/*SCLK_PLL*/)
    {
        RCC->CFG.Word = (reg_sclk & ~0x0F) | 2/*SCLK_HSE*/;
    }
    
    reg_dpll = APBMISC->DPLL_CTRL.Word;
    APBMISC->DPLL_CTRL.Word = 0;   // .DPLL_EN=0, .DPLL2_EN=0
    
    /************************************************************************
     **************              Enter Deep Sleep              **************
     ************************************************************************/
    SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
    __WFI();
    SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
    
    /************************************************************************
     **************             Restore CLK & PWR              **************
     ************************************************************************/

    // enable dpll, restore clk
    APBMISC->DPLL_CTRL.Word = reg_dpll;
    // if enable dpll2_en, delay 50us wait dpll clk stable
    if (reg_dpll & 0x04)
        boot_delayUs(50);    
    RCC->CFG.Word           = reg_sclk;
    
    // Reset FSHC, Restore Single Line
    // 2022.11.05 --- wq.
    RCC->AHBRST_CTRL.FSHC_RSTREQ = 1;
    RCC->AHBRST_CTRL.FSHC_RSTREQ = 0;
}

uint16_t rstrsn(void)  // add 6vp 1118
{
    // reset reason 
    uint16_t ret_st = (RCC->CHIP_RST_ST_CLR.Word & 0xFF); // bit[7:0]
    
    // wakeup src from poweroff
    if (AON->PMU_CTRL.POWEROFF_LOG)
    {
        ret_st |= ((AON->PMU_WKUP_ST.Word & 0x0F) << 8); // bit[3:0] << 8
        APBMISC->POWEROFF_LOG_CLR = 1;
    }
    
    // clear st
    RCC->CHIP_RST_ST_CLR.Word = (0xFF << 8);
    APBMISC->AON_PMU_CTRL.Word |= (1 << APBMISC_WKUP_ST_CLR_POS) | (1 << APBMISC_AON_PMU_INT_CLR_POS);

    return ret_st;
}

void btmr_delay(uint16_t tpsc, uint16_t tcnt)
{
    // config Params
    BTMR->CR1.Word = CR1_ONE_MODE;
    BTMR->PSC = tpsc - 1;
    BTMR->ARR = tcnt - 1;
    BTMR->CNT = 0;
    BTMR->EGR = 1;
    
    // enable CEN, wait Time-Reach
    BTMR->CR1.CEN = 1;
    while(!BTMR->RIF && BTMR->CR1.CEN);
    BTMR->ICR = 1;
    
    // clear Mode
    BTMR->CR1.Word = 0;
}

/// Clock Function, add 6vp 1116
void xo16m_en(void)
{
    // xosc16m mode clk configure
    AON->XOSC16M_CTRL.EN_LDO_SEL = 0;  // EN_LDO_XOSC is controlled by XOSC16M_EN
    AON->PMU_CTRL.OSC_EN_RUN     = 1;
    APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 1; // *default 1 from reset
    
    // Wait 2ms for oscillator stable
    boot_delayUs(2000);   
}

void extclk_en(void)
{
    //AON->XOSC16M_CTRL.EN_LDO_SEL = 1; // EN_LDO_XOSC is controlled by EN_LDO_XOSC_REG
    //AON->XOSC16M_CTRL.EN_LDO_XOSC_REG = 1; // EN_LDO_XOSC support power to DPLL  
    AON->XOSC16M_CTRL.Word = (1 << AON_EN_LDO_XOSC_REG_POS) | (1 << AON_EN_LDO_SEL_POS);
    AON->PMU_CTRL.OSC_EN_RUN = 0;
    APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 0;
    AON->BKHOLD_CTRL.XOSC_EST_MODE = 1;
}

void dpll_en(void)
{
    // enable dpll power and clk_en
    AON->PMU_ANA_CTRL.ANA_RESV |= 0x4; // bit[2]  LDO_DPLL EN
    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
    
    // wait 50us for dpll stable // modify by duao 2022.11.3:200us->50us
    boot_delayUs(50);
}
#endif // (ROM_UNUSED)

__attribute__((section("ram_func")))
void deepsleep_rc16m(void)
{
    volatile uint32_t reg_sclk, reg_dpll;

    /************************************************************************
     **************             Disable CLK & PWR              **************
     ************************************************************************/
    // wait flash cache idle, fix by 6vp 20220915
    while(SYSCFG->ACC_CCR_BUSY);

    // close dpll, First switch sysclk if dpll used.
    reg_sclk = RCC->CFG.Word;      // .SYSCLK_SW; bit[3:0]
    
    // switch rc16m
    RCC->CFG.Word = (reg_sclk & ~0x0F) | 1/*SCLK_HSI*/;
    
    // close xobuf
    APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 0;
    APBMISC->AON_PMU_CTRL.OSC_EN_DP   = 1;
    // XOSC16M_LP=0,about 150uA
    APBMISC->XOSC16M_CTRL.XOSC16M_LP  = 1;

    /**************************************************************/
    // Add. door --- 2023.07.04
    uint32_t reg_usb_ctrl = SYSCFG->USB_CTRL.Word;
    SYSCFG->USB_CTRL.Word &= ~(0x01 << SYSCFG_DIG_USB_RXEN_POS);
    
    // reduce coreldo bias current
    AON->PMU_ANA_CTRL.LDO12_IBSEL = 0x1F;
    
    RF->ANA_PWR_CTRL.Word         = 0x007F7F00; // dis CLK_EN_PLL
    RF->ANA_EN_CTRL.Word          = 0x00060000; // dis EN_BG
//    RF->DIG_CTRL.LDO_TEST_EN    = 0;
    /**************************************************************/
    
    reg_dpll = APBMISC->DPLL_CTRL.Word;
    APBMISC->DPLL_CTRL.Word = 0;   // .DPLL_EN=0, .DPLL2_EN=0
    
    /************************************************************************
     **************              Enter Deep Sleep              **************
     ************************************************************************/
    SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
    __WFI();
    SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
    
    /************************************************************************
     **************             Restore CLK & PWR              **************
     ************************************************************************/
    // xobuf enable, sysclk switch to xo16m
    APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 1;
    APBMISC->XOSC16M_CTRL.XOSC16M_LP  = 0;
    /**************************************************************/
    // Add. door --- 2023.07.04
    // reduce coreldo bias current
    AON->PMU_ANA_CTRL.LDO12_IBSEL = 0x10;
    
    RF->ANA_EN_CTRL.Word          = 0x00070000; // en EN_BG
    RF->ANA_PWR_CTRL.Word         = 0x07FF7F08; // en CLK_EN_PLL
//    RF->DIG_CTRL.LDO_TEST_EN    = 1;
    SYSCFG->USB_CTRL.Word         = reg_usb_ctrl;
    /**************************************************************/
    
    // enable dpll, restore clk
    APBMISC->DPLL_CTRL.Word = reg_dpll;
    // if enable dpll2_en, delay 50us wait dpll clk stable
    if (reg_dpll & 0x04)
        btmr_delay(16, 50);
    
    // restore clk
    RCC->CFG.Word           = reg_sclk;

    // Reset FSHC, Restore Single Line
    // 2022.11.05 --- wq.
//    RCC->AHBRST_CTRL.FSHC_RSTREQ = 1;
//    RCC->AHBRST_CTRL.FSHC_RSTREQ = 0;
}

void tick_delay(uint16_t tpsc, uint16_t tcnt)
{
    uint32_t temp;

    SysTick->LOAD = tcnt * tpsc - 1; 
    SysTick->VAL  = 0x00;                
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    
    do
    {
        temp = SysTick->CTRL;
    } while((temp & SysTick_CTRL_ENABLE_Msk) && !(temp & SysTick_CTRL_COUNTFLAG_Msk));
    
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL   = 0x00;
}

void core_init(void)
{
    // CLK enable
    RCC_APBCLK_EN(APB_BSTIM1_BIT | APB_APBMISC_BIT | APB_AON_BIT | APB_IWDT_BIT);
    RCC_AHBCLK_EN(AHB_SYSCFG_BIT);
    
    // dpll clk select 64m
    RCC->CFG.DPLL_CLK_SW = 0;
    // EM clk enable
    RCC->BLE_CLKRST_CTRL.BLE_AHBEN = 1;
    // EM power enable, wait stable   
    AON->PMU_CTRL.EM_PD_EN = 0;
    while (AON->PMU_CTRL.EM_PD_ACK);
    AON->PMU_CTRL.EM_ACCESS_EN = 1;
    
    // io latch enable
    AON->PMU_WKUP_CTRL.IO_LATCH_N = 1;
    // reset pin enable
    AON->BKHOLD_CTRL.PIOA19_FUNC_SEL = 0;
}

// step uint:rc32k clk(31.25us)
void core_clk_stable(uint8_t step)
{
    APBMISC->AON_PMU_CTRL.CLK_STB_TIME = step;
}

void core_pmuset(uint32_t pmu_ctrl)
{
    //if (pmu_ctrl == 0)
    //{
    //    pmu_ctrl = CFG_PMU_DFLT_CNTL;
    //}

//    APBMISC->AON_PMU_CTRL.Word = pmu_ctrl;
    
    DEBUG("pmu:%X\r\n", APBMISC->AON_PMU_CTRL.Word);
}

void core_ldoset(uint32_t ctrl)
{
    // ctrl bit[23:0]
    AON->PMU_ANA_CTRL.Word = (ctrl & 0x1FFFFFF);
    // ctrl bit[29:24]
    APBMISC->ANAMISC_CTRL.Word = (ctrl >> 25);
}

uint32_t core_ldoget(void)
{
    uint32_t val = (AON->PMU_ANA_CTRL.Word & 0x1FFFFFF);
    
    val |= (APBMISC->ANAMISC_CTRL.Word << 25);
    return val;
}

/**
 ****************************************************************************************
 * @section SLEEP FUNCTIONS
 ****************************************************************************************
 */

/// Bits field of peripheral enabled
enum peri_bfs
{
    PERI_SYSTICK_EN_BIT     = (1 << 0),
    PERI_SADC_EN_BIT        = (1 << 1),
};

void core_dptw_set(uint32_t tw_ble)
{
    APBMISC->BLE_DP_TWCORE = tw_ble;
}

uint16_t core_sleep(uint16_t wkup_en)
{
    uint32_t irq_bak;
    uint32_t peri_en = 0;
    uint16_t wkup_st = 0;

    wkup_en &= CFG_WKUP_DEEPSL_MSK;

    // Don't enter core sleep, if no wakeup source
    if (wkup_en == 0) return 0;

    GLOBAL_INT_DISABLE();
    DBG_TIM_PIO_H();

    // Confirmed if no WKUP_BLE_EN, or else to do
    bool confirm = ((wkup_en & CFG_WKUP_BLE_EN) == 0);

    if (confirm || BLE_IS_DP_ON())
    {
        // Disable SysTick if possible
        if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
        {
            SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
            peri_en |= PERI_SYSTICK_EN_BIT;
        }

        // Backup and Disable External IRQs
        irq_bak = NVIC->ISER[0];
        NVIC->ICER[0] = irq_bak;
        
        // Enable AON_PMU, config WKUP_CTRL
        AON->PMU_WKUP_CTRL.Word = (AON->PMU_WKUP_CTRL.Word & ~CFG_WKUP_DEEPSL_MSK) | wkup_en;
        APBMISC->AON_PMU_CTRL.Word      |= CFG_WKUP_ST_CLR_MSK;
        APBMISC->AON_PMU_CTRL.OSC_EN_DP  = 0;
        
        NVIC->ICPR[0] = (1 << AON_PMU_IRQn) | (1 << BB_LP_IRQn);
        NVIC->ISER[0] = (1 << AON_PMU_IRQn);
        
        // Disable peripheral: SADC ...
        if (SADC->SADC_ANA_CTRL.SADC_EN)
        {
            SADC->SADC_ANA_CTRL.SADC_EN = 0;
            peri_en |= PERI_SADC_EN_BIT;
        }
        
        /**************************************************************/
        // Add. door --- 2023.07.04
        uint32_t reg_usb_ctrl = SYSCFG->USB_CTRL.Word;
        SYSCFG->USB_CTRL.Word &= ~(0x01 << SYSCFG_DIG_USB_RXEN_POS);
        
        // reduce coreldo bias current
        AON->PMU_ANA_CTRL.LDO12_IBSEL = 0x1F;
        
        // close dpll, First switch sysclk if dpll used.
        uint32_t reg_sclk = RCC->CFG.Word;      // .SYSCLK_SW; bit[3:0]
        if ((reg_sclk & 0x0F) == 4/*SCLK_PLL*/)
        {
            RCC->CFG.Word = (reg_sclk & ~0x0F) | 2/*SCLK_HSE*/;
        }
        
        RF->ANA_PWR_CTRL.Word = 0x007F7F00; // dis CLK_EN_PLL
        RF->ANA_EN_CTRL.Word  = 0x00060000; // dis EN_BG
        /**************************************************************/

        // Wait BLE been deepsleep state, check enough time or not
        if (!confirm)
        {
            while (BLE_IS_DP_ON())
            {
                if (BLE_IS_DP_STA())
                {
                    DBG_TIM_PIO_L();
                    confirm = BLE_IS_DP_VALID();
                    DBG_TIM_PIO_H();
                    break;
                }
            }
        }

        // Confirm to enter deep sleep
        if (confirm)
        {
            DBG_TIM_PIO_L();
            deepsleep();
            DBG_TIM_PIO_H();
        }

        /**************************************************************/
        // Add. door --- 2023.07.04
        // reduce coreldo bias current
        AON->PMU_ANA_CTRL.LDO12_IBSEL = 0x10;
        
        RF->ANA_EN_CTRL.Word  = 0x00070000; // en EN_BG
        RF->ANA_PWR_CTRL.Word = 0x07FF7F08;
        
        RCC->CFG.Word         = reg_sclk;
        SYSCFG->USB_CTRL.Word = reg_usb_ctrl;
        /**************************************************************/
        
        // Enable peripheral: SADC ...
        if (peri_en & PERI_SADC_EN_BIT)
        {
            SADC->SADC_ANA_CTRL.SADC_EN = 1;
        }

        // Disable AON_PMU, Read & Clear WKUP_ST
        NVIC->ICER[0] = (1 << AON_PMU_IRQn);
        NVIC->ICPR[0] = (1 << AON_PMU_IRQn) | (1 << BB_LP_IRQn);
        wkup_st = AON->PMU_WKUP_ST.Word & CFG_WKUP_ST_MSK;
        APBMISC->AON_PMU_CTRL.Word |= CFG_WKUP_ST_CLR_MSK;
        
        // Restore External IRQs
        NVIC->ISER[0] = irq_bak;

        // Restore SysTick
        if (peri_en & PERI_SYSTICK_EN_BIT)
        {
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        }
    }
    else
    {
        // Don't enter core sleep, if BLE is ongoing
    }

    DBG_TIM_PIO_L();
    GLOBAL_INT_RESTORE();

    return wkup_st;
}

/// Const Pointer of deepsleep func(run-in-ram), BLX Jump
void (* const _SLEEP)(void) = deepsleep_rc16m;

uint16_t core_sleep_rc16m(uint16_t wkup_en)
{
    uint32_t irq_bak;
    uint32_t peri_en = 0;
    uint16_t wkup_st = 0;

    wkup_en &= CFG_WKUP_DEEPSL_MSK;

    // Don't enter core sleep, if no wakeup source
    if (wkup_en == 0) return 0;

    GLOBAL_INT_DISABLE();
    DBG_TIM_PIO_H();

    // Confirmed if no WKUP_BLE_EN, or else to do
    bool confirm = ((wkup_en & CFG_WKUP_BLE_EN) == 0);

    if (confirm || BLE_IS_DP_ON())
    {
        // Disable SysTick if possible
        if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
        {
            SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
            peri_en |= PERI_SYSTICK_EN_BIT;
        }

        // Backup and Disable External IRQs
        irq_bak = NVIC->ISER[0];
        NVIC->ICER[0] = irq_bak;
        
        // Enable AON_PMU, config WKUP_CTRL
        AON->PMU_WKUP_CTRL.Word = (AON->PMU_WKUP_CTRL.Word & ~CFG_WKUP_DEEPSL_MSK) | wkup_en;
        APBMISC->AON_PMU_CTRL.Word |= CFG_WKUP_ST_CLR_MSK;

        NVIC->ISER[0] = (1 << AON_PMU_IRQn);
        NVIC->ICPR[0] = (1 << AON_PMU_IRQn);
        
        // Disable peripheral: SADC ...
        if (SADC->SADC_ANA_CTRL.SADC_EN)
        {
            SADC->SADC_ANA_CTRL.SADC_EN = 0;
            peri_en |= PERI_SADC_EN_BIT;
        }
        
        // Wait BLE been deepsleep state, check enough time or not
        if (!confirm)
        {
            while (BLE_IS_DP_ON())
            {
                if (BLE_IS_DP_STA())
                {
                    DBG_TIM_PIO_L();
                    confirm = BLE_IS_DP_VALID();
                    DBG_TIM_PIO_H();
                    break;
                }
            }
        }

        // Confirm to enter deep sleep
        if (confirm)
        {
            DBG_TIM_PIO_L();
            _SLEEP();
//            #if (CFG_FLASH_DXIP)
//            puya_enter_dual_read();
//            #endif
            DBG_TIM_PIO_H();
        }

        // Enable peripheral: SADC ...
        if (peri_en & PERI_SADC_EN_BIT)
        {
            SADC->SADC_ANA_CTRL.SADC_EN = 1;
        }

        // Disable AON_PMU, Read & Clear WKUP_ST
        NVIC->ICER[0] = (1 << AON_PMU_IRQn);
        NVIC->ICPR[0] = (1 << AON_PMU_IRQn);
        wkup_st = AON->PMU_WKUP_ST.Word & CFG_WKUP_ST_MSK;
        APBMISC->AON_PMU_CTRL.Word |= CFG_WKUP_ST_CLR_MSK;
        
        // Restore External IRQs
        NVIC->ISER[0] = irq_bak;

        // Restore SysTick
        if (peri_en & PERI_SYSTICK_EN_BIT)
        {
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        }
    }
    else
    {
        // Don't enter core sleep, if BLE is ongoing
    }

    DBG_TIM_PIO_L();
    GLOBAL_INT_RESTORE();

    return wkup_st;
}

/**
 ****************************************************************************************
 * @section POWER-OFF FUNCTIONS
 ****************************************************************************************
 */

enum wakeup_io_edge
{
    PWIO_EDGE_RISING        = (IOM_SEL_GPIO | IOM_INPUT | IOM_PULLDOWN),
    PWIO_EDGE_FALLING       = (IOM_SEL_GPIO | IOM_INPUT | IOM_PULLUP),
};

void wakeup_io_sw(uint32_t wkup_en, uint32_t pad_pu)
{
    APBMISC->PIOA_WKUP_NEG = pad_pu;
    APBMISC->PIOA_WKUP_POS = wkup_en^pad_pu;
    
    if ((1UL << PA_RSTPIN) & wkup_en)
    {
        AON->BKHOLD_CTRL.PIOA19_FUNC_SEL = 1; // as gpio
    }
    
    // conf iopad for wakeup
    for (uint8_t io_idx = 0; io_idx < PA_MAX; ++io_idx)
    {
        if ((1UL << io_idx) & wkup_en)
        {
            CSC->CSC_PIO[io_idx].Word = (pad_pu & (1UL << io_idx)) ? PWIO_EDGE_FALLING : PWIO_EDGE_RISING;
            DEBUG("PA%02d, CSC:%x", io_idx, CSC->CSC_PIO[io_idx].Word);
        }
    }

    GPIO->DIR_CLR = wkup_en;
    
    DEBUG("wkup_en:%x, pad_pu:%x", wkup_en, pad_pu);
    DEBUG("neg:%x, pos:%x", APBMISC->PIOA_WKUP_NEG, APBMISC->PIOA_WKUP_POS);
}

static __inline void pwroff_clk_sw(void)
{
    // colse dpll: flash
    uint8_t fshclk = RCC->CLK_EN_ST.FSHCCLK_SEL;
    
    if ((fshclk == FSH_CLK_DPLL128) || (fshclk == FSH_CLK_DPLL))
    {
        RCC->CLK_EN_ST.FSHCCLK_SEL = FSH_CLK_HSE16;
    }
}

__weak void ble_retain(void)
{
    // todo in BLE lib
}

void core_pwroff(uint16_t cfg_wkup)
{
    uint16_t wkup_en = (cfg_wkup & CFG_WKUP_PWROFF_MSK) | (WKUP_IO_LATCH_N_BIT | WKUP_BLE_LATCH_N_BIT);
        
    GLOBAL_INT_STOP();
    
    if (cfg_wkup & WKUP_IO_LATCH_N_BIT)
    {
        wkup_en &= ~WKUP_IO_LATCH_N_BIT;
        
        // set1 boot no release io latch
        AON->BACKUP0 |= BKUP0_IO_LATCH_BIT;
    }

    if (cfg_wkup & WKUP_BLE_SEL_MSK)
    {
        wkup_en &= ~WKUP_BLE_LATCH_N_BIT;
        ble_retain();
    }
    
    if (wkup_en & WKUP_RTC_EN_BIT)
    {
        //rtc_alarm_set(rtc_ms); remove to outside
    }
    
    // Fixed Bug: Core1V2 not down to 0V when LDO_TEST_EN=1 - add 6vp 0720 
    RF->DIG_CTRL.Word     = 8; // .FSM_CTRL_SEL = 1 //RF->DIG_CTRL.LDO_TEST_EN = 0;

    // wakeup ctrl - set en and clr st
    AON->PMU_WKUP_CTRL.Word = wkup_en;

    //APBMISC->AON_PMU_CTRL.WKUP_ST_CLR = 1;
    APBMISC->AON_PMU_CTRL.Word |= CFG_WKUP_ST_CLR_MSK;
    APBMISC->POWEROFF_LOG_CLR   = 1;

    // dpll close
    pwroff_clk_sw();

    // dpll ldo and lvd close. door --- 2023.07.03
    AON->PMU_ANA_CTRL.Word &= ~(PMU_ANA_CTRL_DPLL_LDO_BIT | PMU_ANA_CTRL_LDO_LVD_EN_BIT);

    // enter poweroff
    APBMISC->POWEROFF_WORD_CMD = 0x77ED29B4;
    while (1);
}

void core_release_io_latch(void)
{
    AON->BACKUP0 &= ~BKUP0_IO_LATCH_BIT;
    AON->PMU_WKUP_CTRL.IO_LATCH_N = 1;
}

void core_vector(uint32_t addr)
{
    uint32_t i;
    
    for (i = 0; i < LEN_VECTOR; i++)
    {
        WR_32((SRAM_VECTOR + (i << 2)), RD_32(addr+ (i << 2))); // copy
    }
    
//    __disable_irq();
    // Set SCB->VTOR to SRAM(0x20003000)
    __DMB();
    SCB->VTOR = SRAM_VECTOR;
    __DSB();
}
