// rf debug test
#ifndef _RF_DEBUG_TEST_H_
#define _RF_DEBUG_TEST_H_

#include <stdint.h>

#define DEBUG_DLY

/**********************define***************************/
// VAL @see enum at0_at1_func_sel
#define REG_AT0_SEL(VAL)       RF->ANAMISC_CTRL1.AT0_SEL = ((VAL) & 0x0F)
#define REG_AT1_SEL(VAL)       RF->ANAMISC_CTRL1.AT1_SEL = ((VAL) & 0x0F)

/*******************************************************/

enum ldo_vdd_case
{
    LDO_VDD_IF  = 0,
    LDO_VDD_RX  = 1,
    LDO_VDD_PLL = 2,
    LDO_VDD_VCO = 3,
    LDO_VDD_TX  = 4,
};

enum at0_at1_func_sel
{
    // AT0 Mux
    AT0_SEL_VDD_IF         = 0,
    AT0_SEL_BPF_IOP,
    AT0_SEL_BPF_QOP,

    AT0_SEL_VDD_PLL        = 5,
    AT0_SEL_PLL_VCTRL,

    AT0_SEL_LMT_OUTI       = 10,

    AT0_SEL_PLL_FBCLK_DIV2 = 12,

    AT0_SEL_BPF_CAL_DONE   = 14,

    // AT1 Mux
    AT1_SEL_VDD_TX         = 0,
    AT1_SEL_BPF_ION,
    AT1_SEL_BPF_QON,

    AT1_SEL_VDD_RX         = 5,
    AT1_SEL_VDD_VCO,
    AT1_SEL_RSSI_OUT,

    AT1_SEL_LMT_OUTQ       = 10,

    AT1_SEL_PLL_LOCK       = 13,

    AT1_SEL_AGC_POWER_DET  = 15,
    
    AT0_AT1_FUNC_END       = 0xFF,
};

struct rx_lna
{
    uint8_t ant_cap_rx;
    uint8_t rf_rsv;
    uint8_t lna_gain;
    uint8_t mixl_bias_ctl;
    uint8_t mixh_bias_ctl;
    uint8_t bpf_iadj;
    uint8_t bpf_gain_adj;
    uint8_t mixh_enb_cap;
    uint8_t mixh_gain_ctl;
    uint8_t mixl_gain_ctl;
    uint8_t rsv_pad[2];
};

void rf_dbg_init(void);

void vco_freq_test_cmd(uint8_t freq_adj);

void pll_lock_test_cmd(uint8_t pll_dis);

void pll_vtxd_ext_test_cmd(uint8_t vtxd_ext);

// pll_dis 0 ~ 31, pll_frach 0 ~ 15
void pll_tx_1m_lock_test_cmd(uint8_t pll_dis, uint8_t pll_frach);

// pll_dis 0 ~ 31, pll_frach 0 ~ 17
void pll_rx_1m_lock_test_cmd(uint8_t pll_dis, uint8_t pll_frach);

void pa_ramp_down(void);

void pa_ramp_up(void);

void rx_lna_test_cmd(struct rx_lna *param);

uint8_t cbpf_cal_test_cmd(uint8_t cent_adj);

uint8_t tx_gain_cal(uint8_t pll_dis, uint8_t pll_frach);

// test_case @see enum ldo_vdd_case
// trim 0x00 ~ 0x07
void ldo_vdd_test_cmd(uint8_t test_case, uint8_t trim);

void ldo_vdd_pa(uint8_t pa_gain);

// bias_trim:0~3, vref_fine:0~7, res_trim:0~0x1F
void ldo_dig_test_cmd(uint8_t bias_trim, uint8_t vref_fine, uint8_t res_trim);

void tx_pa(uint8_t ant_cap_tx, uint8_t pa_cap, uint8_t pa_gain);

uint8_t dpll256m_lock(void);
uint8_t dpll384m_lock(void);
uint8_t dpll256m_384m_lock(uint8_t dpll_dpll2_sel);
void xo16m_test_cmd(uint8_t xosc16_lp, uint8_t bias_adj, uint8_t cap_trim, uint8_t ldo_xo_tr);
void rc16m_test_cmd(uint8_t rc16m_freq_trim);

void pll_vctrl_at0_cmd(void);
void rx_2m_if(void);

uint8_t get_pll_freq_adj_st(uint8_t chan_num);
#endif  //_RF_DEBUG_TEST_H_
