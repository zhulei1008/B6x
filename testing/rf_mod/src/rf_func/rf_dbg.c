#include "regs.h"
#include "drvs.h"
#include "sysdbg.h"

/**
 **********************************
 * attribute
 * RW(rw): Read Write
 * RO(ro): Read Only
 * WO(wo): Write Only
 **********************************
*/
#define at0_hiz_set() iom_ctrl(PA_AT0, IOM_HIZ)
#define at1_hiz_set() iom_ctrl(PA_AT1, IOM_HIZ)

#if defined (DEBUG_DLY)
#define DEBUG_DELAY(VAL) for (uint16_t t = 0; t < VAL; t++)
#else
#define DEBUG_DELAY(VAL)
#endif

// DAC_VCTRL_EN = EN_PLL & gain_cal_en
// EN_VCTRL_BUF = EN_PLL & (~gain_cal_en), VCO模式EN_VCTRL_BUF=1
// gain_cal_en  = pll_gain_cal_by ? sw_gain_cal_en : fsm_gain_cal_en;

// PLL_CALIB_PD        = pll_cal_test_en ? PLL_CALIB_PD_REG : (pll_calib_pd_mux | afc_calib_done)
// TX_OPEN_PD          = pll_cal_test_en ? TX_OPEN_PD_REG// : (pll_calib_pd_mux | afc_calib_done)
// PLL_BAND_CALIB_DONE = pll_cal_test_en ? BAND_CAL_DONE_REG : afc_calib_done

