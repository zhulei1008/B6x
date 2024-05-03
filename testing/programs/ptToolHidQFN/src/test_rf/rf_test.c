#include "regs.h"
#include "drvs.h"
#include "reg_blecore.h"
#include "reg_em_ble_cs.h"
#include "reg_em_ble_rx_desc.h"
#include "rf_test.h"
#include "rf_utils.h"
#include "sysdbg.h"

#if !defined(RF_SYNC_WORD)
#define RF_SYNC_WORD       (0x71764129)
#endif

#if !(RF_PKT_CNT)
#define RF_PKT_CNT       100
#endif
#define USE_DAC_TAB      1
#define SW_PA_TARGET     0x00

#define RF_CAL_VALID     (0x20230630)

uint32_t g_rx_cnt = 0, g_rx_err = 0;

/*
 * RF drivers
 ****************************************************************************************
 */

volatile uint8_t g_init_flag = 0;

void emi_init(void)
{
    uint16_t i;

    for ( i = 0; i < 2048; i++)
    {
        (*(volatile uint32_t *)(EM_BASE_ADDR + i * 4)) = 0x0000;
    }
}

///void radiocntl_config(void)
static void rf_core_init(void)
{
    /* BLE RADIOCNTL0 */
    ble_radiocntl0_pack(
            /*uint16_t spiptr*/   EM_RF_SW_SPI_OFFSET >> 2,
            /*uint8_t  spicfg*/   0,
            /*uint8_t  spifreq*/  0,
            /*uint8_t  spigo*/    0);

    /* BLE RADIOCNTL1 */
    ble_radiocntl1_pack(
            /*uint8_t  forceagcen*/      0,
            /*uint8_t  forceiq*/         0,
            /*uint8_t  rxdnsl*/          0,
            /*uint8_t  txdnsl*/          0,
            /*uint16_t forceagclength*/  0,
            /*uint8_t  syncpulsemode*/   0,
            /*uint8_t  syncpulsesrc*/    1,
            /*uint8_t  dpcorren*/        0,
            /*uint8_t  jefselect*/       1,
    #if (FPGA_TEST)
            /*uint8_t  xrfsel*/          1,
    #else
            /*uint8_t  xrfsel*/          2,
    #endif
            /*uint8_t  subversion*/      0); //0x00005020

    /* BLE RADIOCNTL2 */
    ble_radiocntl2_pack(
            /*uint8_t  lrsynccompmode*/ 0,//0x3,
            /*uint8_t  rxcitermbypass*/ 0x0,
            /*uint8_t  lrvtbflush*/     0,//8,
            /*uint8_t  phymsk*/         1,//3,
            /*uint8_t  lrsyncerr*/      0,
            /*uint8_t  syncerr*/        0,
            /*uint16_t freqtableptr*/   EM_FT_OFFSET >> 2); //0xC8C00100

    /* BLE RADIOCNTL3 */
    ble_radiocntl3_pack(
            /*uint8_t rxrate3cfg*/    0x3,
            /*uint8_t rxrate2cfg*/    0x2,
            /*uint8_t rxrate1cfg*/    0x1,
            /*uint8_t rxrate0cfg*/    0x0,
            /*uint8_t rxsyncrouting*/ 0x0,
            /*uint8_t rxvalidbeh*/    0x0,
            /*uint8_t txrate3cfg*/    0x3,
            /*uint8_t txrate2cfg*/    0x2,
            /*uint8_t txrate1cfg*/    0x1,
            /*uint8_t txrate0cfg*/    0x0,
            /*uint8_t txvalidbeh*/    0x0); //0xE400E400
            
    ble_radiocntl2_phymsk_setf(1); //enable 2Mbps/500Kpbs/125Kbps

    /* BLE RADIOPWRUPDN0: 1Mbps */
    ble_radiopwrupdn0_pack(
            /*uint8_t syncposition0*/ 0,
            /*uint8_t rxpwrup0*/      0x5a,
            /*uint8_t txpwrdn0*/      0x06,
            /*uint8_t txpwrup0*/      0x35); //QFN32 --- 2023.07.08


    /* BLE RADIOPWRUPDN1: 2Mbps */
    ble_radiopwrupdn1_pack(
            /*uint8_t syncposition1*/ 0,
            /*uint8_t rxpwrup1*/      0x50,
            /*uint8_t txpwrdn1*/      0x06,
            /*uint8_t txpwrup1*/      0x5A);//0x50);

    /* BLE RADIOTXRXTIM0 - WR_32(0x50000090, 0x0010130B); */
    ble_radiotxrxtim0_pack(
        /*uint8_t rfrxtmda0*/        7,//0x03,
        /*uint8_t rxpathdly0*/       0x0a,
        /*uint8_t txpathdly0*/      0x10);

    /* BLE RADIOTXRXTIM1 - WR_32(0x50000094, 0x0003070A); */
    ble_radiotxrxtim1_pack(
        /*uint8_t rfrxtmda1*/        0x03,
        /*uint8_t rxpathdly1*/       0x03,
        /*uint8_t txpathdly1*/       0x07);
}

struct rf_cal_env_tag
{
    uint32_t bpf_ctrl;
    uint32_t dac_tab[3];
    uint32_t cal_valid;
} rf_cal_env;

