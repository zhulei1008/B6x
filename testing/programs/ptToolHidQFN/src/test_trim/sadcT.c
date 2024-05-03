/**
 ****************************************************************************************
 *
 * @file sadc.c
 *
 * @brief Successive Approximation A/D Converter (SADC) Driver
 *
 ****************************************************************************************
 */

#include "reg_sadc.h"
#include "sadc.h"
#include "rcc.h"
#include "gpio.h"
#include "iopad.h"
#include "reg_rf.h"
#include "reg_timer.h"

/*
 * DEFINES
 ****************************************************************************************
 */


// clear last flag, start conversion
#define SADC_START()                           \
    dowl(                                      \
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;     \
        SADC->CTRL.SADC_SOC = 1;               \
    )

// wait aux_flag is 1 (conversion done), clear flag
#define SADC_AFLG_CLR()    dowl( SADC->STCTRL.SADC_AUX_FLG_CLR = 1; )
#define SADC_AFLG_WAIT()   dowl( while (!(SADC->STCTRL.SADC_AUX_FLG)); )

#define SADC_CALIB_CNT     (8)
#define SADC_CTRL_MSK      (SADC_CR_CALIB_BIT | SADC_CR_CONV_MODE_BIT | SADC_CR_DMAEN_BIT \
                            | SADC_CR_SAMP_MODE_MSK | SADC_CR_DBGEN_BIT)


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void sadc_calib(void)
{
//    uint32_t sel;
    
    // .SADC_EN=1
//    SADC->SADC_ANA_CTRL.SADC_EN = 1;
    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;
    
    // .Calib_mode=1, .samp_mod=0 .Dbg_ctrl=0
    SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_CALIB_BIT | SADC_CR_SAMP_SOFT);
    //debug("1-SADC(CR:%X,ANA:%X,MIC:%X,ST:%X)\r\n", SADC->CTRL.Word, SADC->SADC_ANA_CTRL.Word, SADC->MIC_CTRL.Word, SADC->STCTRL.Word);

    // SADC_START();
    SADC->CTRL.SADC_SOC = 1;
    SADC_AFLG_WAIT();
    
//    for (sel = 0; sel < SADC_CALIB_CNT; sel++)//   --20230601
//    {
//        SADC->SADC_CALIB_DATSEL = sel;
//        SADC->SADC_CALIB_DATOUT; // read
//        //debug("CALIB DATOUT:%X\r\n", SADC->SADC_CALIB_DATOUT);
//    }
    
    SADC_AFLG_CLR();
    
    // disable calibration
    SADC->CTRL.Word &= ~SADC_CR_CALIB_BIT;
}

void sadc_init(uint32_t ana_ctrl)
{
    RCC_AHBCLK_DIS(AHB_ADC_BIT);
    RCC_AHBRST_REQ(AHB_ADC_BIT);
    RCC_AHBCLK_EN(AHB_ADC_BIT);
    
    SADC->SADC_ANA_CTRL.Word = ana_ctrl;

    sadc_calib();
}

void sadc_conf(uint32_t ctrl)
{
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;
    SADC->CTRL.Word = ctrl;
}

uint16_t sadc_read(uint8_t chset, uint16_t times)
{
    uint16_t dout;
    
    // .SADC_EN=1
//    SADC->SADC_ANA_CTRL.SADC_EN = 1;
    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;
    
    // .Calib_mode=0, .conv_mode=1 or 0, .sadc_dmac_en=0, .samp_mod=0 .Dbg_ctrl=0
    uint16_t cr_val = (times > 0) ? (SADC_CR_CONV_CONTINUE | SADC_CR_SAMP_SOFT) : (SADC_CR_CONV_SINGLE | SADC_CR_SAMP_SOFT);
    SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | cr_val;
    // .auto_sw_ch=0, Set sadc_ch_set0
    SADC->AUTO_SW_CTRL.Word = 0;
    SADC->CH_CTRL.SADC_CH_SET0 = chset;
    
    //Clear
    SADC_AFLG_CLR();
    
    // SADC_START();
    SADC->CTRL.SADC_SOC = 1;
    SADC_AFLG_WAIT();
    dout = SADC->AUX_ST.SADC_AUX_DOUT;
    SADC_AFLG_CLR();
    
    if (times > 0)
    {
        for (uint16_t i = 1; i < times; i++)
        {
//            SADC_AFLG_WAIT();
            dout += SADC->AUX_ST.SADC_AUX_DOUT;
//            SADC_AFLG_CLR();
        } 
        
        dout /= times;
        // Stop continuous mode .conv_mode=0 .sadc_soc=1
        SADC->CTRL.SADC_CONV_MODE = 0;
        SADC->CTRL.SADC_SOC = 1;
        SADC_AFLG_CLR();          
    }
    
    return dout;
}