void rf_debug_test_init(void)
{
    // 0x00, Read Write
    RF->DIG_CTRL.ANA_TEST_EN            = 1;
    RF->DIG_CTRL.LDO_TEST_EN            = 0;
    RF->DIG_CTRL.PLL_CAL_TEST_EN        = 0;
    RF->DIG_CTRL.FSM_CTRL_SEL           = 1;
    RF->DIG_CTRL.PA_GAIN_TARGET_SEL     = 1;
    RF->DIG_CTRL.PLL_AFC_BY             = 0;
    RF->DIG_CTRL.PLL_BPF_CAL_BY         = 0;
    RF->DIG_CTRL.PLL_FREQ_SEL           = 1;
    RF->DIG_CTRL.PLL_FREQ_EXT_EN        = 0;
    RF->DIG_CTRL.PLL_GAIN_CAL_BY        = 0;
    RF->DIG_CTRL.PLL_VTXD_EXT_EN        = 0;
    RF->DIG_CTRL.PLL_GAIN_CAL_STEP      = 0;
    RF->DIG_CTRL.PLL_GAIN_CAL_TAB       = 1;
    RF->DIG_CTRL.PLL_GAIN_CAL_MODE      = 0;
    RF->DIG_CTRL.PLL_AFC_ROUND          = 1;
    RF->DIG_CTRL.PLL_AFC_STEP           = 0;
    RF->DIG_CTRL.PLL_FREQ_DC            = 0;
    RF->DIG_CTRL.PLL_AFC_MODE           = 0;
    RF->DIG_CTRL.PLL_AFC_FRAC_EN        = 1;
    RF->DIG_CTRL.SW_PLL_CAL_CLKEN       = 0;
    RF->DIG_CTRL.SW_AFC_EN              = 0;
    RF->DIG_CTRL.SW_GAIN_CAL_EN         = 0;
    RF->DIG_CTRL.SW_PLL_VREF_SEL        = 0;
    RF->DIG_CTRL.SW_PLL_HBW_EN          = 0;
    RF->DIG_CTRL.SDM_FRAC_INT_MODE      = 0;
    RF->DIG_CTRL.RF_DBG_SEL             = 0;
    
    // 0x04, Read Write
    RF->PLL_TAB_OFFSET.TX_FRAC_OFFSET   = 0;
    RF->PLL_TAB_OFFSET.RX_FRAC_OFFSET   = 0;
    RF->PLL_TAB_OFFSET.PLL_FREQ_EXT     = 0x00;

    // 0x08, Read Write
    RF->PLL_GAIN_CTRL.PLL_VTXD_EXT      = 0x10;
    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_DC   = 0;
    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_WIN  = 1;
    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 0x50;

    // 0x0C, Mixed
//    RF->RSSI_CTRL.RSSI_VAL               = 0;
//    RF->RSSI_CTRL.AGC_LNA_GAIN           = 0;
//    RF->RSSI_CTRL.AGC_BPF_GAIN_ADJ       = 0;
//    RF->RSSI_CTRL.AGC_MIXL_GAIN_CTL      = 0;
//    RF->RSSI_CTRL.AGC_MIXH_GAIN_CTL      = 0;
//    RF->RSSI_CTRL.SW_RSSI_READY          = 0;
//    RF->RSSI_CTRL.SW_RSSI_REQ            = 0; // rw, bit17

    // 0x10, Read Write
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ00         = 0x10;
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ01         = 0x10;
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ02         = 0x10;
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ03         = 0x10;
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ04         = 0x10;

    // 0x14, Read Only
//    RF->PLL_CAL_ST.AFC_CALIB_DONE;
//    RF->PLL_CAL_ST.GAIN_CALIB_DONE;
//    RF->PLL_CAL_ST.PLL_FREQ_ADJ_ST;
//    RF->PLL_CAL_ST.PLL_GAIN_CAL_ST;
//    RF->PLL_CAL_ST.CALIB_CNT_RPT;
//    RF->PLL_CAL_ST.PLL_GAIN_CAL_ERR;

    // 0x18, Read Write
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S        = 0x15; // tx,0x15 --- 2400MHz
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC        = 0;

    // 0x1C, Read Write
    RF->PLL_DYM_CTRL.SW_CH_NUM           = 0;
    RF->PLL_DYM_CTRL.SW_RATE             = 0; // 0:1M, 1:2M
    RF->PLL_DYM_CTRL.SW_TX_EN            = 1;
    RF->PLL_DYM_CTRL.SW_RX_EN            = 0;
    RF->PLL_DYM_CTRL.SW_PA_GAIN_TARGET   = 0x0F; // bit[13:10]
    
    // 0x20, Read Write, default value
//    RF->FSM_DLY_CTRL0.LDO_START_TIME     = 2;
//    RF->FSM_DLY_CTRL0.EN_PLL_TIME        = 2;
//    RF->FSM_DLY_CTRL0.AFC_START_TIME     = 2;
//    RF->FSM_DLY_CTRL0.BPF_START_TIME     = 0x1E;
//    RF->FSM_DLY_CTRL0.TX_END_DLY_TIME    = 1;
//    RF->FSM_DLY_CTRL0.RX_END_DLY_TIME    = 1;
//    RF->FSM_DLY_CTRL0.TX_PA_RAMPDOWN_TIME= 0;

    // 0x24, Read Write
    RF->FSM_DLY_CTRL1.LNA_START_TIME     = 0x28;
    RF->FSM_DLY_CTRL1.EN_PA_TIME         = 0x10;//0x28; // 0x16, 2022.06.29 -- lch, 0x10, 2022.06.29 -- zl
    RF->FSM_DLY_CTRL1.AGC_START_TIME     = 2;
    RF->FSM_DLY_CTRL1.PA_STARTUP_TIME    = 5;//1;//2;// 1, 2022.06.29 -- lch, 5, 2022.06.29 -- zl
    RF->FSM_DLY_CTRL1.PA_STEP_TIME       = 4;//3;//2;// 2022.06.29 -- lch
   
   // 0x2C, Read Only
//    RF->PLL_GAIN_CAL_VAL.PLL_GAIN_CAL_DF0;
//    RF->PLL_GAIN_CAL_VAL.PLL_GAIN_CAL_DF1;
//    RF->PLL_GAIN_CAL_VAL.PLL_CAL_STATE;

    // 0x30, Read Write
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ10       = 0x10;
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ11       = 0x10;
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ12       = 0x10;
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ13       = 0x10;
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ14       = 0x10;
    
    // 0x34, Read Write
    RF->PLL_DAC_TAB2.PLL_DAC_ADJ05       = 0x10;
    RF->PLL_DAC_TAB2.PLL_DAC_ADJ15       = 0x10;
    
    // 0x38, Read Write
    // 0:SDM data from rf_ana, 1: SDM data from mdm
    RF->DATA_DLY_CTRL.SDM_DAT_SEL        = 0;
    RF->DATA_DLY_CTRL.MDM_SDM_DATA_DLY_1M= 0;
    RF->DATA_DLY_CTRL.DAC_DATA_DLY_1M    = 0;
    RF->DATA_DLY_CTRL.MDM_SDM_DATA_DLY_2M= 0;
    RF->DATA_DLY_CTRL.DAC_DATA_DLY_2M    = 0;
    RF->DATA_DLY_CTRL.SDM_CLK_PH         = 0;
    
    // 0x4C, Read Write
    RF->BPFMIX_CTRL.BPF_BW_ADJ           = 0;//1;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT     = 0x1E;//0x1A;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT_EN  = 1;
    RF->BPFMIX_CTRL.BPF_CAL_EN           = 0;
    RF->BPFMIX_CTRL.BPF_CENT_ADJ         = 0;
    RF->BPFMIX_CTRL.BPF_IADJ             = 6;//4;
    RF->BPFMIX_CTRL.BPF_MODE_SEL         = 0;
    RF->BPFMIX_CTRL.MIXL_BIAS_CTL        = 1;//3;
    RF->BPFMIX_CTRL.MIXL_BIAS_SEL        = 0;
    RF->BPFMIX_CTRL.MIXH_ENB_CAP         = 0;
    RF->BPFMIX_CTRL.MIXH_BIAS_CTL        = 2;//3; // bit[18:16]
    RF->BPFMIX_CTRL.MIXH_BIAS_SEL        = 0;
    RF->BPFMIX_CTRL.PA_CAP               = 0; // bit[28:25] --- 调匹配电容

    // AGC最后测试
    // 0x50, Read Write
    // 除了带lna的, 其它和DragonB一样
    // 带lna的参数配置. 2022.09.06 --- rj. 
    RF->AGC_CTRL0.AGC_S00H               = 0x2B; // bit[5:0]
    RF->AGC_CTRL0.AGC_S00L               = 0x15; // bit[11:6]
    RF->AGC_CTRL0.AGC_S00_BPF_ADJ        = 3;    // bit[13:12]
    RF->AGC_CTRL0.AGC_S00_LNA_ADJ        = 3;    // bit[15:14]
    RF->AGC_CTRL0.AGC_S00_LNA_BYPS_ADJ   = 1;    // bit16
    RF->AGC_CTRL0.AGC_S00_LNA_EN_ADJ     = 1;    // bit17
    RF->AGC_CTRL0.AGC_S00_MIX_ADJ        = 0;    // bit[19:18]
    RF->AGC_CTRL0.AGC_POWER_DET_EN       = 0;    // bit20 置1时limt_i和limt_q没数据
    RF->AGC_CTRL0.AGC_TEST_EN            = 0;    // bit21
    RF->AGC_CTRL0.AGC_TEST_S             = 0;    // bit[23:22]
    RF->AGC_CTRL0.AGC_T_ADJ              = 1;    // bit[25:24]
    RF->AGC_CTRL0.AGC_VH_ADD_ADJ         = 1;    // bit[27:26]
    RF->AGC_CTRL0.DISABLE_AGC            = 1;//0;    // bit28

    // 0x54, Read Write
    RF->AGC_CTRL1.AGC_S01H               = 0x32; // bit[5:0]
    RF->AGC_CTRL1.AGC_S01L               = 0x1A; // bit[11:6]
    RF->AGC_CTRL1.AGC_S01_BPF_ADJ        = 2;    // bit[13:12]
    RF->AGC_CTRL1.AGC_S01_LNA_ADJ        = 3;    // bit[15:14]
    RF->AGC_CTRL1.AGC_S01_LNA_BYPS_ADJ   = 1;    // bit16
    RF->AGC_CTRL1.AGC_S01_LNA_EN_ADJ     = 1;    // bit17
    RF->AGC_CTRL1.AGC_S01_MIX_ADJ        = 1;    // bit[19:18]
    
    // 0x58, Read Write
    RF->AGC_CTRL2.AGC_S10L               = 0x1A; // bit[5:0]
    RF->AGC_CTRL2.AGC_S10_BPF_ADJ        = 1;    // bit[7:6]
    RF->AGC_CTRL2.AGC_S10_LNA_ADJ        = 3;    // bit[9:8]
    RF->AGC_CTRL2.AGC_S10_LNA_BYPS_ADJ   = 0;    // bit10
    RF->AGC_CTRL2.AGC_S10_LNA_EN_ADJ     = 1;    // bit11
    RF->AGC_CTRL2.AGC_S10_MIX_ADJ        = 1;    // bit[13:12]
    RF->AGC_CTRL2.AGC_S11_BPF_ADJ        = 0;    // bit[17:16]
    RF->AGC_CTRL2.AGC_S11_LNA_ADJ        = 0;    // bit[19:18]
    RF->AGC_CTRL2.AGC_S11_LNA_BYPS_ADJ   = 0;    // bit20
    RF->AGC_CTRL2.AGC_S11_LNA_EN_ADJ     = 1;    // bit21
    RF->AGC_CTRL2.AGC_S11_MIX_ADJ        = 2;    // bit[23:22]

    // 0x5C, Read Write
    RF->ANA_TRIM.BG_BIAS_TRIM            = 1;
    RF->ANA_TRIM.BG_RES_TRIM             = 0x1F; //LDO_RX_TRIM = 4时, BG_RES_TRIM=0x17,VDD_IF:1.28V对应VBG1.2V
    RF->ANA_TRIM.BG_VREF_FINE            = 0;
    RF->ANA_TRIM.LDO_RX_TRIM             = 4;
    RF->ANA_TRIM.LDO_TX_TRIM             = 4;
    RF->ANA_TRIM.LNA_RES_TRIM            = 3;//4;
    RF->ANA_TRIM.BPF_GAIN_ADJ            = 3;
    RF->ANA_TRIM.MIXL_GAIN_CTL           = 0;
    RF->ANA_TRIM.MIXH_GAIN_CTL           = 0;
    RF->ANA_TRIM.LNA_GAIN                = 0x0F;

    // 0x60, Read Write
    RF->ANAMISC_CTRL1.DAC_BLE_DELAY_ADJ_1M  = 0;//0x10;
    RF->ANAMISC_CTRL1.DAC_REFL_ADJ       = 0x04; //受温度和批次影响 --- 2022.07.05 谢波
    RF->ANAMISC_CTRL1.DAC_CAL_DATA_EXT   = 0;
    RF->ANAMISC_CTRL1.DAC_CAL_EN_EXT     = 0;
    RF->ANAMISC_CTRL1.DAC_EXT_EN         = 0;
    RF->ANAMISC_CTRL1.DAC_REFH_ADDJ      = 1;
    RF->ANAMISC_CTRL1.DAC_REFH_ADJ       = 0;
    RF->ANAMISC_CTRL1.BYP_LDOIF          = 0;
    RF->ANAMISC_CTRL1.BYP_LDOPLL         = 0;
    RF->ANAMISC_CTRL1.BYP_LDOTX          = 0;
    RF->ANAMISC_CTRL1.BYP_LDOVCO         = 0;
    RF->ANAMISC_CTRL1.CF_BW08M_ADJ       = 0;
    RF->ANAMISC_CTRL1.TSTEN_CBPF         = 0;//1;// 观测2M_IF
    RF->ANAMISC_CTRL1.TSTEN_RSSI         = 0;
    RF->ANAMISC_CTRL1.AT0_SEL            = 0;
    RF->ANAMISC_CTRL1.AT1_SEL            = 0;
    RF->ANAMISC_CTRL1.VCO_RES            = 0;
    RF->ANAMISC_CTRL1.VCOAFC_SEL         = 0;

    // 0x64, Read Write
    RF->ANA_PWR_CTRL.TEST_EN_LDO_VCO     = 0;//1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_PA      = 0;//1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_IF      = 0;//1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_PLL     = 0;//1;
    RF->ANA_PWR_CTRL.TEST_EN_DAC_DIG_PWR = 0;//1;
    RF->ANA_PWR_CTRL.TEST_EN_AGC_PWR     = 0;//1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_TX      = 0;//1;

    RF->ANA_PWR_CTRL.TX_EN_LDO_VCO       = 1;
    RF->ANA_PWR_CTRL.TX_EN_LDO_PA        = 1;
    RF->ANA_PWR_CTRL.TX_EN_LDO_IF        = 1;
    RF->ANA_PWR_CTRL.TX_EN_LDO_PLL       = 1;
    RF->ANA_PWR_CTRL.TX_EN_DAC_DIG_PWR   = 1;
    RF->ANA_PWR_CTRL.TX_EN_AGC_PWR       = 1;
    RF->ANA_PWR_CTRL.TX_EN_LDO_TX        = 1;

    RF->ANA_PWR_CTRL.RX_EN_LDO_VCO       = 1;
    RF->ANA_PWR_CTRL.RX_EN_LDO_PA        = 1;
    RF->ANA_PWR_CTRL.RX_EN_LDO_IF        = 1;
    RF->ANA_PWR_CTRL.RX_EN_LDO_PLL       = 1;
    RF->ANA_PWR_CTRL.RX_EN_DAC_DIG_PWR   = 1;
    RF->ANA_PWR_CTRL.RX_EN_AGC_PWR       = 1;
    RF->ANA_PWR_CTRL.RX_EN_LDO_TX        = 1;
    
    RF->ANA_PWR_CTRL.CLK_EN_DAC          = 0;
    RF->ANA_PWR_CTRL.CLK_EN_BPF          = 0;
    RF->ANA_PWR_CTRL.CLK_EN_PLL          = 1;
    RF->ANA_PWR_CTRL.CLK_EN_AGC          = 0;

    // 0x68, Read Write
    RF->ANA_EN_CTRL.EN_AGC_REG           = 0;
    RF->ANA_EN_CTRL.EN_BPF_REG           = 0;
    RF->ANA_EN_CTRL.EN_DAC_BLE_REG       = 0;
    RF->ANA_EN_CTRL.EN_LMT_RSSI_REG      = 0;
    RF->ANA_EN_CTRL.EN_LNA_REG           = 0;
    RF->ANA_EN_CTRL.EN_MIXH_REG          = 0;
    RF->ANA_EN_CTRL.EN_MIXL_REG          = 0;
    RF->ANA_EN_CTRL.EN_PA_REG            = 0;
    RF->ANA_EN_CTRL.EN_PLL_REG           = 0;
    RF->ANA_EN_CTRL.EN_RSSI_I_REG        = 0;
    RF->ANA_EN_CTRL.EN_RSSI_Q_REG        = 0;
    RF->ANA_EN_CTRL.EN_FB_DIV_REG        = 0;
    RF->ANA_EN_CTRL.PLL_CALIB_PD_REG     = 0;
    RF->ANA_EN_CTRL.TX_OPEN_PD_REG       = 1; // 1:PLL环路断开, 0:PLL环路在CP处闭合
    RF->ANA_EN_CTRL.BAND_CAL_DONE_REG    = 0;
    RF->ANA_EN_CTRL.GAIN_CAL_DONE_REG    = 0;
    
    RF->ANA_EN_CTRL.EN_BG                = 1;
    RF->ANA_EN_CTRL.EN_LMT_OUTI_EXT      = 1;
    RF->ANA_EN_CTRL.EN_LMT_OUTQ_EXT      = 1;
    RF->ANA_EN_CTRL.EN_LNA_BYPS          = 0;
    RF->ANA_EN_CTRL.EN_DAC_ZB            = 0;
    RF->ANA_EN_CTRL.CF_BW12M_ADJ_REG     = 0;
    RF->ANA_EN_CTRL.ANT_CAP_RX           = 0;
    RF->ANA_EN_CTRL.ANT_CAP_TX           = 0; // bit[29:26]

    // 0x6C, Read Write
    RF->PLL_ANA_CTRL.PLL_BW_ADJ          = 0; // 影响锁频范围 --- 2022.07.05 谢波
    RF->PLL_ANA_CTRL.PLL_CP_OS_EN        = 0;
    RF->PLL_ANA_CTRL.PLL_CP_OS_ADJ       = 0;
    RF->PLL_ANA_CTRL.PLL_DIV_ADJ         = 2;
    RF->PLL_ANA_CTRL.PLL_DI_P            = 0x1F;
    RF->PLL_ANA_CTRL.PLL_FAST_LOCK_EN    = 1;
    RF->PLL_ANA_CTRL.PLL_FBDIV_RST_EXT   = 0;
    RF->PLL_ANA_CTRL.PLL_FBDIV_RST_SEL   = 0;
    RF->PLL_ANA_CTRL.PLL_LOCK_BYPS       = 0;
    RF->PLL_ANA_CTRL.PLL_PS_CNT_RST_SEL  = 0;
    RF->PLL_ANA_CTRL.PLL_REF_SEL         = 0;
    RF->PLL_ANA_CTRL.PLL_SDM_TEST_EN     = 0;
    RF->PLL_ANA_CTRL.PLL_VCO_ADJ         = 6;
    RF->PLL_ANA_CTRL.PLL_VCTRL_EXT_EN    = 0;
    RF->PLL_ANA_CTRL.PLL_VREF_ADJ        = 4;
    RF->PLL_ANA_CTRL.PLL_MIX_SEL         = 1; // in Rx --- 0:-2M, 1:+2M
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG     = 1; // 1 tx, 0 rx
    RF->PLL_ANA_CTRL.PLL_SEL_RTX_BW      = 0;
    
    // 0x70, Read Write
    // bit[14:11] = 2, bit15:CLK_128M_DAC enable
    RF->RF_RSV = ((2UL << 11) | (1UL << 15));
    
    // 0x74, Read Only
//    RF->RF_ANA_ST0.AGC_STATE0
//    RF->RF_ANA_ST0.AGC_STATE1
//    RF->RF_ANA_ST0.BPF_CAL_CODE
//    RF->RF_ANA_ST0.BG_VREF_OK12
//    RF->RF_ANA_ST0.BPF_CAL_DONE
//    RF->RF_ANA_ST0.PLL_LOCK
//    RF->RF_ANA_ST0.FAST_LOCK_DONE
//    RF->RF_ANA_ST0.AGC_STATE_TEST
}