uint8_t cbpf_cal(void)
{
    uint8_t cbpf_code = 0;
    
    if (RF_CAL_VALID == rf_cal_env.cal_valid)
    {
        RF->BPFMIX_CTRL.Word = rf_cal_env.bpf_ctrl;
    }
    else
    {
        RF->BPFMIX_CTRL.Word  = 0x00437C90;
        RF->DIG_CTRL.Word     = 0x0022921A;
        
        // Rx 2440M
        RF->PLL_DYM_CTRL.Word = ((17 << RF_SW_CH_NUM_LSB) | (1 << RF_SW_RX_EN_POS));
        
        // wait BPF_CAL_DONE is 1
        while (! RF->RF_ANA_ST0.BPF_CAL_DONE);
        for (uint16_t i = 0; i < 300; ++i)
        {
            __nop();__nop();
            __nop();__nop();
        }
        
        cbpf_code = RF->RF_ANA_ST0.BPF_CAL_CODE;
        
        RF->BPFMIX_CTRL.Word  = 0x00437D00 | (cbpf_code << RF_BPF_CAL_CODE_EXT_LSB);
        RF->PLL_DYM_CTRL.Word = 0;
        
        btmr_delay(16, 20);
        
        rf_cal_env.bpf_ctrl = RF->BPFMIX_CTRL.Word;
        
        RF->DIG_CTRL.Word = 0x0022925A;
    }
    
    return cbpf_code;
}

uint8_t get_gain_cal_st(uint8_t ch_idx)
{
    RF->DIG_CTRL.Word     = 0x0032805A;
    RF->PLL_DYM_CTRL.Word = ((ch_idx << RF_SW_CH_NUM_LSB) | (1 << RF_SW_TX_EN_POS) | (SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB));
    
    btmr_delay(16, 200);
    
    while (!(RF->PLL_CAL_ST.GAIN_CALIB_DONE));

    uint8_t sta = RF->PLL_CAL_ST.PLL_GAIN_CAL_ST;
    
    RF->PLL_DYM_CTRL.Word = (SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB);
    btmr_delay(16, 100);
    
    return (sta);
}

uint8_t get_gain_cal_avg(uint8_t ch_idx1, uint8_t chan_idx2)
{
    uint16_t gain_cal = 0;//get_gain_cal_st(ch_idx1);
    for (uint8_t i = 0; i < 2; ++i)
    {
        gain_cal += get_gain_cal_st(ch_idx1);
    }
//    gain_cal += get_gain_cal_st(chan_idx2);
    
    gain_cal >>= 1;
    
    return gain_cal;
}

void gain_cal(void)
{
    if (RF_CAL_VALID == rf_cal_env.cal_valid)
    {
        RF->PLL_DAC_TAB0.Word = rf_cal_env.dac_tab[0];
        RF->PLL_DAC_TAB1.Word = rf_cal_env.dac_tab[1];
        RF->PLL_DAC_TAB2.Word = rf_cal_env.dac_tab[2];
    }
    else
    {
        uint32_t dac_tab[3] = {0};
        //低频段阈值
//        RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 230; //按照287.5KHz计算 --- 2023.6.24 lch.
        RF->PLL_GAIN_CTRL.Word = (0x00002600 | (230 << RF_PLL_GAIN_CAL_TH_LSB));
        
        //2400~2407
//        RF->PLL_DAC_TAB0.PLL_DAC_ADJ00 = get_gain_cal_avg(1, 37); // 2406, 2402
        dac_tab[0] = get_gain_cal_avg(1, 37);
        
        //2408~2415
//        RF->PLL_DAC_TAB1.PLL_DAC_ADJ10 = get_gain_cal_avg(3, 4); // 2410, 2412
        dac_tab[1] = get_gain_cal_avg(3, 4);
        
        //2416~2423
//        RF->PLL_DAC_TAB0.PLL_DAC_ADJ01 = get_gain_cal_avg(7, 8); // 2418, 2420
        dac_tab[0] |= get_gain_cal_avg(7, 8) << RF_PLL_DAC_ADJ01_LSB;
        
        //2424~2431
//        RF->PLL_DAC_TAB1.PLL_DAC_ADJ11 = get_gain_cal_avg(11, 12); // 2428, 2430
        dac_tab[1] |= get_gain_cal_avg(11, 12) << RF_PLL_DAC_ADJ11_LSB;
        
        //2432~2439
//        RF->PLL_DAC_TAB0.PLL_DAC_ADJ02 = get_gain_cal_avg(14, 15); // 2434, 2436
        dac_tab[0] |= get_gain_cal_avg(14, 15) << RF_PLL_DAC_ADJ02_LSB;
        
        //高频段减小阈值
//        RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 220; //按照287.5KHz计算 --- 2023.6.24 lch.
        RF->PLL_GAIN_CTRL.Word = (0x00002600 | (220 << RF_PLL_GAIN_CAL_TH_LSB));
        
        //2440~2447
//        RF->PLL_DAC_TAB1.PLL_DAC_ADJ12 = get_gain_cal_avg(18, 19); // 2442, 2444
        dac_tab[1] |= get_gain_cal_avg(18, 19) << RF_PLL_DAC_ADJ12_LSB;
        
        //2448~2455
//        RF->PLL_DAC_TAB0.PLL_DAC_ADJ03 = get_gain_cal_avg(22, 23); // 2450, 2452
        dac_tab[0] |= get_gain_cal_avg(22, 23) << RF_PLL_DAC_ADJ03_LSB;
        
        //2456~2463
//        RF->PLL_DAC_TAB1.PLL_DAC_ADJ13 = get_gain_cal_avg(26, 27); // 2458, 2460
        dac_tab[1] |= get_gain_cal_avg(26, 27) << RF_PLL_DAC_ADJ13_LSB;
        
        //2464~2471
//        RF->PLL_DAC_TAB0.PLL_DAC_ADJ04 = get_gain_cal_avg(30, 31); // 2466, 2468
        dac_tab[0] |= get_gain_cal_avg(30, 31) << RF_PLL_DAC_ADJ04_LSB;
        
        //2472~2479
//        RF->PLL_DAC_TAB1.PLL_DAC_ADJ14 = get_gain_cal_avg(34, 35); // 2474, 2476
        dac_tab[1] |= get_gain_cal_avg(34, 35) << RF_PLL_DAC_ADJ14_LSB;
        
        //2480~2487
//        RF->PLL_DAC_TAB2.PLL_DAC_ADJ05 = get_gain_cal_avg(39, 39); // 2480
        dac_tab[2] =  get_gain_cal_avg(39, 39);
        
        //2488~2495
        dac_tab[2] |=  dac_tab[2] << RF_PLL_DAC_ADJ15_LSB;
        
        RF->PLL_DAC_TAB0.Word = dac_tab[0];
        RF->PLL_DAC_TAB1.Word = dac_tab[1];
        RF->PLL_DAC_TAB2.Word = dac_tab[2];
        
        rf_cal_env.dac_tab[0] = dac_tab[0];
        rf_cal_env.dac_tab[1] = dac_tab[1];
        rf_cal_env.dac_tab[2] = dac_tab[2];
        
        rf_cal_env.cal_valid  = RF_CAL_VALID;
    }
}

