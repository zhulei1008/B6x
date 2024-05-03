#include "drvs.h"
#include "regs.h"
//#include "rf_test.h"
#include "CRCxx.h"

#if (DEBUG_RF_MDM)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<RF_MDM>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
#endif

#if !defined(PA_TARGET)
#define PA_TARGET 0x0C
#endif

#define DMA_CH_MDM_TX         0
#define DMA_CH_MDM_RX         1
#define MDM_TX_DATA_LEN       42
#define MDM_RX_DATA_LEN       37

const uint8_t rf_mdm_tx_data[MDM_TX_DATA_LEN] = 
{
    0x55,
    0x29, 0x41, 0x76, 0x71,
    0x0c,
    0x20,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0xce, 0x3d, 0x44
};

const uint8_t rf_mdm_tx_payload[MDM_TX_DATA_LEN] = 
{
    [0 ... (MDM_TX_DATA_LEN-1)] = 0x0F
};

uint8_t rf_mdm_rx_data[MDM_RX_DATA_LEN] = {0};

void rf_mdm_tx_start(uint8_t rf_chan, uint8_t rf_rate)
{
    RF->PLL_DYM_CTRL.Word = 0;
    
//    rf_reg_spi_tx_en(rf_chan, rf_rate);
//    nop_delay(100);

    RF->PLL_DYM_CTRL.Word = rf_chan | (rf_rate << RF_SW_RATE_LSB) | (0x01UL << RF_SW_TX_EN_POS) | (PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB);
}

void rf_mdm_tx_stop(void)
{
    while (!MDM->EXT_ST.MDM_TX_DONE)
    {
        
    }
    
//    rf_reg_spi_tx_dis();
    
    RF->PLL_DYM_CTRL.Word &= ~(0x01UL << RF_SW_TX_EN_POS);
    
    dma_chnl_done(DMA_CH_MDM_TX);
}

volatile bool rxChnlAlt  = false;
volatile bool txChnlBusy = false;

static void dmaUartRxDone(void)
{
    rxChnlAlt = dma_chnl_reload(DMA_CH_MDM_TX);
    if (rxChnlAlt)
    {
        // head to Pong
//        rxdHead = RXD_BUFF_HALF;
//        // Ping done pulse
//        GPIO_DAT_SET(GPIO_RX_PING);
//        GPIO_DAT_CLR(GPIO_RX_PING);
    }
    else
    {
        // head to Ping
//        rxdHead = 0;
//        // Pong done pulse
//        GPIO_DAT_SET(GPIO_RX_PONG);
//        GPIO_DAT_CLR(GPIO_RX_PONG);
    }
}

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;
    
//    GPIO_DAT_SET(GPIO_RUN);
    
    // disable intr
    DMACHCFG->IEFR0 &= ~iflag;
    // clear intr flag
    DMACHCFG->ICFR0 = iflag;
    
    if (iflag & (1UL << DMA_CH_MDM_TX))
    {
        dmaUartRxDone();
    }
    
//    if (iflag & (1UL << DMA_CH_UART_TX))
//    {
//        txChnlBusy = false;
//        GPIO_DAT_SET(GPIO_TX_DONE);
//        GPIO_DAT_CLR(GPIO_TX_DONE);
//    }
    
    // re-enable intr
    DMACHCFG->IEFR0 |= iflag;
    
//    GPIO_DAT_CLR(GPIO_RUN);
}

void rf_mdm_init(void)
{
    RF->DIG_CTRL.FSM_CTRL_SEL  = 1;
    
    MDM->CRC_PRESET               = 0x555555;
    MDM->ACCESS_REG               = 0x71764129;
    
    MDM->REG0.ACC_REG_EN          = 1;
    
    MDM->EXT_CTRL.MDM_INT_EN      = 0x16;
    MDM->EXT_CTRL.MDM_EXT_EN      = 1;
    
    MDM->SLOT_SET.MDM_SLOT_OFF    = 100;
    MDM->SLOT_SET.MDM_SLOT_WIN    = 511;

    MDM->RXSYNC_WIN               = 0xFFF;
    
    //rf_mdm_dma_init();
    dma_init();
    DMA_MDM_TX_INIT(DMA_CH_MDM_TX);
    DMA_MDM_RX_INIT(DMA_CH_MDM_RX);

//    // init dma chnl
//    DMA_UARTx_TX_INIT(DMA_CH_UART_TX, 1);
//    DMA_UARTx_RX_INIT(DMA_CH_UART_RX, 1);
    // config RX chnl
//    DMA_MDM_TX_CONF(DMA_CH_MDM_TX, rf_mdm_tx_payload, MDM_TX_DATA_LEN>>1, CCM_PING_PONG);
//    DMA_MDM_TX_CONF(DMA_CH_MDM_TX | DMA_CH_ALT, (rf_mdm_tx_payload+(MDM_TX_DATA_LEN>>1)), MDM_TX_DATA_LEN>>1, CCM_PING_PONG);
//    DMA_MDM_TX_CONF(DMA_CH_MDM_TX, rf_mdm_tx_data, MDM_TX_DATA_LEN, CCM_BASIC);
//    DMA_MDM_TX_CONF(DMA_CH_MDM_TX | DMA_CH_ALT, (rf_mdm_tx_payload+(MDM_TX_DATA_LEN>>1)), MDM_TX_DATA_LEN>>1, CCM_PING_PONG);
//    DMA_UARTx_RX_CONF(DMA_CH_UART_RX, 1, &rxdBuffer[0], RXD_BUFF_HALF, CCM_PING_PONG);
//    DMA_UARTx_RX_CONF(DMA_CH_UART_RX | DMA_CH_ALT, 1, &rxdBuffer[RXD_BUFF_HALF], RXD_BUFF_HALF, CCM_PING_PONG);

    // enable irq
//    DMACHCFG->IEFR0 = (1UL << DMA_CH_MDM_TX);
//    NVIC_EnableIRQ(DMAC_IRQn);  
    NVIC_EnableIRQ(MDM_IRQn);
    __enable_irq();
}

