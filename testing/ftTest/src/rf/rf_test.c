#include <stdio.h>
#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "reg_blecore.h"
#include "reg_em_ble_cs.h"
#include "reg_em_ble_rx_desc.h"
#include "rf_test.h"
#include "rf_utils.h"
#include "ft_test.h"
#include "bb_mdm_rf_debug.h"

#if !defined(RF_SYNC_WORD)
#define RF_SYNC_WORD       (0x71764129)
#endif

#if !(RF_PKT_CNT)
#define RF_PKT_CNT       20
#endif

uint16_t g_rx_cnt = 0, g_rx_err = 0;

/*
 * RF drivers
 ****************************************************************************************
 */
#if (DEBUG_RF)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<RF>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
#endif

volatile bool g_tx_flag = false, g_evt_end_flag = false;
volatile bool g_rx_int_flag = false;

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
    ble_radiocntl1_pack(/*uint8_t  forceagcen*/      0,
            /*uint8_t  forceiq*/         0,
            /*uint8_t  rxdnsl*/          0,
            /*uint8_t  txdnsl*/          0,
            /*uint16_t forceagclength*/  0,
            /*uint8_t  syncpulsemode*/   0,
            /*uint8_t  syncpulsesrc*/    1,
            /*uint8_t  dpcorren*/        0,
            /*uint8_t  jefselect*/       1,
            /*uint8_t  xrfsel*/          2,
            /*uint8_t  subversion*/      0); //0x00005020

    /* BLE RADIOCNTL2 */
    ble_radiocntl2_pack(/*uint8_t  lrsynccompmode*/ 0,//0x3,
            /*uint8_t  rxcitermbypass*/ 0x0,
            /*uint8_t  lrvtbflush*/     0,//8,
            /*uint8_t  phymsk*/         1,//3,
            /*uint8_t  lrsyncerr*/      0,
            /*uint8_t  syncerr*/        0,
            /*uint16_t freqtableptr*/   EM_FT_OFFSET >> 2); //0xC8C00100

    /* BLE RADIOCNTL3 */
    ble_radiocntl3_pack(/*uint8_t rxrate3cfg*/    0x3,
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
            
    ble_radiocntl2_phymsk_setf(3); //enable 2Mbps/500Kpbs/125Kbps

    /* BLE RADIOPWRUPDN0: 1Mbps */
    ble_radiopwrupdn0_pack(/*uint8_t syncposition0*/ 0,
            /*uint8_t rxpwrup0*/      0x5a,
            /*uint8_t txpwrdn0*/      0x06,
            /*uint8_t txpwrup0*/      0x46);

    /* BLE RADIOPWRUPDN1: 2Mbps */
    ble_radiopwrupdn1_pack(/*uint8_t syncposition1*/ 0,
            /*uint8_t rxpwrup1*/      0x50,
            /*uint8_t txpwrdn1*/      0x06,
            /*uint8_t txpwrup1*/      0x50);

    /* BLE RADIOPWRUPDN2: 500Kbps/125Kbps */
    ble_radiopwrupdn2_pack(/*uint8_t syncposition2*/ 0,
            /*uint8_t rxpwrup2*/      0x5a,
            /*uint8_t txpwrdn2*/      0x06,
            /*uint8_t txpwrup2*/      0x5a);

    /* BLE RADIOPWRUPDN3: 500Kbps */
    ble_radiopwrupdn3_pack(/*uint8_t txpwrdn3*/      0x06,
            /*uint8_t txpwrup3*/      0x5a);

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

    /* BLE RADIOTXRXTIM2 - WR_32(0x50000098, 0x1003030B); */
    ble_radiotxrxtim2_pack(
        /*uint8_t rxflushpathdly2*/  0x03,
        /*uint8_t rfrxtmda2*/        0x03,
        /*uint8_t rxpathdly2*/       0x10,
        /*uint8_t txpathdly2*/      0x10);

    /* BLE RADIOTXRXTIM3 - WR_32(0x5000009C, 0x0403000B); */
    ble_radiotxrxtim3_pack(
        /*uint8_t rxflushpathdly3*/  0x03,
        /*uint8_t rfrxtmda3*/        0x03,
        /*uint8_t txpathdly3*/       0x10);                
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

void rf_em_init(void)
{
    memcpy((void *)(EM_BASE_ADDR + EM_FT_OFFSET), &freq_table[0], 40);
}

static uint8_t rf_freq2chnl(uint8_t freq)
{
    return freq_table[freq];
}

static void rf_reg_init(void)
{
    /* 1. reg_rf Init */


    /* 2. reg_mdm Init */
    MDM->REG0.Word         = 0x00000081;
    MDM->REG1.Word         = 0x03E83474;
    
    //16000003.56, ppm:0.22

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
        chnl/*rf_freq2chnl(chnl)*//*chnl*/ /*ch_idx*/, 7 /*hopint*/, 0 /*hop_mode*/, 0 /*fh_en*/,
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
//    RCC->BLECFG.BLE_MASTER_RST = 1;
//    RCC->BLECFG.BLE_MASTER_RST = 0;
    RF->RF_RSV &= ~(0x01UL << 15);
    g_tx_flag = false;

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
    RF->RF_RSV |= (0x01UL << 15);
    
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
    
    g_tx_flag = true;
}

void rf_stop_test(void)
{
    g_tx_flag = false;
    g_rx_int_flag = false;

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
    
    rf_em_init();
    rf_core_init();
    rf_reg_init();
    
    // Re-enable force AGC mechanism
    ble_radiocntl1_forceagc_en_setf(1);

#if (WHIT_DISABLE)
    ble_blecntl_whit_dsb_setf(1);
#else
    // Enable the whitening
    ble_blecntl_whit_dsb_setf(0);
#endif

//    DEBUG("CS_OFFSET:0x%x", EM_BLE_CS_OFFSET);
//    DEBUG("RX_OFFSET:0x%x", EM_BLE_RX_DESC_OFFSET);
}

static uint8_t rf_rx_result(void)
{
    uint8_t rxDataCnt = 0;
    NVIC_DisableIRQ(BLE_IRQn);

    if ((RD_32(0x500000D0) >> 27) & 0x01)
        rxDataCnt = RD_32(0x500000D8);
    else
        rxDataCnt = 0;

    return rxDataCnt;
}

static uint8_t rf_tx_result(void)
{
    uint16_t tx_desc = *(volatile uint16_t *)(TX_DESC_ADDR + 2);
//    NVIC_DisableIRQ(BLE_IRQn);
    
    tx_desc >>= 8;
    
    if ((RD_32(0x500000D0) >> 11) & 0x01)
        tx_desc = RD_32(0x500000D4);
    else
        tx_desc = 0;
    
    return tx_desc;
}

// freq: Tx:0x21~0x48(2402~2480MHz), Rx:0x81~0xA8(2402~2480MHz) --> chnl: 0~39
// pkt_len: 37~255
uint8_t rf_test(uint8_t freq, uint8_t pkt_len)
{
//    WR_32(0x50000050, 0x00B2);
//    ioFuncSel(2, 9); //tx_en
//    ioFuncSel(3, 9); //rx_en
    uint8_t result;
    ioBleTxRx(4, 5);
    rf_init_test();

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
            chnl = rf_freq2chnl(chnl - 1);
        }
        
        if (pkt_len < 37)
            pkt_len = 37;
        
        if (freq & 0x80)
        {
            #if (AUX_TEST_CHIP)
            rf_stop_test();
            
            for (uint16_t i = 0; i < 20; i++)
            { // packet count
                rf_tx_test(chnl, RATE_1Mbps, pkt_len, PAYL_PRBS9);
                sysTickDelay(16*1250); // 1.2ms
                ble_rwblecntl_rftest_abort_setf(1);
            }
            sysTickDelay(16*1000);
            #else
            g_rx_cnt = 0;
            g_rx_err = 0;

            bootDelayMs(1);
            rf_rx_test(chnl, RATE_1Mbps, pkt_len);
            NVIC_EnableIRQ(BLE_IRQn);
            __enable_irq();
            bootDelayMs(32);
            ble_blecntl_rftest_abort_setf(1);
            result = rf_rx_result();
//            printf("freq:%x, rxDataCnt:%d, rx_cnt:%d, rx_err:%d\r\n", freq, result, g_rx_cnt, g_rx_err);
            #endif
        }
        else
        {
            #if (AUX_TEST_CHIP)
            rf_stop_test();
            rf_rx_test(chnl, RATE_1Mbps, pkt_len);
            NVIC_EnableIRQ(BLE_IRQn);
            __enable_irq();
            sysTickDelay(16 * 28000); //28ms
            ble_rwblecntl_rftest_abort_setf(1);
            result = rf_rx_result();
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
            bootDelayMs(9);
            rf_tx_cmw500(chnl, RATE_1Mbps, pkt_len, PAYL_PRBS9);

            // 等待发送指定数量
            while (RD_32(0x500000D4) < RF_PKT_CNT)
            {
            }
            ble_blecntl_rftest_abort_setf(1);
            #endif
//            result = rf_tx_result();
            #endif
        }
        
        if (result > 17 && result < 21)
            FT_RET(T_OK, freq, result);
        else
            FT_RET(T_FAIL, freq, result);
        
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

/************************************************/
int fputc(int ch, FILE *f) {
    // Remap printf(...) to UART
    uart_putc(0, ch);
    return ch;
}
/************************************************/