void rf_reg_init(void)
{
#if (0)
    // 0x00, Read Write
    RF->DIG_CTRL.ANA_TEST_EN            = 0;
    RF->DIG_CTRL.LDO_TEST_EN            = 1;
    RF->DIG_CTRL.PLL_CAL_TEST_EN        = 0;
    RF->DIG_CTRL.FSM_CTRL_SEL           = 0;
    RF->DIG_CTRL.PA_GAIN_TARGET_SEL     = 1;
    RF->DIG_CTRL.PLL_AFC_BY             = 0;
    RF->DIG_CTRL.PLL_BPF_CAL_BY         = 1;
    RF->DIG_CTRL.PLL_FREQ_SEL           = 0;
    RF->DIG_CTRL.PLL_FREQ_EXT_EN        = 0;
    RF->DIG_CTRL.PLL_GAIN_CAL_BY        = 0;
    RF->DIG_CTRL.PLL_VTXD_EXT_EN        = 0;
    RF->DIG_CTRL.PLL_GAIN_CAL_STEP      = 0;
#if (USE_DAC_TAB)
    RF->DIG_CTRL.PLL_GAIN_CAL_BY        = 1;
    RF->DIG_CTRL.PLL_GAIN_CAL_TAB       = 1;
#else
    RF->DIG_CTRL.PLL_GAIN_CAL_TAB       = 0;
#endif
    RF->DIG_CTRL.PLL_GAIN_CAL_MODE      = 0;
    RF->DIG_CTRL.PLL_AFC_ROUND          = 1;
    RF->DIG_CTRL.PLL_AFC_STEP           = 0;
    RF->DIG_CTRL.PLL_FREQ_DC            = 1;
#if (USE_DAC_TAB)
    RF->DIG_CTRL.PLL_AFC_MODE           = 0;
#else
    RF->DIG_CTRL.PLL_AFC_MODE           = 1;
#endif
    RF->DIG_CTRL.PLL_AFC_FRAC_EN        = 1;
    RF->DIG_CTRL.SW_PLL_CAL_CLKEN       = 0;
    RF->DIG_CTRL.SW_AFC_EN              = 0;
    RF->DIG_CTRL.SW_GAIN_CAL_EN         = 0;
    RF->DIG_CTRL.SW_PLL_VREF_SEL        = 0;
    RF->DIG_CTRL.SW_PLL_HBW_EN          = 0;
    RF->DIG_CTRL.SDM_FRAC_INT_MODE      = 0;
//    RF->DIG_CTRL.RF_DBG_SEL             = 0;
    
    // 0x04, Read Write
    RF->PLL_TAB_OFFSET.TX_FRAC_OFFSET   = 0;
    RF->PLL_TAB_OFFSET.RX_FRAC_OFFSET   = 0;
    RF->PLL_TAB_OFFSET.PLL_FREQ_EXT     = 0x00;

    // 0x08, Read Write
    RF->PLL_GAIN_CTRL.PLL_VTXD_EXT      = 0x2C;
    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_DC   = 0;
    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_WIN  = 19;
    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 200;//230; //按照287.5KHz计算 --- 2023.6.24 lch.
//    RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 220; //按照287.5KHz计算 --- 2023.6.24 lch.

    // 0x0C, Mixed
//    RF->RSSI_CTRL.RSSI_VAL               = 0;
//    RF->RSSI_CTRL.AGC_LNA_GAIN           = 0;
//    RF->RSSI_CTRL.AGC_BPF_GAIN_ADJ       = 0;
//    RF->RSSI_CTRL.AGC_MIXL_GAIN_CTL      = 0;
//    RF->RSSI_CTRL.AGC_MIXH_GAIN_CTL      = 0;
//    RF->RSSI_CTRL.SW_RSSI_READY          = 0;
//    RF->RSSI_CTRL.SW_RSSI_REQ            = 0; // rw, bit17

//chan30~34
    // 0x10, Read Write
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ00         = 43; //2400~2407
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ01         = 37; //2416~2423
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ02         = 35; //2432~2439
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ03         = 32; //2448~2455
    RF->PLL_DAC_TAB0.PLL_DAC_ADJ04         = 27; //2464~2471

    // 0x14, Read Only
//    RF->PLL_CAL_ST.AFC_CALIB_DONE;
//    RF->PLL_CAL_ST.GAIN_CALIB_DONE;
//    RF->PLL_CAL_ST.PLL_FREQ_ADJ_ST;
//    RF->PLL_CAL_ST.PLL_GAIN_CAL_ST;
//    RF->PLL_CAL_ST.CALIB_CNT_RPT;
//    RF->PLL_CAL_ST.PLL_GAIN_CAL_ERR;

    // 0x18, Read Write
//    RF->PLL_FREQ_CTRL.SW_PLL_DI_S        = 0x15; // tx,0x15 --- 2400MHz
//    RF->PLL_FREQ_CTRL.SW_PLL_FRAC        = 0;

    // 0x1C, Read Write
    RF->PLL_DYM_CTRL.SW_CH_NUM           = 0;
    RF->PLL_DYM_CTRL.SW_RATE             = 0; // 0:1M, 1:2M
    RF->PLL_DYM_CTRL.SW_TX_EN            = 0;
    RF->PLL_DYM_CTRL.SW_RX_EN            = 0;
    RF->PLL_DYM_CTRL.SW_PA_GAIN_TARGET   = 0x0F; // bit[13:10]
    
    // 0x20, Read Write, default value
    RF->FSM_DLY_CTRL0.LDO_START_TIME     = 1;
    RF->FSM_DLY_CTRL0.EN_PLL_TIME        = 1;
    RF->FSM_DLY_CTRL0.AFC_START_TIME     = 3;
    RF->FSM_DLY_CTRL0.BPF_START_TIME     = 0x1E;
    RF->FSM_DLY_CTRL0.TX_END_DLY_TIME    = 1;
    RF->FSM_DLY_CTRL0.RX_END_DLY_TIME    = 1;
    RF->FSM_DLY_CTRL0.TX_PA_RAMPDOWN_TIME= 0;

    // 0x24, Read Write
    RF->FSM_DLY_CTRL1.LNA_START_TIME     = 0x28;
    RF->FSM_DLY_CTRL1.EN_PA_TIME         = 6;
    RF->FSM_DLY_CTRL1.AGC_START_TIME     = 2;
    RF->FSM_DLY_CTRL1.PA_STARTUP_TIME    = 1;
    RF->FSM_DLY_CTRL1.PA_STEP_TIME       = 3;
   
   // 0x2C, Read Only
//    RF->PLL_GAIN_CAL_VAL.PLL_GAIN_CAL_DF0;
//    RF->PLL_GAIN_CAL_VAL.PLL_GAIN_CAL_DF1;
//    RF->PLL_GAIN_CAL_VAL.PLL_CAL_STATE;

    // 0x30, Read Write
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ10       = 40; //2408~2415
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ11       = 36; //2424~2431
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ12       = 33; //2440~2447
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ13       = 31; //2456~2463
    RF->PLL_DAC_TAB1.PLL_DAC_ADJ14       = 26; //2472~2479
    
    // 0x34, Read Write
    RF->PLL_DAC_TAB2.PLL_DAC_ADJ05       = 25; //2480~2487
    RF->PLL_DAC_TAB2.PLL_DAC_ADJ15       = 24; //2488~2495
    
    // 0x38, Read Write
    // 0:SDM data from rf_ana, 1: SDM data from mdm
    RF->DATA_DLY_CTRL.SDM_DAT_SEL        = 1;
    RF->DATA_DLY_CTRL.MDM_SDM_DATA_DLY_1M= 5;
    RF->DATA_DLY_CTRL.DAC_DATA_DLY_1M    = 0;
    RF->DATA_DLY_CTRL.MDM_SDM_DATA_DLY_2M= 0;
    RF->DATA_DLY_CTRL.DAC_DATA_DLY_2M    = 0;
    RF->DATA_DLY_CTRL.SDM_CLK_PH         = 1;
    
    // 0x4C, Read Write
    RF->BPFMIX_CTRL.BPF_BW_ADJ           = 0;//1;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT     = 0x24;//0x1A;
    RF->BPFMIX_CTRL.BPF_CAL_CODE_EXT_EN  = 1;
    RF->BPFMIX_CTRL.BPF_CAL_EN           = 0;
    RF->BPFMIX_CTRL.BPF_CENT_ADJ         = 3;//0;
    RF->BPFMIX_CTRL.BPF_IADJ             = 7;
    RF->BPFMIX_CTRL.BPF_MODE_SEL         = 0;
    RF->BPFMIX_CTRL.MIXL_BIAS_CTL        = 2;
    RF->BPFMIX_CTRL.MIXL_BIAS_SEL        = 0;
    RF->BPFMIX_CTRL.MIXH_ENB_CAP         = 0;
    RF->BPFMIX_CTRL.MIXH_BIAS_CTL        = 3; // bit[18:16]
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
    RF->AGC_CTRL0.DISABLE_AGC            = 0;//1;    // bit28 // blocking测试时配成0

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
    RF->ANA_TRIM.BG_VREF_FINE            = 6;//0;
    RF->ANA_TRIM.LDO_RX_TRIM             = 4;
    RF->ANA_TRIM.LDO_TX_TRIM             = 4;
    RF->ANA_TRIM.LNA_RES_TRIM            = 2;
    RF->ANA_TRIM.BPF_GAIN_ADJ            = 3;
    RF->ANA_TRIM.MIXL_GAIN_CTL           = 0;
    RF->ANA_TRIM.MIXH_GAIN_CTL           = 0;
    RF->ANA_TRIM.LNA_GAIN                = 0x0F;
    RF->ANA_TRIM.DAC_BLE_DELAY_ADJ_2M    = 0;

    // 0x60, Read Write
    RF->ANAMISC_CTRL1.DAC_BLE_DELAY_ADJ_1M= 0;//0x10;
    RF->ANAMISC_CTRL1.DAC_REFL_ADJ        = 4;//受温度和批次影响 --- 2022.07.05 谢波
    RF->ANAMISC_CTRL1.DAC_CAL_DATA_EXT    = 0;
    RF->ANAMISC_CTRL1.DAC_CAL_EN_EXT      = 0;
    RF->ANAMISC_CTRL1.DAC_EXT_EN          = 0;
    RF->ANAMISC_CTRL1.DAC_REFH_ADDJ       = 1;
    RF->ANAMISC_CTRL1.DAC_REFH_ADJ        = 0;
    RF->ANAMISC_CTRL1.BYP_LDOIF           = 0;
    RF->ANAMISC_CTRL1.BYP_LDOPLL          = 0;
    RF->ANAMISC_CTRL1.BYP_LDOTX           = 0;
    RF->ANAMISC_CTRL1.BYP_LDOVCO          = 0;
    RF->ANAMISC_CTRL1.CF_BW08M_ADJ        = 0;
    RF->ANAMISC_CTRL1.TSTEN_CBPF          = 0;//1;// 观测2M_IF
    RF->ANAMISC_CTRL1.TSTEN_RSSI          = 0;
    RF->ANAMISC_CTRL1.AT0_SEL             = 0;
    RF->ANAMISC_CTRL1.AT1_SEL             = 0;
    RF->ANAMISC_CTRL1.VCO_RES             = 0; //可能影响Blocking. ---rj
    RF->ANAMISC_CTRL1.VCOAFC_SEL          = 0;

    // 0x64, Read Write
    RF->ANA_PWR_CTRL.TEST_EN_LDO_VCO     = 1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_PA      = 1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_IF      = 1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_PLL     = 1;
    RF->ANA_PWR_CTRL.TEST_EN_DAC_DIG_PWR = 1;
    RF->ANA_PWR_CTRL.TEST_EN_AGC_PWR     = 1;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_TX      = 1;

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
    
    RF->ANA_PWR_CTRL.CLK_EN_DAC          = 0;//1;
    RF->ANA_PWR_CTRL.CLK_EN_BPF          = 0;//1;
    RF->ANA_PWR_CTRL.CLK_EN_PLL          = 1;
    RF->ANA_PWR_CTRL.CLK_EN_AGC          = 0;//1;

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
    RF->PLL_ANA_CTRL.PLL_BW_ADJ          = 0; // 影响锁频范围 --- 2022.07.05 谢波. //可能影响Blocking. ---rj
    RF->PLL_ANA_CTRL.PLL_CP_OS_EN        = 0;
    RF->PLL_ANA_CTRL.PLL_CP_OS_ADJ       = 0;
    RF->PLL_ANA_CTRL.PLL_DIV_ADJ         = 2; //可能影响Blocking. ---rj
    RF->PLL_ANA_CTRL.PLL_DI_P            = 0x1F;
    RF->PLL_ANA_CTRL.PLL_FAST_LOCK_EN    = 1;
    RF->PLL_ANA_CTRL.PLL_FBDIV_RST_EXT   = 0;
    RF->PLL_ANA_CTRL.PLL_FBDIV_RST_SEL   = 0;
    RF->PLL_ANA_CTRL.PLL_LOCK_BYPS       = 0;
    RF->PLL_ANA_CTRL.PLL_PS_CNT_RST_SEL  = 0;
    RF->PLL_ANA_CTRL.PLL_REF_SEL         = 0;
    RF->PLL_ANA_CTRL.PLL_SDM_TEST_EN     = 0;
    RF->PLL_ANA_CTRL.PLL_VCO_ADJ         = 4;//6; //调VCO电流, 可能影响drift --- 2023.6.23 rj
    RF->PLL_ANA_CTRL.PLL_VCTRL_EXT_EN    = 0;
    RF->PLL_ANA_CTRL.PLL_VREF_ADJ        = 4;
    RF->PLL_ANA_CTRL.PLL_MIX_SEL         = 1; // in Rx --- 0:-2M, 1:+2M
    RF->PLL_ANA_CTRL.PLL_RTX_SEL_REG     = 1; // 1 tx, 0 rx
    RF->PLL_ANA_CTRL.PLL_SEL_RTX_BW      = 0;
    
    // 0x70, Read Write
    // bit[14:11] = 2, bit15:CLK_128M_DAC enable
    RF->RF_RSV = ((7UL << 11) | (1UL << 15));
    
    // 0x74, Read Only
//    RF->RF_ANA_ST0.AGC_STATE0
//    RF->RF_ANA_ST0.AGC_STATE1
//    RF->RF_ANA_ST0.BPF_CAL_CODE
//    RF->RF_ANA_ST0.BG_VREF_OK12
//    RF->RF_ANA_ST0.BPF_CAL_DONE
//    RF->RF_ANA_ST0.PLL_LOCK
//    RF->RF_ANA_ST0.FAST_LOCK_DONE
//    RF->RF_ANA_ST0.AGC_STATE_TEST
#else
    RF->FSM_DLY_CTRL0.Word = 0x0045E311;
    //.PA_STEP_TIME = 3
    RF->FSM_DLY_CTRL1.Word = 0x03120628;
    RF->DATA_DLY_CTRL.Word = 0x0000238B;
    RF->BPFMIX_CTRL.Word   = 0x00437D90;
    RF->AGC_CTRL0.Word     = 0x050BD572;
    RF->AGC_CTRL1.Word     = 0x000966B2;
    RF->AGC_CTRL2.Word     = 0x00E0331A;
    
    //.BG_RES_TRIM = 0x14
//    RF->ANA_TRIM.Word      = 0x07DA9351;
    // 0x5C, Read Write
    RF->ANA_TRIM.BG_BIAS_TRIM            = 1;
//    RF->ANA_TRIM.BG_RES_TRIM             = 0x1F; //LDO_RX_TRIM = 4时, BG_RES_TRIM=0x17,VDD_IF:1.28V对应VBG1.2V
//    RF->ANA_TRIM.BG_RES_TRIM             = RD_32(0x18000F0C) >> 12;
    RF->ANA_TRIM.BG_VREF_FINE            = 6;
    RF->ANA_TRIM.LDO_RX_TRIM             = 4;
    RF->ANA_TRIM.LDO_TX_TRIM             = 4;
    RF->ANA_TRIM.LNA_RES_TRIM            = 2;
    RF->ANA_TRIM.BPF_GAIN_ADJ            = 3;
    RF->ANA_TRIM.MIXL_GAIN_CTL           = 0;
    RF->ANA_TRIM.MIXH_GAIN_CTL           = 1;
    RF->ANA_TRIM.LNA_GAIN                = 0x0F;
    RF->ANA_TRIM.DAC_BLE_DELAY_ADJ_2M    = 0;
    
    RF->ANAMISC_CTRL1.Word = 0x00000880;
    //.TEST_EN_LDO_PLL = 1
    RF->ANA_PWR_CTRL.Word  = 0x07FF7F7F;
    RF->ANA_EN_CTRL.Word   = 0x00070000;
    RF->PLL_ANA_CTRL.Word  = 0x3180FE00;
    RF->RF_RSV             = 0x0000B800;
#endif

    cbpf_cal();

//    gain_cal();
    
    RF->PLL_DAC_TAB0.Word = 0x229AEBAD;
    RF->PLL_DAC_TAB1.Word = 0x228E6BEE;
    RF->PLL_DAC_TAB2.Word = 0x0000079E;

    RF->PLL_DYM_CTRL.Word = (SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB);
    
    // .PA_STEP_TIME = 7
    RF->FSM_DLY_CTRL1.Word = 0x07120628;
    RF->DIG_CTRL.Word      = 0x00229252;
}

