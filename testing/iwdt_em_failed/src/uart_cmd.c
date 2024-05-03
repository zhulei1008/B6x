#include "afe_common.h"
#include "uart_cmd.h"
#include "uart.h"
#include "rf_dbg.h"
#include "analog_pad.h"

#if !defined(UART_CMD_PORT)
#define UART_CMD_PORT 0 // UART1
#endif

#if !defined(UART_IO_TX)
#define UART_IO_TX 14 // PA14
#endif
#if !defined(UART_IO_RX)
#define UART_IO_RX 15 // PA15
#endif

#define UART_RSP_OK        0x00 //rsp ok
#define UART_RSP_CRC       0xA0 //CRC fail
#define UART_RSP_ERR       0xA3 //head error,include id error and length error

#define CRC_XOR_SEED     0xFF  // XOR(crc) Seed-Value, 0xFF inverse ECHO - 20200707 6vip

#define U32_VAL(addr)    (*(uint32_t *)(addr)) // addr must align 4
#define U16_VAL(addr)    (*(uint16_t *)(addr)) // addr must align 2

//@struct frame_t
typedef struct
{
    /* head(4B): 1-sync; 1-cmd; 2-len */
    uint8_t   sync;
    uint8_t   cmd;
    uint16_t  len;
    
    /* body(nB): A set of variable value */
    uint8_t   body[BODY_LEN_CWR_MAX];
} frame_t;


/// **** uart init ****
static void uartCmdPortInit(void)
{
    uart_init(UART_CMD_PORT, UART_IO_TX, UART_IO_RX);

    uart_conf(UART_CMD_PORT, BRR_115200, LCR_BITS_DFLT); 
}

#define WAIT_UART_IDLE() do{while (!(UARTCTRL[UART_CMD_PORT]->SR.TBEM)); while(UARTCTRL[UART_CMD_PORT]->SR.BUSY);}while(0)

/// **** uart response ****
static void uartRspState(uint8_t rspId, uint8_t state)
{
    /* fixed format: 1-sync 1-rspId 2-flen(01 00) 1-state */
    uart_putc(UART_CMD_PORT, UART_RSP_SYNC);
    uart_putc(UART_CMD_PORT, rspId);

    // frame.len=0x0001, LSB
    uart_putc(UART_CMD_PORT, 0x01); 
    uart_putc(UART_CMD_PORT, 0x00);

    uart_putc(UART_CMD_PORT, state);
}

/// define macro func to simple used
#define uartRspOK(rspId)     uartRspState(rspId, UART_RSP_OK)
#define uartRspErr(rspId)    uartRspState(rspId, UART_RSP_ERR)
#define uartRspErrCrc(rspId) uartRspState(rspId, UART_RSP_CRC)

static void uartRspData(uint8_t rspId, uint16_t len, const uint8_t *dat)
{
    /* del format(no crc): 1-sync 1-rspId 2-flen(len+1) len-data 1-ack */
    /* del format(is crc): 1-sync 1-rspId 2-flen(len+2) len-data 1-crc 1-ack */
    /* add format( 1 crc): 1-sync 1-rspId 2-flen(len+1) len-data 1-crc */
    uint16_t i, flen;
    uint8_t crc = CRC_XOR_SEED;

    uart_putc(UART_CMD_PORT, UART_RSP_SYNC);
    uart_putc(UART_CMD_PORT, rspId);

    //flen = IS_CRC_RSP(rspId) ? len + 2 : len + 1;
    flen = len + 1; // add 1 Byte: crc, 20200707 duao

    uart_putc(UART_CMD_PORT, (uint8_t)(flen & 0xFF)); // frame.len, LSB
    uart_putc(UART_CMD_PORT, (uint8_t)(flen >> 8));

    for (i = 0; i < len; i++ )
    {
        uart_putc(UART_CMD_PORT, dat[i]);
        crc ^= dat[i];
    }
    uart_putc(UART_CMD_PORT, crc);
}

