#include "drvs.h"
#include "regs.h"
#include "utils.h"
#include "uartRb.h"
#include "flash_test.h"

#if (DBG_FSH_TEST)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#undef debugHex
#define debugHex(dat,len)     do{                                 \
                                  for (int i=0; i<len; i++){      \
                                      debug("%08X ", dat[i]); \
                                  }                               \
                                  debug("\r\n");                  \
                              } while (0)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

//#define FSH_SUS_TIME 9600//(32*30)
//#define FSH_RSM_TIME 96000//(32*3000)
#define FSH_SUS_TIME 960//(32*30)
#define FSH_RSM_TIME 96000//(32*3000)
//#define FSH_SUS_TIME (1290)//(43*30)
//#define FSH_RSM_TIME (129000)//(43*3000)
                              
enum flash_test_uart_cmd
{
    CMD_READ         = 0x10,
    CMD_DIR_READ,    
    CMD_WRITE,

    CMD_PAGE_ERASE   = 0x20,
    CMD_SECTOR_ERASE,
    CMD_CHIP_ERASE,
    
    CMD_QREAD        = 0x30,
    CMD_QWRITE,
    
    CMD_READ_OTP     = 0x40,
    CMD_ERASE_OTP,
    CMD_WRITE_OTP,
    
    CMD_READ_STA     = 0x50,
    CMD_WRITE_STA,
    
    CMD_ENTER_HPM    = 0x60,
    CMD_EXIT_HPM,
    
    CMD_SUSPEND_EN   = 0x70,
    
    CMD_TRIM_REG_VAL = 0x80,
    
    CMD_CHIP_RST     = 0x90,
    
    CMD_CAP_DLY      = 0xA0,
    
    CMD_RELEASE_HPM  = 0xB0,
    
    CMD_EM_TRIM      = 0xC0,
    
    CMD_FSH_CLK      = 0xD0,
};


/// Init Uart
void uartInit(void)
{
#if (DBG_MODE == 1)
    // Be inited via dbgInit()
#else
    uart_init(UART_PORT, GPIO_TX, GPIO_RX);
    uart_conf(UART_PORT, BRR_115200, LCR_BITS_DFLT);
    
    uartReceiveFIFOSet(UART_PORT, UART_RX_FIFO_TRIGGER_LVL_8BYTE);
    uartReceiveTimeOutSet(UART_PORT, 20);
    uartITEnable(UART_PORT, (UART_IT_RXRD | UART_IT_RTO));    
#endif
    
    NVIC_EnableIRQ(UART1_IRQn);
}

#define FSH_BUFF_LEN 128
uint32_t fsh_buff[FSH_BUFF_LEN] = {0};
#define CMD_MAX_LEN 20
#define NULL_CNT 60
static uint8_t buff[CMD_MAX_LEN];
static uint16_t buff_len = 0;

__SRAMFN void by_quad_mode(void)
{
//    write en singal
    fshc_en_cmd(FSH_CMD_WR_EN);
//    write state enable for write flash state
//    fshc_en_cmd(FSH_CMD_WR_STA_EN);
//    send write sta cmd
    fshc_wr_sta(0x31, 1, 2);
    bootDelayMs(20);
}