static void rf_mdmr_init(void)
{
    MDM->REG0.Word         = 0x00000081;
    MDM->REG1.Word         = 0x03E83474;
}

//Freq: 2402MHz ~ 2480MHz, Channel: 0 ~ 39
const uint8_t freq_table[40] =
{
    0x25, //ch37:2402
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, //ch0~10:2404~2424
    0x26, //ch38:2426
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, //ch11~36:2428~2478
    0x27 //ch39:2480
};

void rf_emi_init(void)
{
    memcpy((void *)(EM_BASE_ADDR + EM_FT_OFFSET), &freq_table[0], 40);
}

static uint8_t rf_freq2chnl(uint8_t freq)
{
    return freq_table[freq];
}

/*
 * RF Tests
 ****************************************************************************************
 */
//chnl: 0~39(2402MHz~2480MHz, step 2MHz)
static void rf_ble_prg(uint8_t fmt, uint8_t chnl, uint8_t rate, uint8_t pkt_len)
{
    uint32_t timestamp = get_ble_time().time + 2;

    struct ev_et_tag ev_et0 =
    {
        2 /* mode*/,
        0 /* status*/,
        0 /*iso*/,
        0 /*rsvd*/,
        0 /*ae_nps*/,
        0 /*isobufsel*/,
        0 /*spa*/,
        5 /*sch_prio1*/,
        (unsigned short)(timestamp & 0xFFFF) /*rawstp0*/,
        (unsigned short)((timestamp >> 16) & 0xFFFF) /*rawstp1*/,
        0x00 /*finestp*/,
        EM_BLE_CS_OFFSET /*csptr*/,
        0x02 /*priobw*/,
        1 /*priobw_unit*/,
        2 /*sch_prio2*/,
        4 /*sch_prio3*/,
        0 /*isochan*/,
        4 /*pti_prio*/
    };
    ble_ex_et_prg(0, &ev_et0);

    // format(0x1C-Tx,0x1D-Rx,0x1E-TxRx)
    struct ev_cs_tag  rf_test_cs =
    {
        fmt /*format*/, 0 /*dnabort*/, 1 /*rxbsyen*/, 1 /*txbsyen*/,
        0 /*priv_npub*/, 0 /*rxcrypt_en*/, 0 /*txcrypt_en*/, 0 /*cryptmode*/, 0/*mic_mode*/, 0 /*nullrxllidflt*/, 0/*sas*/, 0xA /*linklbl*/, 
        rate /*txrate*/, rate /*rxrate*/, 0 /*txthr*/, 1 /*rxthr*/,
        0xCAFEFADE /*bdaddrlsb*/,
        0xDECA /*bdaddrmsb*/,
        RF_SYNC_WORD/*syncword*/,
        0x555555 /*crcinit*/,
        rf_freq2chnl(chnl)/*chnl*/ /*ch_idx*/, 7 /*hopint*/, 0 /*hop_mode*/, 0 /*fh_en*/,
        0x1F/*5*/ /*txpwr*/, 1 /*nesn*/, 1 /*sn*/,
        // Modify. 2022.03.03 --- wq.
//        2100/*0x64*/ /*rxwinsz*/, 0 /*1*//*rxwide*/,
        38 /*rxwinsz*/, 1/*rxwide*/,
        EM_BLE_TX_DESC_OFFSET>>2 /*txdescptr*/,
        0x100 /*minevtime*/,
        0x140 /*maxevtime*/,
        0xFFFFFFFF /*llchmaplsb*/, 0x1F /*llchmapmsb*/,
        pkt_len /*rxmaxbuf*/,
        0 /*rxmaxtime*/,
        0 /*sk10*/,
        0 /*sk32*/,
        0 /*sk54*/,
        0 /*sk67*/,
        0 /*iv10*/,
        0 /*iv32*/,
        0 /*evtcnt*/
    };
    ble_cs_prg(0, &rf_test_cs);
}