uint16_t rf_tx_gain_cal(uint8_t pll_vref_sel, uint8_t pll_vtxd_ext)
{
//    RF->DIG_CTRL.PLL_CAL_TEST_EN        = 1;
    RF->ANA_EN_CTRL.GAIN_CAL_DONE_REG   = 0;
//    RF->DIG_CTRL.PLL_AFC_BY           = 1;
//    RF->DIG_CTRL.SW_AFC_EN            = 0;
    RF->ANA_EN_CTRL.BAND_CAL_DONE_REG = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN         = 0;
    RF->DIG_CTRL.SW_GAIN_CAL_EN       = 0;
    
    RF->DIG_CTRL.SW_PLL_VREF_SEL      = pll_vref_sel;
    RF->PLL_GAIN_CTRL.PLL_VTXD_EXT    = pll_vtxd_ext;
    RF->PLL_DYM_CTRL.SW_TX_EN         = 1;
//    RF->DIG_CTRL.SW_AFC_EN            = 1;
//    RF->ANA_EN_CTRL.BAND_CAL_DONE_REG = 1;
    
    // delay,wait pll lock
//    for (uint8_t i = 0; i < 40; i++)
//    {
//        __NOP(); __NOP();
//        __NOP(); __NOP();
//    }
    
    RF->DIG_CTRL.SW_GAIN_CAL_EN       = 1;
//    RF->ANA_EN_CTRL.GAIN_CAL_DONE_REG = 1;
    
    while (!(RF->PLL_CAL_ST.GAIN_CALIB_DONE));
    
    RF->DIG_CTRL.SW_GAIN_CAL_EN    = 0;
    
    return (RF->PLL_CAL_ST.CALIB_CNT_RPT);
//    return (RF->PLL_CAL_ST.PLL_GAIN_CAL_ST);
}