/// **** wait frames ****
static uint8_t crcXor(uint8_t *buff, uint16_t len)
{
    uint16_t i;
    uint8_t crc = CRC_XOR_SEED;
    
    for (i = 0; i < len; i++)
    {
        crc ^= buff[i];
    }
    return crc;
} 

static bool waitFixedFrame(frame_t *pfm, uint16_t fixlen)
{
    uint16_t i, flen = pfm->len;
    
    // judge valid length 
    if (flen != fixlen)
    {
        uartRspErr(pfm->cmd | 0x80); // pkt->recvlen = 0;
        return false;
    }
    
    if (flen == 0)
        return true; // none-body cmd: RST|RETURN
    
    // wait to fill body
    for (i = 0; i < flen; i++)
    {
        pfm->body[i] = uart_getc(UART_CMD_PORT);
    }
    
    // chk crc when body >=4
    if ((flen >= 4) && (crcXor(pfm->body, flen) != 0)) // not CRC_XOR_SEED, fix bug 6vip 20200707
    {
        uartRspErrCrc(pfm->cmd | 0x80);
        return false;
    }

    return true;
}

static bool waitVariableFrame(frame_t *pfm, uint16_t minlen, uint16_t maxlen)
{
    uint16_t i, flen = pfm->len;
    
    // judge length range
    if ((flen < minlen) || (flen > maxlen))
    {
        uartRspErr(pfm->cmd | 0x80);
        return false;
    }
    
    // wait to fill body
    for (i = 0; i < flen; i++)
    {
        pfm->body[i] = uart_getc(UART_CMD_PORT);
    }
    
    // chk crc
    if (crcXor(pfm->body, flen) != 0) // not CRC_XOR_SEED, fix bug 6vip 20200707
    {
        uartRspErrCrc(pfm->cmd | 0x80);
        return false;
    }

    return true;
}