void rf_rx_test(uint8_t chnl, uint8_t rate, uint8_t pkt_len)
{
    RCC->BLE_CLKRST_CTRL.BLE_MASTER_RSTREQ  = 1;
    RCC->BLE_CLKRST_CTRL.BLE_MASTER_RSTREQ  = 0;

//    RF->RF_RSV &= ~(0x01UL << 15);
 
    for (uint8_t idx = 0; idx < 6; idx++)
    {
        if (em_ble_rxcntl_rxdone_getf(idx))
        {
            em_ble_rxcntl_rxdone_setf(idx, 0);
        }
    }

    // Set Rx Window Size
    ble_blecntl_rxwinszdef_setf(0x0A);

    // Advertising Channels Error Filtering Enable
    ble_blecntl_advertfilt_en_setf(1);

    // Enable BLE Core
    ble_blecntl_rwble_en_setf(1);

    ble_timgencntl_set(0x014000F0);

    ble_blecntl_rftest_abort_setf(1);
    
    ble_blecntl_radiocntl_soft_rst_setf(0);
//    ble_radiocntl1_forceagc_en_setf(0);

    // Clear counter dedicated for the test mode
    em_ble_rxccmpktcnt0_set(0, 0);
    em_ble_rxccmpktcnt1_set(0, 0);
    em_ble_rxccmpktcnt2_set(0, 0);

    // Set Rx Packet Counter enable, reg: BB_Base + 0xD0
    ble_rftestcntl_rxpktcnten_setf(1);
    ble_rftestcntl_txpktcnten_setf(0);

    //  ble_rftestcntl_infiniterx_setf(1);
    //# Define Rx Descriptor Pointer
    //LE_WR_RG 00000024 00000300
    ble_currentrxdescptr_setf(EM_BLE_RX_DESC_OFFSET>>2);
    ble_etptr_setf(EM_ET_OFFSET>>2);

    // Prg ble_et & ble_cs
    rf_ble_prg(EM_BLE_CS_FMT_RX_TEST, chnl, rate, pkt_len);
    
    //# Rx
    for (uint8_t idx = 0; idx < 6; idx++)
    {
        em_ble_rxcntl_rxnextptr_setf(idx, (EM_BLE_RX_DESC_OFFSET + ((idx+1)%6) * REG_EM_BLE_RX_DESC_SIZE)>>2);
        em_ble_rxdataptr_setf(idx, EM_BLE_DATARXBUF_OFFSET + idx * EM_BLE_DATARXBUF_SIZE);
    }
    
    //# Kick the event
    ble_actschcntl_start_act_setf(1);
    ble_slotclk_clkn_upd_setf(1);
}