uint8_t get_pll_freq_adj_st(uint8_t chan_num)
{
    
    RF->PLL_ANA_CTRL.PLL_VCTRL_EXT_EN    = 0;
    
    RF->PLL_DYM_CTRL.SW_TX_EN            = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN            = 0;
    RF->PLL_DYM_CTRL.SW_CH_NUM           = chan_num;
    RF->PLL_DYM_CTRL.SW_TX_EN            = 1;
    
    while (!(RF->PLL_CAL_ST.AFC_CALIB_DONE));

    return (RF->PLL_CAL_ST.PLL_FREQ_ADJ_ST);
}

void rx_2m_if(void)
{
    // at0, at1 select CBPF's Output
    REG_AT0_SEL(AT0_SEL_BPF_IOP);
    REG_AT1_SEL(AT1_SEL_BPF_ION);
    
    RF->ANAMISC_CTRL1.TSTEN_CBPF         = 1;// 观测2M_IF
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG     = 0;
    RF->DIG_CTRL.PLL_FREQ_EXT_EN         = 0;
    
    RF->PLL_DYM_CTRL.SW_TX_EN            = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN            = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC        = 0;  
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S        = 0x05; //0x05:2144MHz, 0x15:2400MHz, 0x18:2448MHz
    RF->PLL_DYM_CTRL.SW_RX_EN            = 1;

    RF->ANA_EN_CTRL.EN_PA_REG            = 0;
    RF->ANA_EN_CTRL.EN_MIXH_REG          = 1;
    RF->ANA_EN_CTRL.EN_MIXL_REG          = 1;
    RF->ANA_EN_CTRL.EN_LNA_REG           = 1;
    RF->ANA_EN_CTRL.EN_BPF_REG           = 1;

    RF->ANA_EN_CTRL.EN_LNA_BYPS          = 0;  // bit19
   
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT     = 0x1A;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT_EN  = 1;
}

void vco_freq_test_cmd(uint8_t freq_adj)
{
    pll_vctrl_at0_cmd();
    
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 1;
    RF->ANA_EN_CTRL.EN_PA_REG         = 1;
    
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN         = 0;
    RF->PLL_ANA_CTRL.PLL_VCTRL_EXT_EN = 0;
    
    RF->DIG_CTRL.PLL_FREQ_EXT_EN      = 0;
    RF->PLL_TAB_OFFSET.PLL_FREQ_EXT   = (freq_adj & 0x3F); // bit[26:21]
    RF->DIG_CTRL.PLL_FREQ_EXT_EN      = 1; // soft cfg en
}

void pll_vtxd_ext_test_cmd(uint8_t vtxd_ext)
{
    RF->ANA_EN_CTRL.EN_PA_REG         = 1;
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 1;
    RF->DIG_CTRL.PLL_FREQ_EXT_EN      = 1;

    RF->DIG_CTRL.PLL_VTXD_EXT_EN      = 0;
    RF->PLL_GAIN_CTRL.PLL_VTXD_EXT    = (vtxd_ext & 0x1F);
    RF->DIG_CTRL.PLL_VTXD_EXT_EN      = 1; // soft cfg en
}

