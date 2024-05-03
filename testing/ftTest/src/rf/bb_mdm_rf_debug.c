#include "regs.h"
#include "bb_mdm_rf_debug.h"

#define IO_SEL_DEBUG_FUNC 4

void debug_io_init(void)
{
    // PA00, PA01 Used to SWD
    for (uint8_t i = 2; i < 13; i++)
    {
        CSC->CSC_PIO[i].Word = IO_SEL_DEBUG_FUNC;
    }
}

/***********************************************************/
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
/***********************************************************/

// rtx_sel:1 --- tx debug, 2 --- rx debug
// @see enum rf_debug
void rf_debug_io(uint8_t rtx_sel)
{
    debug_io_init();
    
    // 2'b01: tx debug port, 2'b10: rx debug port
    RF->DIG_CTRL.RF_DBG_SEL = (rtx_sel & 0x03);
    SYSCFG->DBG_CTRL.MDM_BBN_DEBUG_SEL = MDM_BBN_DEBUG_SEL_RF;
}

/***********************************************************/
// Modem debug(debug_mode = 1) bit Map
// IO | func
// 0   mclk
// 1   reset_n
// 3:2 rate
// 4   tx_en
// 5   tx_data
// 6   tx_valid
// 7   receive_start
// 8   vb_decoder_flush
// 9   sync_pulse
// 10  rx_valid
// 11  rx_data
// 12  tx_invert
// 13  rx_invert
// 14  acc_invert
// 15  iq_invert
/***********************************************************/
// Modem debug(debug_mode = 3) bit Map
// IO | func
// 0   mclk
// 2:1 rate
// 3   receive_start
// 4   vb_decoder_flush
// 5   i_in
// 6   q_in
// 7   sync_pulse
// 8   rx_valid
// 9   rx_data
// 10  demod_ready
// 11  demod_clk
// 12  demod_data
/***********************************************************/
// @see enum mdm_debug
void modem_debug_io(uint8_t debug_mode)
{
    debug_io_init();
    
    MDM->REG0.DEBUG_MODE               = debug_mode;
    SYSCFG->DBG_CTRL.MDM_BBN_DEBUG_SEL = MDM_BBN_DEBUG_SEL_MDM;
}

// Note: pu, pd, oe, ie dsiable, gpio mode
void at0_at1_analog_func_enable(void)
{
    CSC->CSC_PIO[AT0_PAD].Word   = IO_ANALOG_ENABLE_BIT;
    CSC->CSC_PIO[AT1_PAD].Word   = IO_ANALOG_ENABLE_BIT;
//    RF->ANAMISC_CTRL1.AT0_SEL    = 1;
    RF->ANAMISC_CTRL1.AT1_SEL    = 7;//AT1_SEL_RSSI_OUT;
//    RF->ANAMISC_CTRL1.TSTEN_CBPF = 1;
    RF->ANAMISC_CTRL1.TSTEN_RSSI         = 1;
}

void ble_io_init(void)
{
    debug_io_init();

    // BB DIAG1
    WR_32(0x50000050, 0x8300);
    
    SYSCFG->DBG_CTRL.MDM_BBN_DEBUG_SEL = MDM_BBN_DEBUG_SEL_BB;
}

uint32_t g_aon_bkup2_old = 0xFF;
/*static */void mdm_bb_dbg_sel(void)
{
    if (g_aon_bkup2_old == AON->BACKUP2)
    {
        return;
    }
    
    g_aon_bkup2_old = AON->BACKUP2;
    
    switch (g_aon_bkup2_old)
    {
        // RF Tx
        case 0:
        // RF Rx
        case 1:
        {
            RF->DIG_CTRL.RF_DBG_SEL = (g_aon_bkup2_old + 1);
        } break;
        
        // BB 
        case 2:
        {
        } break;
        
        // MDM debug1
        case 3:
        {
            MDM->REG0.DEBUG_MODE    = MDM_DBG1;
        } break;
    }
    
    SYSCFG->DBG_CTRL.MDM_BBN_DEBUG_SEL = (g_aon_bkup2_old == 0 ? 0: g_aon_bkup2_old - 1) ;
}