void uart_proc(void)
{
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart1Rb_Read(&buff[buff_len], CMD_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < CMD_MAX_LEN)
        {
            return; // wait full
        }
    }
    else
    {
        if ((buff_len > 0) && (null_cnt++ > NULL_CNT))
        {
            //finish = true;
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }
    
    DEBUG("cmd:%02x, flash_st_done:%d", buff[0], FSHC->ST.FLASH_ST_DONE);
    if (FSHC->ST.FLASH_ST_DONE)
    {
//        GPIO->DAT_SET = 0x04;
        FSHC->SPCR.FLASH_BUSY_CLR = 1;
//        GPIO->DAT_CLR = 0x04;
    }
    DEBUG("cmd:%02x, flash_st_done:%d", buff[0], FSHC->ST.FLASH_ST_DONE);
    
    switch (buff[0])
    {
        case CMD_READ:
        {
            if (buff_len >= 6)
            {
                uint32_t offset  = read32p(buff + 1);
                uint8_t read_len = *(buff + 5);
                uint16_t fcmd    = FSH_CMD_RD;
                DEBUG("READ, offset:%x, len:%x", offset, read_len);

                fshc_read(offset, fsh_buff, read_len, fcmd);
                debugHex(fsh_buff, read_len);
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;

        case CMD_DIR_READ:
        {
            for(uint8_t i = 0; i < 64; ++i)
            {
                fsh_buff[i] = RD_32(0x18008000 + (i << 2));
            }
            debugHex(fsh_buff, 64);
        } break;
        
        case CMD_WRITE:
        {
            if (buff_len >= 7)
            {
                for (uint8_t i = 0; i < 64; ++i)
                {
                    fsh_buff[i] = 0x30303030 + (0x01010101 * i);
                }
                
                uint32_t offset      = read32p(buff + 1);
                uint8_t wr_len       = *(buff + 5);
                uint8_t suspend_cfg  = *(buff + 6);
                DEBUG("Write, offset:%x, len:%d, suspend_cfg:%d", offset, wr_len, suspend_cfg);
                DEBUG("AUTO_CHECK_ST:%x, FLASH_BUSY_SET:%d", FSHC->SPCR.AUTO_CHECK_ST, FSHC->SPCR.FLASH_BUSY_SET);
                
                uint16_t fcmd = FSH_CMD_WR;
                if (suspend_cfg)
                {
                    fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
                    FSHC->SPCR.FLASH_BUSY_SET = 1;
                }
                fcmd |= (suspend_cfg ? FCM_SUSPEND_BIT: 0);
                
//                debugHex(fsh_buff, wr_len);
//                while (SYSCFG->ACC_CCR_BUSY);
                fshc_write(offset, fsh_buff, wr_len, fcmd);

                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;

        case CMD_PAGE_ERASE:
        {
            if (buff_len >= 6)
            {
                uint32_t offset      = read32p(buff + 1);
                uint8_t suspend_cfg  = *(buff + 5);
                uint16_t fcmd        = FSH_CMD_ER_PAGE;
                DEBUG("PageErase, offset:%x, suspend_cfg:%d", offset, suspend_cfg);
                DEBUG("AUTO_CHECK_ST:%x, FLASH_BUSY_SET:%d", FSHC->SPCR.AUTO_CHECK_ST, FSHC->SPCR.FLASH_BUSY_SET);
                if (suspend_cfg)
                {
                    fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
                    FSHC->SPCR.FLASH_BUSY_SET = 1;
                }
                fcmd |= (suspend_cfg ? FCM_SUSPEND_BIT: 0);
//                while (SYSCFG->ACC_CCR_BUSY);
                
                fshc_erase(offset, fcmd);
                DEBUG("AUTO_CHECK_ST:%x, FLASH_BUSY_SET:%d, fcmd:%x", FSHC->SPCR.AUTO_CHECK_ST, FSHC->SPCR.FLASH_BUSY_SET, fcmd);
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;
        
        case CMD_SECTOR_ERASE:
        {
            if (buff_len >= 6)
            {
                uint32_t offset      = read32p(buff + 1);
                uint8_t suspend_cfg  = *(buff + 5);
                uint16_t fcmd        = FSH_CMD_ER_SECTOR;
                DEBUG("SectorErase, offset:%x, suspend_cfg:%d", offset, suspend_cfg);
                if (suspend_cfg)
                {
                    fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
                }
                
                fcmd |= (suspend_cfg ? FCM_SUSPEND_BIT: 0);
                
                fshc_erase(offset, fcmd);
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;
        
        case CMD_CHIP_ERASE:
        {
            DEBUG("ChipErase");
            fshc_erase(0, FCM_ERCHIP_BIT|0x160);
        } break;

        case CMD_QREAD:
        {
            if (buff_len >= 6)
            {
                uint32_t offset  = read32p(buff + 1);
                uint8_t read_len = *(buff + 5);
                uint16_t fcmd    = FCM_MODE_QUAD | FSH_CMD_QDRD;
                DEBUG("QRead, offset:%x, len:%x", offset, read_len);
                fshc_quad_mode(PUYA_QUAD);
                fshc_read(offset, fsh_buff, read_len, fcmd);
                debugHex(fsh_buff, read_len);
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;
        
        case CMD_QWRITE:
        {
            if (buff_len >= 7)
            {
                for (uint8_t i = 0; i < 64; ++i)
                {
                    fsh_buff[i] = 0x30303030 + (0x01010101 * i);
                }
                
                uint32_t offset      = read32p(buff + 1);
                uint8_t wr_len       = *(buff + 5);
                uint8_t suspend_cfg  = *(buff + 6);
                DEBUG("QWrite, offset:%x, suspend:%d", offset, suspend_cfg);
                
                uint16_t fcmd = FCM_MODE_QUAD | FSH_CMD_QDWR;
                if (suspend_cfg)
                {
                    fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
                }
                fcmd |= (suspend_cfg ? FCM_SUSPEND_BIT: 0);
                
//                debugHex(fsh_buff, wr_len);
                fshc_quad_mode(PUYA_QUAD);
                fshc_write(offset, fsh_buff, wr_len, fcmd);

                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;
        
        case CMD_READ_OTP:
        {
            if (buff_len >= 6)
            {
                uint32_t offset  = read32p(buff + 1);
                uint8_t read_len = *(buff + 5);
                uint16_t fcmd    = FCM_RWOTP_BIT | FSH_CMD_RD_OTP;
                DEBUG("Read OTP, offset:%x, len:%x", offset, read_len);
                
                fshc_read(offset, fsh_buff, read_len, fcmd);
                debugHex(fsh_buff, read_len);
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;

        case CMD_ERASE_OTP:
        {
            if (buff_len >= 6)
            {
                uint32_t offset      = read32p(buff + 1);
                uint8_t suspend_cfg  = *(buff + 5);
                uint16_t fcmd        = FSH_CMD_ER_OTP;
                DEBUG("EraseOTP, offset:%x, suspend_cfg:%d", offset, suspend_cfg);
                if (suspend_cfg)
                {
                    // suspendÒì³£.
                    fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
                }
                fcmd |= (suspend_cfg ? FCM_SUSPEND_BIT: 0);
                
                fshc_erase(offset, fcmd);
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;
        
        case CMD_WRITE_OTP:
        {
            if (buff_len >= 8)
            {
                fsh_buff[5] = 0xC8F5D5B6;
                fsh_buff[4] = 0x6C1DCE7A;
                fsh_buff[3] = 0x3A00FE3A;
                fsh_buff[2] = 0x00;
                fsh_buff[1] = 0x00;
                fsh_buff[0] = 0x1CA;
                
                uint32_t offset      = read32p(buff + 1);
                uint8_t wr_len       = *(buff + 5);
                uint8_t suspend_cfg  = *(buff + 6);
                uint8_t invalid_flag = *(buff + 7);
                
                if (invalid_flag)
                {
                    fsh_buff[5] = 0x00;
                }
                DEBUG("Write OTP, offset:%x, suspend:%d, invalid_flag:%d", offset, suspend_cfg, invalid_flag);
                
                uint16_t fcmd = FCM_RWOTP_BIT | FSH_CMD_WR_OTP;
                if (suspend_cfg)
                {
                    fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
                }
                fcmd |= (suspend_cfg ? FCM_SUSPEND_BIT: 0);
                
//                debugHex(fsh_buff, wr_len);
                fshc_quad_mode(PUYA_QUAD);
                fshc_write(offset, fsh_buff, wr_len, fcmd);

                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;

        case CMD_READ_STA:
        {
            uint32_t sta = 0;
            
            sta = fshc_rd_sta(0x35, 1);
            sta <<= 8;
            sta |= fshc_rd_sta(0x05, 1);
            DEBUG("sta:%x", sta);
            
            sta = fshc_rd_sta(0x9F, 3);
            DEBUG("id:%x", sta);
        } break;
        
        case CMD_WRITE_STA:
        {
            fshc_quad_mode(*(buff + 1));
        } break;
        
        case CMD_ENTER_HPM:
        {
            DEBUG("CACHE_EN:%d, BYPASS_HPM:%x", CACHE->CCR.CACHE_EN, FSHC->BYPASS_HPM);
//            fshc_quad_mode(PUYA_QUAD);

            by_quad_mode();
            
            puya_enter_hpm();
            DEBUG("CACHE_EN:%d, BYPASS_HPM:%x", CACHE->CCR.CACHE_EN, FSHC->BYPASS_HPM);
        } break;
        
        case CMD_EXIT_HPM:
        {
            DEBUG("CACHE_EN:%d, BYPASS_HPM:%x", CACHE->CCR.CACHE_EN, FSHC->BYPASS_HPM);
            puya_exit_hpm();
            DEBUG("CACHE_EN:%d, BYPASS_HPM:%x", CACHE->CCR.CACHE_EN, FSHC->BYPASS_HPM);
        } break;
        
        case CMD_SUSPEND_EN:
        {
            uint8_t fshc_clk = rcc_fshclk_mhz();
            fshc_clk = 32;
            fshc_suspend_conf(FSH_CMD_SUSPEND, FSH_CMD_RESUME, FSH_SUS_TIME, FSH_RSM_TIME);
            DEBUG("FSHC Freq:%d, clk_src:%d", fshc_clk, RCC->CLK_EN_ST.FSHCCLK_SEL);
        } break;

        case CMD_TRIM_REG_VAL:
        {
//            DEBUG("CORELDO_TRIM_RUN:%x", AON->BKHOLD_CTRL.CORELDO_TRIM_RUN);
//            DEBUG("CORELDO_TRIM_DP:%x ", APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP);
//            DEBUG("CORELDO_TRIM_RUN:%x", AON->BKHOLD_CTRL.AONLDO_TRIM_RUN);
//            DEBUG("CORELDO_TRIM_DP:%x ", APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF);
//            
//            DEBUG("RC16M_FREQ_TRIM:%x ", APBMISC->RC16M_FREQ_TRIM);
//            DEBUG("LDO_XOSC_TR:%x     ", APBMISC->XOSC16M_CTRL.LDO_XOSC_TR);
//            
//            DEBUG("XOSC16M_CAP:%x     ", APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR);
            DEBUG("AUTO_CHECK_ST:%x, FLASH_BUSY_SET:%d, DONE:%d", FSHC->SPCR.AUTO_CHECK_ST, FSHC->SPCR.FLASH_BUSY_SET, FSHC->ST.FLASH_ST_DONE);
                
            if (FSHC->ST.FLASH_ST_DONE)
            {
                FSHC->SPCR.FLASH_BUSY_CLR = 1;
            }
        } break;
        
        case CMD_CHIP_RST:
        {
            DEBUG("ChipReset");
//            RCC->CHIP_RSTREQ = 1;
            NVIC_SystemReset();
        } break;
        
        case CMD_CAP_DLY:
        {
            DEBUG("Flash CapDelay:%d", FSHC->DLY_CFG);
            uint8_t cap_dly = fshc_capdly_cfg(0xFF);
            DEBUG("Flash Auto CapDelay:%d", cap_dly);
        } break;
        
        case CMD_RELEASE_HPM:
        {
            DEBUG("release_hpm");
            fshc_en_cmd(FSH_CMD_EXIT_HMP);
            
            fshc_en_cmd(FSH_CMD_RST_EN);
            fshc_en_cmd(FSH_CMD_RESET);
        } break;
        
        case CMD_EM_TRIM:
        {
            #define EM_TRIM_ADDR         0x20009FE8
            uint16_t fcmd    = FCM_RWOTP_BIT | FSH_CMD_RD_OTP;
            uint8_t i = 0;
            fshc_read(0x11E8, fsh_buff, 6, fcmd);
            DEBUG("OTP Val:");
            debugHex(fsh_buff, 6);
            DEBUG("EM Trim Bf:");
            for (i = 0; i < 6; ++i)
            {
                debug("%08X ", RD_32(EM_TRIM_ADDR+(i<<2)));
            }
            debug("\r\n");
            for (i = 0; i < 6; ++i)
            {
                WR_32(EM_TRIM_ADDR + (i<<2), fsh_buff[i]);
            }
            DEBUG("EM Trim Af:");
            for (i = 0; i < 6; ++i)
            {
                debug("%08X ", RD_32(EM_TRIM_ADDR+(i<<2)));
            }
            debug("\r\n");
        } break;
        
        case CMD_FSH_CLK:
        {
            DEBUG("FSH_CLK Bf:%d", rcc_fshclk_mhz());
            uint8_t fsh_clk_sel = *(buff + 1);
            rcc_fshclk_set(fsh_clk_sel);
            DEBUG("FSH_CLK Af:%d", rcc_fshclk_mhz());
        } break;

        default:
        {
        } break;
    }
    
    if (finish)
    {
        buff_len = 0;
    }
}