// pll_dis 0 ~ 31, pll_frach 0 ~ 15
void pll_tx_1m_lock_test_cmd(uint8_t pll_dis, uint8_t pll_frach)
{
    RF->DIG_CTRL.PLL_AFC_BY           = 0;
    RF->ANA_EN_CTRL.EN_PA_REG         = 1;
    RF->PLL_TAB_OFFSET.TX_FRAC_OFFSET = 0;
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 1;
    
    RF->DIG_CTRL.PLL_FREQ_EXT_EN      = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN         = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S     = (pll_dis & 0x1F);
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC     = (uint32_t)((pll_frach & 0x0F) << 20);
    RF->PLL_DYM_CTRL.SW_TX_EN         = 1; // soft cfg en
}

uint32_t g_rx_pll_frach[18] = 
{
    0,        0xE38E3,  0x1C71C7, 0x2AAAAA, 0x38E38E, 0x471C71,
    0x555555, 0x638E38, 0x71C71C, 0x800000, 0x8E38E3, 0x9C71C7,
    0xAAAAAA, 0xB8E38E, 0xC71C71, 0xD55555, 0xE38E38, 0xF1C71C
};

// pll_dis 0 ~ 31, pll_frach 0 ~ 17
void pll_rx_1m_lock_test_cmd(uint8_t pll_dis, uint8_t pll_frach)
{
    RF->PLL_TAB_OFFSET.RX_FRAC_OFFSET = 0;
    RF->PLL_ANA_CTRL.PLL_MIX_SEL      = 1; // in Rx --- 0:-2M, 1:+2M
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 0; // 选择RX模式
    RF->ANA_EN_CTRL.EN_PA_REG         = 0; // 关闭PA
    
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN         = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S     = (pll_dis & 0x1F);
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC     = g_rx_pll_frach[pll_frach];
    RF->PLL_DYM_CTRL.SW_RX_EN         = 1; // soft cfg en
}

void rx_lna_test_cmd(struct rx_lna *param)
{
    // at0, at1 select CBPF's Output
    REG_AT0_SEL(AT0_SEL_BPF_IOP);
    REG_AT1_SEL(AT1_SEL_BPF_ION);
    
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG     = 0;
    RF->DIG_CTRL.PLL_FREQ_EXT_EN         = 0;

    RF->PLL_DYM_CTRL.SW_RX_EN            = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC        = 0;  
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S        = 0x05; //0x05:2144MHz, 0x15:2400MHz, 0x18:2448MHz
    RF->PLL_DYM_CTRL.SW_RX_EN            = 1;

    RF->ANA_EN_CTRL.EN_PA_REG            = 0;
    RF->ANA_EN_CTRL.EN_MIXH_REG          = 0;
    RF->ANA_EN_CTRL.EN_MIXL_REG          = 0;
    RF->ANA_EN_CTRL.EN_LNA_REG           = 0;
    RF->ANA_EN_CTRL.EN_BPF_REG           = 0;

    RF->ANA_EN_CTRL.EN_LNA_BYPS          = 0;  // bit19
   
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT     = 0x1A;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT_EN  = 1;
    
    ///*******************/    
    RF->ANA_EN_CTRL.ANT_CAP_RX           = (param->ant_cap_rx & 0x0F); // bit[25:22]
    RF->RF_RSV                           = ((param->rf_rsv & 0x0F) << 11); // bit[14:11]
    RF->ANA_TRIM.LNA_GAIN                = (param->lna_gain & 0x0F); // bit[26:23]
    RF->BPFMIX_CTRL.BPF_IADJ             = (param->bpf_iadj & 0x07); // bit[14:12]
    RF->ANA_TRIM.BPF_GAIN_ADJ            = (param->bpf_gain_adj & 0x03); // bit[20:19]
    
    RF->BPFMIX_CTRL.MIXH_ENB_CAP         = (param->mixh_enb_cap & 0x01);  // bit20
    
    RF->BPFMIX_CTRL.MIXL_BIAS_CTL        = (param->mixl_bias_ctl & 0x07); // bit[23:21]
    RF->BPFMIX_CTRL.MIXH_BIAS_CTL        = (param->mixh_bias_ctl & 0x07); // bit[18:16]
    
    RF->ANA_TRIM.MIXL_GAIN_CTL           = (param->mixl_gain_ctl & 0x01);  // bit21
    RF->ANA_TRIM.MIXH_GAIN_CTL           = (param->mixh_gain_ctl & 0x01);  // bit22
}

uint8_t cbpf_cal_test_cmd(uint8_t cent_adj)
{
     // at0, at1 select CBPF's Output
    REG_AT0_SEL(AT0_SEL_BPF_IOP);
    REG_AT1_SEL(AT1_SEL_BPF_ION);

    RF->ANA_EN_CTRL.EN_PA_REG           = 0;
    RF->ANA_EN_CTRL.EN_MIXH_REG         = 1;
    RF->ANA_EN_CTRL.EN_MIXL_REG         = 1;
    RF->ANA_EN_CTRL.EN_LNA_REG          = 1;
    RF->ANA_EN_CTRL.EN_LNA_BYPS         = 0;
    RF->ANA_EN_CTRL.CF_BW12M_ADJ_REG    = 0;

    RF->BPFMIX_CTRL.BPF_MODE_SEL        = 0;
    RF->BPFMIX_CTRL.MIXH_ENB_CAP        = 0;
    RF->BPFMIX_CTRL.MIXH_BIAS_CTL       = 3;
    RF->BPFMIX_CTRL.MIXL_BIAS_CTL       = 3;
    RF->BPFMIX_CTRL.BPF_BW_ADJ          = 1;
    RF->BPFMIX_CTRL.BPF_IADJ            = 4;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT_EN = 0;

    RF->ANA_TRIM.LNA_GAIN               = 0;
    RF->ANA_TRIM.MIXL_GAIN_CTL          = 0;
    RF->ANA_TRIM.MIXH_GAIN_CTL          = 0;

    RF->ANAMISC_CTRL1.CF_BW08M_ADJ      = 0;
    
    pll_rx_1m_lock_test_cmd(5, 0);// Rx--2144MHz
    
/**********************SCAN BPF_CENT_ADJ<1:0>***************/
    uint8_t bpf_cal_results = 0;

    RF->ANA_EN_CTRL.EN_BPF_REG          = 1; // soft cfg en
    RF->BPFMIX_CTRL.BPF_CENT_ADJ        = (cent_adj & 0x03);
    RF->ANA_EN_CTRL.EN_BPF_REG          = 0;  

    // wait BPF_CAL_DONE is 1
    while (! RF->RF_ANA_ST0.BPF_CAL_DONE);
    
    bpf_cal_results = RF->RF_ANA_ST0.BPF_CAL_CODE;
    
    return bpf_cal_results;
}