void rf_mdm_tx(uint8_t rf_chan, uint8_t rf_rate, uint8_t *data, uint8_t data_len)
{    
    rf_rate &= 0x01;
    
    RF->PLL_DYM_CTRL.Word = 0;
    
    // 7fixed data + 1 len + 3 CRC
    uint16_t pkt_len = 10 + data_len + rf_rate;
    uint8_t pkt[pkt_len];
    uint32_t crc_result = 0;
    
    // 1Mbps 1PREAMBLE, 2Mbps 2PREAMBLE
    // 2 PREAMBLE + 4 Access Code + 1 Type
    uint8_t pkt_fixed_data[7]  = {0x55, 0x55, 0x29, 0x41, 0x76, 0x71, 0x0C};

    memcpy(pkt, pkt_fixed_data + 1 - rf_rate, 6 + rf_rate);
    pkt[6 + rf_rate] = data_len;
    memcpy(pkt + 7 + rf_rate, data, data_len);

    // crc计算除preamble和access code之外的数据
    // data_len + 2(type+data_len)

    crc_result = crc24_ble(pkt + 5 + rf_rate, data_len + 2);
    pkt[pkt_len-3] = ((crc_result >> 0)  & 0xFF);
    pkt[pkt_len-2] = ((crc_result >> 8)  & 0xFF);
    pkt[pkt_len-1] = ((crc_result >> 16) & 0xFF);
    DEBUG("crc_result:%x, pkt_len:%d", crc_result, pkt_len);
    debugHex(pkt, pkt_len);

    DMA_MDM_TX_CONF(DMA_CH_MDM_TX, pkt, pkt_len, CCM_BASIC);

//    MDM->EXT_TX_DAT = 0x55;
//    rf_reg_spi_tx_en(rf_chan, rf_rate);
//    nop_delay(1200);

    RF->PLL_DYM_CTRL.Word = rf_chan | (rf_rate << RF_SW_RATE_LSB) | (0x01UL << RF_SW_TX_EN_POS) | (PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB);
    
    while (!MDM->EXT_ST.MDM_TX_DONE)
    {
        
    }
    
    RF->PLL_DYM_CTRL.Word = 0;
    
//    rf_reg_spi_tx_dis();

    dma_chnl_done(DMA_CH_MDM_TX);
}

void rf_mdm_rx(uint8_t rf_chan, uint8_t rf_rate)
{
    DMA_MDM_RX_CONF(DMA_CH_MDM_RX, rf_mdm_rx_data, MDM_RX_DATA_LEN, CCM_BASIC);
    
    RF->PLL_DYM_CTRL.Word      = 0;
    
//    rf_reg_spi_rx_en(rf_chan, rf_rate);

    RF->PLL_DYM_CTRL.Word = rf_chan | (rf_rate << RF_SW_RATE_LSB) | (0x01UL << RF_SW_RX_EN_POS);
    
    while (!(dma_chnl_done(DMA_CH_MDM_RX)))
    {
        
    }
    
//    while (!MDM->EXT_ST.MDM_SYNC_ERR)
//    {
//        
//    }

//    rf_reg_spi_rx_dis();
    
    RF->PLL_DYM_CTRL.Word = 0;
     
    debugHex(rf_mdm_rx_data, MDM_RX_DATA_LEN);
}

#define MDM_SLOT_INT_BIT   0x04UL
#define MDM_SLOT_OVER_BIT  0x08UL
#define MDM_SYNC_FOUND_BIT 0x10UL

void MDM_IRQHandler(void)
{
    uint32_t irq_stat = MDM->EXT_ST.Word;
    
    if (irq_stat & MDM_SLOT_INT_BIT)
    {
        MDM->EXT_CTRL.MDM_SLOT_INT_CLR = 1;
    }
    
    if (irq_stat & MDM_SLOT_OVER_BIT)
    {
        MDM->EXT_CTRL.MDM_SLOT_INT_CLR = 1;
    }
    
    if (irq_stat & MDM_SYNC_FOUND_BIT)
    {
        RF->PLL_DYM_CTRL.Word = 0;
    }
}