void rf_tx_cmw500(uint8_t chnl, uint8_t rate, uint16_t data_len, uint8_t tx_payl)
{
//    RF->RF_RSV |= (0x01UL << 15);    
    
    em_ble_rxcntl_rxdone_setf(0, 0);
    
    // Advertising Channels Error Filtering Enable
    ble_blecntl_advertfilt_en_setf(1);

    // Enable BLE Core
    ble_blecntl_rwble_en_setf(1);

    ble_timgencntl_set(0x014000F0);
    
    ble_blecntl_rftest_abort_setf(1);

    // Set Tx Packet Counter enable, reg: BB_Base + 0xD0
    ble_rftestcntl_txpktcnten_setf(1);
    ble_rftestcntl_rxpktcnten_setf(0);

    //# Define Rx Descriptor Pointer
    //LE_WR_RG 00000024 00000300
    ble_currentrxdescptr_setf(EM_BLE_RX_DESC_OFFSET);
    ble_etptr_setf(EM_ET_OFFSET>>2);

    // Prg ble_et & ble_cs
    rf_ble_prg(EM_BLE_CS_FMT_TX_TEST, chnl, rate, data_len);
    
    struct ev_tx_adv_des_tag  tx_test_desc =
    {
        0 /*nextptr*/,
        tx_payl, 0 /*txtxadd*/, 0 /*txrxadd*/, 0 /*txchsel*/,
        data_len /*txadvlen*/,
        EM_BLE_ACLTXBUF_OFFSET /*txdatptr*/
    };
    ble_txdsc_phadv_prg(0, &tx_test_desc);
    
    // sets the source to CS
    ble_rftestcntl_txpldsrc_setf(0);
    
    ble_actschcntl_start_act_setf(1);
    ble_slotclk_clkn_upd_setf(1);
}