uint8_t tx_gain_cal(uint8_t pll_dis, uint8_t pll_frach)
{
    uint8_t gain_result = 0;
    RF->DIG_CTRL.PLL_GAIN_CAL_BY        = 1;
    
    RF->ANA_EN_CTRL.EN_PA_REG           = 1;
    RF->PLL_TAB_OFFSET.TX_FRAC_OFFSET   = 0;
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG    = 1;
    
    RF->PLL_DYM_CTRL.SW_RX_EN           = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN           = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S       = (pll_dis & 0x1F);
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC       = (uint32_t)((pll_frach & 0x0F) << 20);
    RF->PLL_DYM_CTRL.SW_TX_EN           = 1;
    
    // gain_cal start by fsm
    RF->DIG_CTRL.PLL_GAIN_CAL_BY        = 0;
    
    // wait GAIN_CALIB_DONE is 1
    while (! RF->PLL_CAL_ST.GAIN_CALIB_DONE);
    
    gain_result = RF->PLL_CAL_ST.PLL_GAIN_CAL_ST;
    
    return gain_result;
}

static void ldo_rx_trim_cmd(uint8_t ldo_rx_trim)
{
    RF->PLL_TAB_OFFSET.RX_FRAC_OFFSET = 0;
    RF->PLL_ANA_CTRL.PLL_MIX_SEL      = 1; // in Rx --- 0:-2M, 1:+2M
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 0;
    
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN         = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S     = 5; // Rx:2414MHz
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC     = 0;
    
    RF->PLL_DYM_CTRL.SW_RX_EN         = 1;
    
    RF->ANA_TRIM.LDO_RX_TRIM = (ldo_rx_trim & 0x07);
}

static void ldo_tx_trim_cmd(uint8_t ldo_tx_trim)
{
    RF->ANA_EN_CTRL.EN_PA_REG         = 1;
    RF->PLL_TAB_OFFSET.TX_FRAC_OFFSET = 0;
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG  = 1;
    
    RF->PLL_DYM_CTRL.SW_RX_EN         = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN         = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S     = 0x15; // Tx:2400MHz
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC     = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN         = 1;
    
    RF->ANA_TRIM.LDO_TX_TRIM = (ldo_tx_trim & 0x07);
}

void pll_vctrl_at0_cmd(void)
{
    // PLL_VCTRL at0_6
    REG_AT0_SEL(AT0_SEL_PLL_VCTRL);
    
    RF->PLL_ANA_CTRL.PLL_VCTRL_EXT_EN    = 1;
}

static void vdd_if_at0_cmd(uint8_t ldo_rx_trim)
{
    // vdd if at0_0
    REG_AT0_SEL(AT0_SEL_VDD_IF);
    
    ldo_rx_trim_cmd(ldo_rx_trim);
}

static void vdd_pll_at0_cmd(uint8_t ldo_rx_trim)
{
    // vdd pll at0_5
    REG_AT0_SEL(AT0_SEL_VDD_PLL);
    
    ldo_rx_trim_cmd(ldo_rx_trim);
}

static void vdd_rx_at1_cmd(uint8_t ldo_rx_trim)
{
    // vdd rx at1_5
    REG_AT1_SEL(AT1_SEL_VDD_RX);
    
    ldo_rx_trim_cmd(ldo_rx_trim);
}

static void vdd_tx_at1_cmd(uint8_t ldo_tx_trim)
{
    // vdd tx at1_0
    REG_AT1_SEL(AT1_SEL_VDD_TX);
    
    ldo_tx_trim_cmd(ldo_tx_trim);
}

static void vdd_vco_at1_cmd(uint8_t ldo_tx_trim)
{
    // vdd tx at1_6
    REG_AT1_SEL(AT1_SEL_VDD_VCO);
    
    ldo_tx_trim_cmd(ldo_tx_trim);
}

// ok
// test_case @see enum ldo_vdd_case
// trim 0x00 ~ 0x07
void ldo_vdd_test_cmd(uint8_t test_case, uint8_t trim)
{
    switch (test_case)
    {
        case LDO_VDD_IF:
        {
            at1_hiz_set();
            vdd_if_at0_cmd(trim);
        } break;
            
        case LDO_VDD_RX:
        {
            at0_hiz_set();
            vdd_rx_at1_cmd(trim);
        } break;
        
        case LDO_VDD_PLL:
        {
            at1_hiz_set();
            vdd_pll_at0_cmd(trim);
        } break;

        case LDO_VDD_VCO:
        {
            at0_hiz_set();
            vdd_vco_at1_cmd(trim);
        } break;
        
        case LDO_VDD_TX:
        {
            at0_hiz_set();
            vdd_tx_at1_cmd(trim);
        } break;

        default:
            break;
    }
}

// ok
void ldo_vdd_pa(uint8_t pa_gain)
{
    RF->DIG_CTRL.PA_GAIN_TARGET_SEL    = 1;
    
    RF->PLL_DYM_CTRL.SW_PA_GAIN_TARGET = (pa_gain & 0x0F); // bit[13:10]
}

// ok
// rf_bandgap
// bias_trim:0~3, vref_fine:0~7, res_trim:0~0x1F
void rf_bandgap_test_cmd(uint8_t bias_trim, uint8_t vref_fine, uint8_t res_trim)
{
    // 测试VDD_IF的电压, ldo_if_trim=000，也是vbg12的输出
    static uint8_t init_flag = 0;
    
    if (init_flag == 0)
    {
        vdd_if_at0_cmd(0);
        init_flag = 1;
    }
    
    RF->ANA_TRIM.BG_BIAS_TRIM  = (bias_trim & 0x03); // bit[1:0]
    RF->ANA_TRIM.BG_VREF_FINE  = (vref_fine & 0x07); // bit[9:7]
    RF->ANA_TRIM.BG_RES_TRIM   = (res_trim  & 0x1F); // bit[6:2]
}

// ok
void tx_pa(uint8_t ant_cap_tx, uint8_t pa_cap, uint8_t pa_gain)
{
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG   = 1;
    
    RF->PLL_DYM_CTRL.SW_RX_EN          = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN          = 0;
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S      = 0x15; // Tx:2400MHz
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC      = 0;
    RF->PLL_DYM_CTRL.SW_TX_EN          = 1; // fsm start
    
    RF->DIG_CTRL.PA_GAIN_TARGET_SEL    = 1;
    RF->PLL_DYM_CTRL.SW_PA_GAIN_TARGET = (pa_gain & 0x0F);    // bit[13:10]
    RF->BPFMIX_CTRL.PA_CAP             = (pa_cap & 0x0F);     // bit[28:25] --- 调匹配电容
    RF->ANA_EN_CTRL.ANT_CAP_TX         = (ant_cap_tx & 0x0F); // bit[29:26]
}

