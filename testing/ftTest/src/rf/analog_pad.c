#include "afe_common.h"
#include "analog_pad.h"

#define IO_ANALOG_ENABLE_BIT (0x01UL << 10)

// Note: pu, pd, oe, ie dsiable, gpio mode
//void at0_at1_analog_func_enable(void)
//{
//    CSC->CSC_PIO[AT0_PAD].Word = IO_ANALOG_ENABLE_BIT;
//    CSC->CSC_PIO[AT1_PAD].Word = IO_ANALOG_ENABLE_BIT;
//}

// Z-State
void at1_hiz_set(void)
{
    CSC->CSC_PIO[AT1_PAD].Word = 0;
}

// Z-State
void at0_hiz_set(void)
{
    CSC->CSC_PIO[AT0_PAD].Word = 0;
}

// clk_out @see enum clk_out_sel
// Note: IOMCTL clock first enable
void sys_clk_out(uint8_t clk_out)
{
    RCC->CLK_EN_ST.HSE_DIG_EN  = 1;
    RCC->CLK_EN_ST.HSI_DIG_EN  = 1;
    RCC->CLK_EN_ST.LSI_DIG_EN  = 1;
    RCC->CLK_EN_ST.DPLL_DIG_EN = 1;
//    APBMISC->DPLL_CTRL.Word    = 1; 
    if (APBMISC->DPLL_CTRL.DPLL2_EN)
    {
        APBMISC->DPLL_CTRL.DPLL2_EN_CLK48M = 1;
    }
    
    RCC->AHBCLK_EN_RUN.FSHC_CLKEN_RUN    = 1;
    RCC->AHBCLK_EN_RUN.USB_CLKEN_RUN     = 1;
    RCC->AHBCLK_EN_RUN.AHB2P1_CLKEN_RUN  = 1;
    RCC->AHBCLK_EN_RUN.AHB2P2_CLKEN_RUN  = 1;
    RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    
//    switch (clk_out)
//    {
//        case CLK_OUT_LSI:
//        {
//            RCC->CLK_EN_ST.LSI_DIG_EN = 1;
//        } break;

//        case CLK_OUT_HSI:
//        {
//            RCC->CLK_EN_ST.HSI_DIG_EN = 1;
//        } break;  
//        
//        case CLK_OUT_HSE:
//        {
//            RCC->CLK_EN_ST.HSE_DIG_EN = 1;
//        } break;       

//        case CLK_OUT_PLL:
//        {
//            RCC->CLK_EN_ST.DPLL_DIG_EN = 1;
//        } break;

//        default:
//            break;
//    }

    RCC->CFG.MCO_SW = clk_out;
    
    CSC->CSC_PIO[CLK_OUT_PAD].DS   = 1;
    // PA05 sel function2
    CSC->CSC_PIO[CLK_OUT_PAD].FSEL = 2;
}

// Note: IOMCTL clock first enable
void rc16m_clk_out(uint8_t freq_cco, uint8_t freq_cl, uint8_t tc, uint8_t fvch)
{
    #if (0)
    SYSCFG->FRO16MHZCTRL.EN = 1;
    SYSCFG->FRO16MHZCTRL.TRIM_CL = freq_cl; // 0x20
    SYSCFG->FRO16MHZCTRL.TRIM_CCO = freq_cco; // 0x20
    SYSCFG->FRO16MHZCTRL.TRIM_TC = tc;
    SYSCFG->FRO16MHZCTRL.TRIM_FVCH = fvch; // 0x10;
    SYSCFG->FRO16MHZCTRL.EN_OMUX = 1;
    AON->MISCAONCTRL.FRO16M_ISO_ATX_V33 = 1;
    
    // PA04 (reset 0x340)sel function2
    // IOMCTL->PIOA[RC16M_CLK_OUT].FSEL = 2;
    IOMCTL->PIOA[RC16M_CLK_OUT].Word = 0x342;
    #endif
}

#define IO_SEL_DEBUG_FUNC 4

// tx debug map
// IO  | Debug
// 3:0  fsm_pa_gain_vb[3:0]
// 4    fast_lock_done
// 5    pll_lock
// 6    fsm_out_tx_en
// 7    fsm_cal_clken
// 8    fsm_gain_cal_en
// 9    fsm_afc_en
// 10   fsm_tx_ldo_en
// 11   fsm_en_pll
// 12   fsm_in_tx_en

// rx debug map
// IO  | Debug
// 0    fsm_en_agc
// 1    fsm_cal_clken
// 2    fsm_en_pa
// 3    fsm_en_lna
// 4    fast_lock_done
// 5    pll_lock
// 6    fsm_bpf_cal_en
// 7    fsm_out_rx_en
// 8    fsm_afc_en
// 9    fsm_rx_ldo_en
// 10   fsm_en_pll
// 11   fsm_in_rx_en

// rtx_sel:0 --- tx debug, 1 --- rx debug
//void rf_debug_io(uint8_t rtx_sel)
//{
//    for (uint8_t i = 0; i < 13; i++)
//    {
//        CSC->CSC_PIO[i].FSEL = IO_SEL_DEBUG_FUNC;
//    }
//    // 2'b01: tx debug port, 2'b10: rx debug port
//    RF->DIG_CTRL.RF_DBG_SEL = (rtx_sel + 1);
//    SYSCFG->DBG_CTRL.MDM_BBN_DEBUG_SEL = 0; // RF debug
//}