/// **** command parse ****
void userUartParse(void)
{
    uint32_t address;
    uint16_t datalen;
    uint16_t recvlen;
    uint8_t buffer[MAX_FRAME_LEN];

    frame_t *frame = (frame_t*)buffer;
    
    // init port and buffer
    uartCmdPortInit();
    recvlen = 0;

    while (1)
    {
        // 1. wait frame's head: 4B[1-sync 1-cmd 2-len]
        buffer[recvlen++] = uart_getc(UART_CMD_PORT);
        
        if (buffer[0] != UART_CMD_SYNC) // check sync word
        {
            recvlen = 0;
            continue;
        }

        if (recvlen < CMD_HEAD_LEN) // check header's len
            continue;

        // 2. wait cmd's frame finish, then process
        switch (frame->cmd)
        {
            case UART_CMD_TX_GAIN_CAL:
                if (waitFixedFrame(frame, BODY_LEN_TX_GAIN_CAL))
                {
                    uint8_t pll_dis    = *(frame->body + 0);
                    uint8_t pll_frach  = *(frame->body + 1);

                    uint8_t gain_res = tx_gain_cal(pll_dis, pll_frach);

                    uartRspData(UART_RSP_TX_GAIN_CAL, 1, &gain_res);
                }
                break;
                
            case UART_CMD_TX_PA:
                if (waitFixedFrame(frame, BODY_LEN_TX_PA))
                {
                    uint8_t ant_cap_tx  = *(frame->body + 0);
                    uint8_t pa_cap      = *(frame->body + 1);
                    uint8_t pa_gain     = *(frame->body + 2);
                    
                    tx_pa(ant_cap_tx, pa_cap, pa_gain);
                    
                    uartRspData(UART_RSP_TX_PA, BODY_LEN_TX_PA, frame->body);
                }
                break;
                
            case UART_CMD_DPLL_LOCK:
                if (waitFixedFrame(frame, BODY_LEN_DPLL_LOCK))
                {
                    uint8_t sel  = *(frame->body + 0);
                    uint8_t lock = dpll256m_384m_lock(sel);
                    
                    uartRspData(UART_RSP_DPLL_LOCK, BODY_LEN_DPLL_LOCK, &lock);
                }
                break;
                
            case UART_CMD_VDD_PA:
                if (waitFixedFrame(frame, BODY_LEN_VDD_PA))
                {
                    uint8_t pa_gain = *(frame->body + 0);
                    
                    ldo_vdd_pa(pa_gain);
                    uartRspData(UART_RSP_VDD_PA, BODY_LEN_VDD_PA, frame->body);
                }
                break;
                
            case UART_CMD_VTXD_EXT:
                if (waitFixedFrame(frame, BODY_LEN_VTXD_EXT))
                {
                    uint8_t vtxd_ext = *(frame->body + 0);
                    
                    pll_vtxd_ext_test_cmd(vtxd_ext);
                    uartRspData(UART_RSP_VTXD_EXT, BODY_LEN_VTXD_EXT, frame->body);
                }
                break;
                
            case UART_CMD_DCDC_PWM:
                if (waitFixedFrame(frame, 2))
                {
//                    SYSCFG->DCDC.PWM_FI = *(frame->body + 0);
//                    SYSCFG->DCDC.PWM_FC = *(frame->body + 1);

                    uartRspData(UART_RSP_DCDC_PWM, 2, frame->body);
                }
                break;
                
            case UART_CMD_RC16M:
                if (waitFixedFrame(frame, BODY_LEN_RC16M))
                {
                    uint8_t freq_trim = *(frame->body + 0);
                    rc16m_test_cmd(freq_trim);
                    
//                    uint8_t freq_cco = *(frame->body + 0);
//                    uint8_t freq_cl = *(frame->body + 1);
//                    uint8_t tc = *(frame->body + 2);
//                    uint8_t fvch = *(frame->body + 3);
                    
//                    rc16m_clk_out(freq_cco, freq_cl, tc, fvch);
                    uartRspData(UART_RSP_RC16M, (BODY_LEN_RC16M - 1), frame->body);
                }
                break;
                
            case UART_CMD_RX_LNA:
                if (waitFixedFrame(frame, BODY_LEN_RX_LNA))
                {
                    struct rx_lna *param = (struct rx_lna*)(frame->body);
                    
                    rx_lna_test_cmd(param);
                    
                    uartRspData(UART_RSP_RX_LNA, (BODY_LEN_RX_LNA-1), frame->body);
                }
                break;

            case UART_CMD_CBPF_CAL:
                if (waitFixedFrame(frame, BODY_LEN_CBPF_CAL))
                {
                    uint8_t cal_result = cbpf_cal_test_cmd(*(frame->body + 0));
                    uartRspData(UART_RSP_CBPF_CAL, BODY_LEN_CBPF_CAL, &cal_result);
                }
                break;
            
            case UART_CMD_XOSC16M:
                if (waitFixedFrame(frame, BODY_LEN_XOSC16M))
                {
                    uint8_t xo16m_lp  = *(frame->body + 0);
                    uint8_t xo16m_adj = *(frame->body + 1);
                    uint8_t cap_trim  = *(frame->body + 2);
                    uint8_t ldo_trim  = *(frame->body + 3);
                    
                    if (CLK_OUT_HSE != RCC->CFG.MCO_SW)
                    {
                        sys_clk_out(CLK_OUT_HSE);
                    }
                    
                    xo16m_test_cmd(xo16m_lp, xo16m_adj, cap_trim, ldo_trim);
                    uartRspData(UART_RSP_XOSC16M, BODY_LEN_XOSC16M - 1, frame->body);
                }
                break;

            case UART_CMD_XOSC32K:
                if (waitFixedFrame(frame, BODY_LEN_XOSC32K))
                {
//                    xosc32k_test_cmd(*(frame->body + 0));
                    uartRspData(UART_RSP_XOSC32K, BODY_LEN_XOSC32K, frame->body);
                }
                break;

            case UART_CMD_RC32K_CAL:
                if (waitFixedFrame(frame, BODY_LEN_RC32K_CAL))
                {
                    uint16_t cal_code = 0;
//                    cal_code = rc32k_cal_test_cmd();
                    uartRspData(UART_RSP_RC32K_CAL, 2, (uint8_t *)&cal_code);
                }
                break;
 
            case UART_CMD_RC32K:
                if (waitFixedFrame(frame, BODY_LEN_RC32K))
                {
//                    uint8_t ctl = *(frame->body + 0);
//                    uint16_t spi_code = 0;
//                    memcpy(&spi_code, (frame->body + 1), 2);

//                    rc32k_test_cmd(ctl, spi_code);
                    
                    uartRspData(UART_RSP_RC32K, BODY_LEN_RC32K, frame->body);
                }
                break;

            case UART_CMD_LDO_DIG:
                if (waitFixedFrame(frame, BODY_LEN_LDO_DIG))
                {
                    // 0 ~ 31
                    uint8_t bg_bias_trim  = *(frame->body + 0); 
                                        
                    // 0 ~ 7
                    uint8_t bg_vref_fine = *(frame->body + 1);
                    
                    // 0 ~ 3
                    uint8_t bg_res_trim = *(frame->body + 2);

                    ldo_dig_test_cmd(bg_bias_trim, bg_vref_fine, bg_res_trim);
                    
                    uartRspData(UART_RSP_LDO_DIG, BODY_LEN_LDO_DIG, frame->body);
                }
                break;

            case UART_CMD_LDO:
                if (waitFixedFrame(frame, BODY_LEN_LDO))
                {
                    // @see enum ldo_vdd_case, 0 ~ 4
                    uint8_t test_case  = *(frame->body + 0); 
                    
                    // 0 ~ 7
                    uint8_t trim       = *(frame->body + 1);
                    
                    ldo_vdd_test_cmd(test_case, trim);
                    
                    uartRspData(UART_RSP_LDO, BODY_LEN_LDO, frame->body);
                }
                break;

            case UART_CMD_PA_RAMP:
                if (waitFixedFrame(frame, 1))
                {
                    uint8_t ramp_up_down = *(frame->body + 0);

                    if (ramp_up_down) // Ramp-Up
                    {
                        pa_ramp_up();
                    }
                    else //  Ramp-Down
                    {
                        pa_ramp_down();
                    }
                    
                    uartRspData(UART_RSP_PA_RAMP, 1, frame->body);
                }
                break;

            case UART_CMD_PLL_1M_LOCK:
                if (waitFixedFrame(frame, BODY_LEN_PLL_1M_LOCK))
                {
                    uint8_t tx_rx_mode = *(frame->body + 0);
                    uint8_t pll_dis    = *(frame->body + 1);
                    uint8_t pll_frach  = *(frame->body + 2);

                    if (tx_rx_mode) // Tx mode
                    {
                        pll_tx_1m_lock_test_cmd(pll_dis, pll_frach);
                    }
                    else // Rx mode
                    {
                        pll_rx_1m_lock_test_cmd(pll_dis, pll_frach);
                    }
                    
                    uartRspData(UART_RSP_PLL_1M_LOCK, BODY_LEN_PLL_1M_LOCK, frame->body);
                }
                break;

            case UART_CMD_VCO_FREQ:
                if (waitFixedFrame(frame, BODY_LEN_VCO))
                {
                    vco_freq_test_cmd(*(frame->body + 0));
                    uartRspData(UART_RSP_VCO_FREQ, BODY_LEN_VCO, frame->body);
                }
                break;

            case UART_CMD_PLL_LOCK:
                if (waitFixedFrame(frame, BODY_LEN_PLL_LOCK))
                {
//                    pll_lock_test_cmd(*(frame->body + 0));
//                    uartRspData(UART_RSP_PLL_LOCK, BODY_LEN_PLL_LOCK, frame->body);
                    uint8_t ret = get_pll_freq_adj_st(*(frame->body + 0));
                    uartRspData(UART_RSP_PLL_LOCK, BODY_LEN_PLL_LOCK, &ret);
                }
                break;

            case UART_CMD_SRD: // single read 4B
                if (waitFixedFrame(frame, BODY_LEN_SRD))
                {
                    address = U32_VAL(frame->body+OFFSET_BODY_ADDR);
                    if (address & 0x03) // align: byte or word
                    {
                        uartRspData(UART_RSP_SRD, 4, (uint8_t *)(address));
                    }
                    else
                    {
                        uint32_t value = U32_VAL(address);
                        uartRspData(UART_RSP_SRD, 4, (uint8_t *)&value);
                    }
                }
                break;

            case UART_CMD_SWR: // single write 4B
                if (waitFixedFrame(frame, BODY_LEN_SWR))
                {
                    address = U32_VAL(frame->body + OFFSET_BODY_ADDR);
                    if (address & 0x03) // align: byte or word 
                    {
                        memcpy((void *)address, frame->body+OFFSET_BODY_DATA, 4);
                    }
                    else
                    {
                        U32_VAL(address) = U32_VAL(frame->body+OFFSET_BODY_DATA);
                    }
                    
                    uartRspOK(UART_RSP_SWR);
                }
                break;

            case UART_CMD_CRD: // continue read
                if (waitFixedFrame(frame, BODY_LEN_CRD))
                {
                    address = U32_VAL(frame->body + OFFSET_BODY_ADDR);
                    datalen = U16_VAL(frame->body + OFFSET_BODY_LEN);
                    
                    uartRspData(UART_RSP_CRD, datalen, (uint8_t *)(address));
                }
                break;

            case UART_CMD_WR_HD: // notice write(4addr+2len)
                if (waitFixedFrame(frame, BODY_LEN_WR_HD))
                {
                    uartRspOK(UART_RSP_WR_HD);
                    // ? need do something
                }
                break;

            case UART_CMD_CWR: //continue write nB
                if (waitVariableFrame(frame, BODY_LEN_CWR_MIN, BODY_LEN_CWR_MAX))
                {
                    uint16_t i;
                    
                    address = U32_VAL(frame->body + OFFSET_BODY_ADDR);
                    datalen = frame->len - 5; // 4B addr + 1B crc

                    if ((datalen & 0x3) || (address & 0x03)) // align: byte or word 
                    {
                        memcpy((void *)address, frame->body+OFFSET_BODY_DATA, datalen);
                    }
                    else
                    {
                        for (i = 0; i < datalen; i += 4)
                        {
                            U32_VAL(address+i) = U32_VAL(frame->body+OFFSET_BODY_DATA+i);
                        }
                    }   

                    uartRspOK(UART_RSP_CWR);
                }
                break;

            case UART_CMD_ECHO:
                if (waitFixedFrame(frame, BODY_LEN_ECHO))
                {
                    uartRspData(UART_RSP_ECHO, 1, frame->body + OFFSET_BODY_ECHO); // 1B echo
                }
                break;

            case UART_CMD_RST:
                if (waitFixedFrame(frame, BODY_LEN_RST))
                {
                    uartRspOK(UART_RSP_RST);
                    WAIT_UART_IDLE();
                    NVIC_SystemReset(); // reset
                }
                break;

            case UART_CMD_JUMP:
                if (waitFixedFrame(frame, BODY_LEN_JUMP))
                {
                    address = U32_VAL(frame->body + OFFSET_BODY_ADDR);
                    if (/*sramValid*/(U32_VAL(address))) // judge to jump - 6vip 0611
                    {
                        uartRspOK(UART_RSP_JUMP);
                        WAIT_UART_IDLE();
//                        sysJumpTo(address); // jump, never return
                    }
                    else
                    {
                        uartRspErr(UART_RSP_JUMP);
                    }
                }
                break;

            default:
                uartRspErr(frame->cmd);
                break;
        }
        
        // 3. clean package
        recvlen = 0;
    }
}