// ok
uint8_t dpll_384m_lock(uint8_t dpll_48m)
{
    uint8_t lock_ready = 0;
    
    APBMISC->DPLL_CTRL.DPLL2_EN = 1;
    
    // 0:64M, 1:48M
    RCC->CFG.DPLL_CLK_SW = dpll_48m; 
    
    lock_ready = APBMISC->ANAMISC_ST.DPLL2_LOCK_READY;
    
    return lock_ready;
}

// ok
void xo16m_test_cmd(uint8_t xosc16_lp, uint8_t bias_adj, uint8_t cap_trim, uint8_t ldo_xo_tr)
{
    if (CLK_OUT_XO16M != RCC->CFG.MCO_SW)
    {
        iospc_clkout(CLK_OUT_XO16M);
    }
    
    APBMISC->XOSC16M_CTRL.XOSC16M_LP       = (xosc16_lp & 0x01); // bit6
    APBMISC->XOSC16M_CTRL.LDO_XOSC_TR      = (ldo_xo_tr & 0x0F); // bit[17:14]
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR   = (cap_trim & 0x3F);  // bit[5:0]
    APBMISC->XOSC16M_CTRL.XOSC16M_BIAS_ADJ = (bias_adj & 0x0F);  // bit[11:8]
}

// ok
void rc16m_test_cmd(uint8_t rc16m_freq_trim)
{
    if (CLK_OUT_HSI != RCC->CFG.MCO_SW)
    {
        iospc_clkout(CLK_OUT_HSI);
    }
    
    APBMISC->RC16M_FREQ_TRIM = rc16m_freq_trim;
}

/****************************************************/
volatile uint8_t g_sadc_init_flag = 0;

#define RF_RSV_ADC_TEMP_BIT   (0x01UL)
#define RF_RSV_ADC_1P2V_BIT   (0x02UL)
#define RF_RSV_ADC_RSSI_BIT   (0x04UL)
#define RF_RSV_ADC_VDD_IF_BIT (0x08UL)
#define RF_RSV_ADC_MSK        (0x0FUL)

#define ADC_INIT_FLAG_TEMP    (0x01UL)
#define ADC_INIT_FLAG_1P2V    (0x02UL)
#define ADC_INIT_FLAG_RSSI    (0x04UL)
#define ADC_INIT_FLAG_VDD_IF  (0x08UL)

#define SADC_CNT 16
//RF Temperature, SADC普通模式, 通道15
static void sadc_calibration(void)
{
    // enable ADC
//    SADC->SADC_ANA_CTRL.SADC_EN   = 1;
    
    // clear flag
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    // enable calibration mode
    SADC->CTRL.SADC_CALIB_MODE    = 1;
    
    // start calibration
    SADC->CTRL.SADC_SOC           = 1;
    
    // wait calibration done is 1
    while (! SADC->STCTRL.SADC_AUX_FLG);
    
    // clear flag
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    for (uint8_t i = 0; i < 8; i++)
    {
        SADC->SADC_CALIB_DATSEL = i;
    }
    
    // disable calibration mode
    SADC->CTRL.SADC_CALIB_MODE    = 0;
}

void sadc_init(void)
{
    // Clock
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 0;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 1;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 0;
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 1;
    
    // enable ADC
    SADC->SADC_ANA_CTRL.SADC_EN          = 1;
    SADC->SADC_ANA_CTRL.SADC_VREF_TRIM   = 0; // 1.2V
    SADC->SADC_ANA_CTRL.SADC_IBSEL_CMP   = 3;
    SADC->SADC_ANA_CTRL.SADC_IBSEL_VCM   = 3;
    SADC->SADC_ANA_CTRL.SADC_IBSEL_VREF  = 3;
    SADC->SADC_ANA_CTRL.SADC_IBSEL_BUF   = 3;
    SADC->SADC_ANA_CTRL.SADC_VREF_SEL    = 1;//1-VBG, 0-VDD
    SADC->SADC_ANA_CTRL.SADC_CALCAP_SEL  = 1;
    SADC->SADC_ANA_CTRL.SADC_INBUF_BYPSS = 1;
    
    SADC->CTRL.SADC_CLK_PH               = 1; // conversion at posedge of clk
    SADC->CTRL.SADC_AUX_CLK_DIV          = 159; // rssi must 16M.
    
    sadc_calibration();

    SADC->CTRL.SADC_CONV_MODE            =  0; // single sample
    SADC->CTRL.SADC_SAMP_MOD             =  0; // software mode
    SADC->CH_CTRL.SADC_CH_SET0           = 15; // RF temperature & rssi
    
    for (uint32_t i = 0; i < 200000; i++)
    {
        __NOP();
    }
    
    g_sadc_init_flag = 0;
}

uint16_t sadc_temperature_get(void)
{
    uint16_t sadc_val = 0;
    
    // RF_RSV<0> 温度
    if ((RF->RF_RSV & RF_RSV_ADC_TEMP_BIT) != RF_RSV_ADC_TEMP_BIT)
    {
        RF->RF_RSV &= ~RF_RSV_ADC_MSK;
        RF->RF_RSV |= RF_RSV_ADC_TEMP_BIT;
    }
    
    if ((g_sadc_init_flag & ADC_INIT_FLAG_TEMP) != ADC_INIT_FLAG_TEMP)
    {
        sadc_init();

        g_sadc_init_flag |= ADC_INIT_FLAG_TEMP;
    }
    
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
//    for (uint8_t cnt = 0; cnt < SADC_CNT; cnt++)
    {
        // start conversion
        SADC->CTRL.SADC_SOC           = 1;
        
        // wait conversion done is 1
        while (! (SADC->STCTRL.SADC_AUX_FLG));
        
        // clear flag
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
        
        sadc_val += ((SADC->AUX_ST.Word) & 0x03FF);
    }
    
    // average
//    sadc_val /= SADC_CNT;
    
    return sadc_val;
}

#define RF_RSSI 1
static void rssi_init(void)
{
    // RF_RSV<2> rssi
    if ((RF->RF_RSV & RF_RSV_ADC_RSSI_BIT) != RF_RSV_ADC_RSSI_BIT)
    {
        RF->RF_RSV &= ~RF_RSV_ADC_MSK;
        RF->RF_RSV |= RF_RSV_ADC_RSSI_BIT;
    }

    sadc_init();
    SADC->CTRL.SADC_AUX_CLK_DIV          = 0; // rssi must 16M.
    #if (RF_RSSI)
    SADC->CTRL.SADC_SAMP_MOD             = 1; // RSSI sample mode
    #endif
//    SADC->CH_CTRL.SADC_CH_SET0           = 15;

    if (AT1_SEL_RSSI_OUT != RF->ANAMISC_CTRL1.AT1_SEL)
    {
        REG_AT1_SEL(AT1_SEL_RSSI_OUT);
    }
    
    RF->ANAMISC_CTRL1.TSTEN_RSSI         = 1;
    RF->ANA_EN_CTRL.EN_RSSI_I_REG        = 1;
    RF->ANA_EN_CTRL.EN_RSSI_Q_REG        = 1;
    
    pll_rx_1m_lock_test_cmd(5, 0);// Rx--2144MHz
    
    RF->ANA_TRIM.LDO_RX_TRIM = 0;
}