void sadc_stop(void)
{
    // Stop continuous mode .conv_mode=0 .sadc_soc=1
    SADC->CTRL.SADC_CONV_MODE = 0;
    SADC->CTRL.SADC_SOC = 1;
    SADC_AFLG_CLR();
}

void sadc_dma(uint8_t sw_auto, uint32_t ch_ctrl)
{
    if (sw_auto < 0x10)
    {
        // .SADC_EN=1
//        SADC->SADC_ANA_CTRL.SADC_EN = 1;
        // .MIC_PD=1, .PGA_EN=0
        SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;
        
        // .Calib_mode=0, .conv_mode=1, .sadc_dmac_en=1, .samp_mod=0 .Dbg_ctrl=0
        SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_CONV_CONTINUE | SADC_CR_DMAEN_BIT | SADC_CR_SAMP_SOFT);
        // .auto_sw_ch=0 or 1, Set sadc_ch_set0
        SADC->AUTO_SW_CTRL.Word = sw_auto;
        SADC->CH_CTRL.SADC_CH_SET0 = ch_ctrl;
        
        // SADC_START();
        SADC->CTRL.SADC_SOC = 1;
    }
    else
    {
        // Stop continuous mode .conv_mode=0 .sadc_soc=1
        SADC->CTRL.SADC_CONV_MODE = 0;
        SADC->CTRL.SADC_SOC = 1;
    }
}

void sadc_rssi(uint8_t rf_rsv)
{
    // .SADC_EN=1
//    SADC->SADC_ANA_CTRL.SADC_EN = 1;
    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;
    
    // .Calib_mode=0, .conv_mode=0, .sadc_dmac_en=0, .samp_mod=1 .Dbg_ctrl=0
    SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_SAMP_RSSI);
    // .auto_sw_ch=0, .sadc_ch_set0=15
    SADC->AUTO_SW_CTRL.Word = 0;
    SADC->CH_CTRL.SADC_CH_SET0 = SADC_CH_RFRSV;
    RF->RF_RSV = rf_rsv;
    
    // Set sadc_rssi_samp_dly sadc_aux_clk_div=16M
    // no need software set sadc_soc to start
}

void sadc_pcm(uint32_t mic_sel)
{
    if (mic_sel)
    {
        // micbias and micin io configure
        GPIO_DIR_CLR(GPIO02 | GPIO03);
        iom_ctrl(PA02, IOM_HIZ);
        iom_ctrl(PA03, IOM_ANALOG);
        
        // .SADC_EN=1
//        SADC->SADC_ANA_CTRL.SADC_EN = 1;
        // .MIC_PD=0, .PGA_EN=1; Set PGA_VOL
        SADC->MIC_CTRL.Word = mic_sel;
        SADC->DC_OFFSET = 0x200;
        
        // .Calib_mode=0, .conv_mode=0, .sadc_dmac_en=1, .samp_mod=2 .Dbg_ctrl=0
        SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_DMAEN_BIT | SADC_CR_SAMP_PCM);

        // .auto_sw_ch=0, .sadc_ch_set0=4
        SADC->AUTO_SW_CTRL.Word = 0;
        SADC->CH_CTRL.SADC_CH_SET0 = SADC_CH_MICIN;
        
        // SADC_START();
        SADC->CTRL.SADC_SOC = 1;
    }
    else
    {
        // Stop decimation filter mode
        SADC->CTRL.SADC_DECIM_END = 1;
    }
}

void sadc_adtmr(uint8_t sw_auto, uint32_t ch_ctrl)
{
    // Set ADTIM work as basic timer and trigger source is uevent (mms = 3'b010)
    ADTMR1->CR2.MMS = 2;
    
    // .SADC_EN=1
//    SADC->SADC_ANA_CTRL.SADC_EN = 1;
    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;
    
    // .Calib_mode=0, .conv_mode=0, .sadc_dmac_en=1, .samp_mod=3 .Dbg_ctrl=0 .sadc_aux_clk_div=0
    SADC->CTRL.Word = (SADC->CTRL.Word & ~(SADC_CTRL_MSK | SADC_CR_CLK_DIV_MSK)) | (SADC_CR_DMAEN_BIT | SADC_CR_SAMP_ADTMR);
    // .auto_sw_ch=0 or 1, Set sadc_ch_set0
    SADC->AUTO_SW_CTRL.Word = sw_auto;
    SADC->CH_CTRL.SADC_CH_SET0 = ch_ctrl;
    
    // SADC_START();
    SADC->CTRL.SADC_SOC = 1;
}