void rf_stop_test(void)
{
    // Re-enable force AGC mechanism
//    ble_radiocntl1_forceagc_en_setf(1);

    // Clear the Regulatory Body and RF Testing Register
    ble_rftestcntl_set(0);

    ble_rftestcntl_infiniterx_setf(1);
    ble_blecntl_set( ble_blecntl_get() | BLE_RFTEST_ABORT_BIT | BLE_RADIOCNTL_SOFT_RST_BIT);
    ble_rftestcntl_infiniterx_setf(0);
}

void rf_init_test(void)
{
    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ     = 1;
    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ     = 0;
    RCC->BLE_CLKRST_CTRL.BLE_MASTER_RSTREQ  = 1;
    RCC->BLE_CLKRST_CTRL.BLE_MASTER_RSTREQ  = 0;
  
    emi_init();
    
    // set ble interrupt
    ble_intcntl0_set(BLE_RXINTMSK_BIT | BLE_ERRORINTMSK_BIT);
    ble_intcntl1_set(BLE_FIFOINTMSK_BIT);
    
    rf_emi_init();
    rf_core_init();
//    rf_reg_init();
    rf_mdmr_init();
    
    // Re-enable force AGC mechanism
//    ble_radiocntl1_forceagc_en_setf(1);

#if (WHIT_DISABLE)
    ble_blecntl_whit_dsb_setf(1);
#else
    // Enable the whitening
    ble_blecntl_whit_dsb_setf(0);
#endif

//    WR_32(0x50000050, 0x83);
    NVIC_EnableIRQ(BLE_IRQn);
}

static uint8_t rf_rx_result(void)
{
    uint8_t rx_ok = 0;
    NVIC_DisableIRQ(BLE_IRQn);

    //while(!((RD_32(0x500000D0) >> 27) & 0x01));
    
    if ((RD_32(0x500000D0) >> 27) & 0x01)
        rx_ok = RD_32(0x500000D8);
    else
        rx_ok = 0;

    return rx_ok;
}