volatile bool g_rssi_init_flag = false;
uint32_t rssi_get(void)
{
    if ((g_sadc_init_flag & ADC_INIT_FLAG_RSSI) != ADC_INIT_FLAG_RSSI)
    {
        rssi_init();
//        sadc_init();

        g_sadc_init_flag |= ADC_INIT_FLAG_RSSI;
    }
    
    #if (RF_RSSI)
    RF->RSSI_CTRL.SW_RSSI_REQ = 1;
    
    // wait rssi_ready is 1
    while (! RF->RSSI_CTRL.SW_RSSI_READY);
    
    RF->RSSI_CTRL.SW_RSSI_REQ = 0;
    
    uint32_t val = (((SADC->AUX_ST.Word) & 0x03FF) | (RF->RSSI_CTRL.RSSI_VAL << 16));
    
    return val;
    #else
    // start conversion
    SADC->CTRL.SADC_SOC           = 1;
    
    // wait conversion done is 1
    while (! (SADC->STCTRL.SADC_AUX_FLG));
    
    // clear flag
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    return ((uint16_t)((SADC->AUX_ST.Word) & 0x03FF));
    #endif
}

uint16_t sadc_rf_bandgap(uint8_t bias_trim, uint8_t vref_fine, uint8_t res_trim)
{
    uint16_t sadc_val = 0;
    // RF_RSV<1> rf bandgap
    if ((RF->RF_RSV & RF_RSV_ADC_1P2V_BIT) != RF_RSV_ADC_1P2V_BIT)
    {
        RF->RF_RSV &= ~RF_RSV_ADC_MSK;
        RF->RF_RSV |= RF_RSV_ADC_1P2V_BIT;
    }
    
    if ((g_sadc_init_flag & ADC_INIT_FLAG_1P2V) != ADC_INIT_FLAG_1P2V)
    {
        sadc_init();

        g_sadc_init_flag |= ADC_INIT_FLAG_1P2V;
    }
    
    // 测试VDD_IF的电压，ldo_if_trim=000，也是vbg12的输出
    static uint8_t init_flag = 0;
    
    if (init_flag == 0)
    {
        vdd_if_at0_cmd(0);
        init_flag = 1;
    }
    
    RF->ANA_TRIM.BG_BIAS_TRIM  = (bias_trim & 0x03); // bit[1:0]
    RF->ANA_TRIM.BG_RES_TRIM   = (res_trim  & 0x1F); // bit[6:2]
    RF->ANA_TRIM.BG_VREF_FINE  = (vref_fine & 0x07); // bit[9:7]
    
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    for (uint8_t cnt = 0; cnt < SADC_CNT; cnt++)
    {
        // start conversion
        SADC->CTRL.SADC_SOC           = 1;
        
        // wait conversion done is 1
        while (! (SADC->STCTRL.SADC_AUX_FLG));
        
        // clear flag
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
        
        sadc_val += ((SADC->AUX_ST.Word) & 0x03FF);
    }
    
    // average
    sadc_val /= SADC_CNT;
    
    return sadc_val;
}

uint16_t sadc_vdd_if(uint8_t ldo_rx_trim)
{
    uint16_t sadc_val = 0;
    
    // RF_RSV<3> vdd_if
    if ((RF->RF_RSV & RF_RSV_ADC_VDD_IF_BIT) != RF_RSV_ADC_VDD_IF_BIT)
    {
        RF->RF_RSV &= ~RF_RSV_ADC_MSK;
        RF->RF_RSV |= RF_RSV_ADC_VDD_IF_BIT;
    }
    
    if ((g_sadc_init_flag & ADC_INIT_FLAG_VDD_IF) != ADC_INIT_FLAG_VDD_IF)
    {
        // vdd if at0_0
        REG_AT0_SEL(AT0_SEL_VDD_IF);
        
        sadc_init();

        g_sadc_init_flag |= ADC_INIT_FLAG_VDD_IF;
    }

    ldo_rx_trim_cmd(ldo_rx_trim);
    
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    
    for (uint8_t cnt = 0; cnt < SADC_CNT; cnt++)
    {
        // start conversion
        SADC->CTRL.SADC_SOC           = 1;
        
        // wait conversion done is 1
        while (! (SADC->STCTRL.SADC_AUX_FLG));
        
        // clear flag
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
        
        sadc_val += ((SADC->AUX_ST.Word) & 0x03FF);
    }
    
    // average
    sadc_val /= SADC_CNT;
    
    return sadc_val;
}

uint32_t random_number_get(void)
{
    uint32_t random_num = 0;
    
    if ((RF->RF_RSV & RF_RSV_ADC_TEMP_BIT) != RF_RSV_ADC_TEMP_BIT)
    {
        RF->RF_RSV &= ~(0x0F << 0);
        RF->RF_RSV |= RF_RSV_ADC_TEMP_BIT;
    }

    if ((g_sadc_init_flag & 0x10) != 0x10)
    {
        sadc_init();

        for (uint8_t i = 0; i < 8; i++)
        {
            SADC->SADC_CALIB_DATSEL = i;
            SADC->SADC_CALIB_DATIN  = 0xFF;
        }
        
        g_sadc_init_flag |= 0x10;
    }

    for (uint8_t cnt = 0; cnt < 32; cnt++)
    {
        AON->BKHOLD_CTRL.CORELDO_TRIM_RUN    = cnt;
        SADC->CTRL.SADC_AUX_CLK_DIV          = cnt;
        SADC->SADC_ANA_CTRL.SADC_IBSEL_CMP   = cnt;
        SADC->SADC_ANA_CTRL.SADC_IBSEL_VCM   = cnt;
        SADC->SADC_ANA_CTRL.SADC_IBSEL_VREF  = cnt;
        SADC->SADC_ANA_CTRL.SADC_IBSEL_BUF   = cnt;
        SADC->SADC_ANA_CTRL.SADC_CALCAP_SEL  = cnt;
        
        // start conversion
        SADC->CTRL.SADC_SOC           = 1;        
        // wait conversion done is 1
        while (! (SADC->STCTRL.SADC_AUX_FLG));
        // clear flag
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
        
        random_num |= ((((SADC->AUX_ST.Word) & 0x01)) << (cnt));
    }

    return (random_num);
}