//static uint8_t rf_tx_result(void)
//{
//    uint16_t tx_desc = *(volatile uint16_t *)(TX_DESC_ADDR + 2);
////    NVIC_DisableIRQ(BLE_IRQn);
//    
//    tx_desc >>= 8;
//    
//    if ((RD_32(0x500000D0) >> 11) & 0x01)
//        tx_desc = RD_32(0x500000D4);
//    else
//        tx_desc = 0;
//    
//    return tx_desc;
//}
uint16_t g_tx_payl = PAYL_10101010, g_tx_pkt_cnt = RF_PKT_CNT;
// freq: Tx:0x21~0x48(2402~2480MHz), Rx:0x81~0xA8(2402~2480MHz) --> chnl: 0~39
// pkt_len: 37~255
uint8_t rf_test(uint8_t freq, uint8_t pkt_len)
{
    uint8_t result;
    
//    sys_dbg_init();
//    ble_dbg_sel(0x8300);
    GPIO_DAT_SET(GPIO15); 
    rf_init_test();
    GPIO_DAT_CLR(GPIO15);
    
    iospc_rstpin(true);
    
//    ble_io_init();
    if ((freq == 0x00) || (freq == 0x80))
    {
        rf_stop_test();
    }
    else 
    {  
        uint8_t chnl = (freq & 0x7F);
        if (chnl > 0x28)
        {
            return 0; // error freq
        }
        else
        {
            //chnl = rf_freq2chnl(chnl - 1);
            chnl = (chnl - 1);
        }
        
//        if (pkt_len < 37)
//            pkt_len = 37;
        
        if (freq & 0x80)
        {
//            ioBleTxRx(17, 19);
            rf_stop_test();
            #if (AUX_TEST_CHIP)
            rf_stop_test();

            bootDelayMs(3);
            rf_tx_cmw500(chnl, RATE_1Mbps, pkt_len, PAYL_PRBS9);
//            // ????????
            while (RD_32(0x500000D4) < RF_PKT_CNT);          
            ble_blecntl_rftest_abort_setf(1);
            #else
            g_rx_cnt = 0;
            g_rx_err = 0;

//            bootDelayMs(4);
            btmr_delay(16, 4000);
            
                 
            rf_rx_test(chnl, RATE_1Mbps, pkt_len);
            NVIC_EnableIRQ(BLE_IRQn);
            __enable_irq();
            

            bootDelayMs(90);
            ble_blecntl_rftest_abort_setf(1);
            result = rf_rx_result();
//            DEBUG("freq:%02x, rx_ok:%d, rx_cnt:%d, rx_err:%d", freq, result, g_rx_cnt, g_rx_err);
            #if (DBG_MODE)
            printf("%02X, %d, %d, %d\r\n", freq, result, g_rx_cnt, g_rx_err);
            #endif
            #endif
        }
        else
        {
            result = (0x80 | g_tx_pkt_cnt);
            
//            ioBleTxRx(19, 17);
            #if (AUX_TEST_CHIP)
            g_rx_cnt = 0;
            g_rx_err = 0;

//            bootDelayMs(4);
//            btmr_delay(16, 4500);
            rf_rx_test(chnl, RATE_1Mbps, pkt_len);
            NVIC_EnableIRQ(BLE_IRQn);
            __enable_irq();
            bootDelayMs(32);
            ble_blecntl_rftest_abort_setf(1);
            result = rf_rx_result();
            DEBUG("freq:%x, rx_ok:%d, rx_cnt:%d, rx_err:%d", freq, result, g_rx_cnt, g_rx_err);
            #else
            rf_stop_test();
            #if (0)
            for (uint16_t i = 0; i < 20; i++)
            { // packet count
                //sysTickDelay(16*150);
                rf_tx_cmw500(chnl, RATE_1Mbps, pkt_len, PAYL_PRBS9);
                bootDelayUs(1200); // 1.2ms
                ble_blecntl_rftest_abort_setf(1);
            }
            #else
            bootDelayMs(4);
            rf_tx_cmw500(chnl, RATE_1Mbps, pkt_len, g_tx_payl/*PAYL_PRBS9*/);
            while (RD_32(0x500000D4) < g_tx_pkt_cnt/*RF_PKT_CNT*/);          
            ble_blecntl_rftest_abort_setf(1);
            #endif
//            result = rf_tx_result();
            #endif
        }

        rf_stop_test();
    }
    
    return result;
}

void BLE_IRQHandler(void)
{
    uint32_t irq_stat      = ble_intstat0_get();
    uint32_t irq_stat1     = ble_intstat1_get();
    
    // 2021.8.4.  6vip
    if ((irq_stat == 0) && (irq_stat1 == 0))
        return;
    
    // Error interrupt
    if (irq_stat & BLE_ERRORINTSTAT_BIT)
    {
        // Clear the interrupt
        ble_intack0_errorintack_clearf(1);
    }
    
    // FIFO
    if (irq_stat1 & BLE_FIFOINTSTAT_BIT) // FIFO interrupt
    {
        uint32_t actfifostat = ble_actfifostat_get();
        
        // TX
        if (actfifostat & BLE_TXINTSTAT_BIT)
        {
        }
        
        // RX
        if (actfifostat & BLE_RXINTSTAT_BIT)
        {
            uint16_t rx_sta = 0;
            
            for (uint8_t idx = 0; idx < 6; idx++)
            {
                if (em_ble_rxcntl_rxdone_getf(idx))
                {
                    rx_sta = em_ble_rxstatce_get(idx);
                    em_ble_rxcntl_rxdone_setf(idx, 0);
                    g_rx_cnt++;
                    
                    if (rx_sta/* & EM_BLE_CRC_ERR_BIT*/)
                    {
                        g_rx_err++;
                    }
                }
            }
        }

        // Ack FIFO interrupt
        ble_intack1_fifointack_clearf(1);
    }
}
