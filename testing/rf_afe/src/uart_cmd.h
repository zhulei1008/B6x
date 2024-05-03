//@ IC uart_cmd: userUartParse

#ifndef AFE_UART_CMD_H_
#define AFE_UART_CMD_H_


//@ Uart Commands
//  1. Single  Read  Cmd: 9B
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||-- 4 BYTES  --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_SRD    --||-- 0x0004  --||-- ADDRESS  --||-- CRC(xor) --||

//  2. Single  Write  Cmd: 13B
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||-- 4 BYTES  --||-- 4 BYTES --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_SWR    --||-- 0x0008  --||-- ADDRESS  --||-- DATA    --||-- CRC(xor) --||

//  3. Continue  Read  Cmd: 11B
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||-- 4 BYTES  --||-- 2 BYTES --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_CRD    --||-- 0x0007  --||-- ADDRESS  --||-- DATALEN --||-- CRC(xor) --||

//  4. Continue  Write  Header Cmd: 11B
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||-- 4 BYTES  --||-- 2 BYTES --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_WR_HD  --||-- 0x0006  --||-- ADDRESS  --||-- DATALEN --||-- CRC(xor) --||

//  5. Continue  Write  Data Cmd: 9+nB
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||-- 4 BYTES  --||-- n BYTES --||-- 1 BYTE    --||
//    ||-- CMD_SYNC --||-- CMD_CWR    --||-- 0x(n+5) --||-- ADDRESS  --||--  DATA   --||-- CRC(xor) --||

//  6. Modify baud rate  Cmd: 9B
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||-- 4 BYTES  --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_BAUD   --||-- 0x0004  --||-- BAUDRATE --||-- CRC(xor) --||

//  7. Echo  Cmd: 5B
//    ||-- 1 BYTE   --||--  1 BYTE    --||-- 2 BYTES --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_ECHO   --||-- 0x0001  --||--  DATA    --||

//  8. Reset Cmd: 4B
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES --||
//    ||-- CMD_SYNC --||-- CMD_RST    --||-- 0x0000  --||

//  9. Jump Cmd: 9B
//    ||-- 1 BYTE   --||--  1 BYTE    --||-- 2 BYTES --||-- 4 BYTES  --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_JUMP   --||-- 0x0004  --||-- ADDRESS  --||-- CRC(xor) --||

//  10. Patch Cmd: 9B
//    ||-- 1 BYTE   --||--  1 BYTE    --||-- 2 BYTES --||-- 4 BYTES  --||-- 1 BYTE   --||
//    ||-- CMD_SYNC --||-- CMD_PATCH  --||-- 0x0004  --||-- ADDRESS  --||-- CRC(xor) --||

//  11. Return ROM Cmd: 4B
//    ||-- 1 BYTE   --||--   1 BYTE   --||-- 2 BYTES --||
//    ||-- CMD_SYNC --||-- CMD_RETURN --||-- 0x0000  --||
// -------------------------------------------------------------------------------------

#define UART_CMD_SYNC    0x5A

#define UART_CMD_SRD     0x01 //single read
#define UART_CMD_SWR     0x02 //single write
#define UART_CMD_CRD     0x04 //continue read
#define UART_CMD_WR_HD   0x07 //continue write header
#define UART_CMD_CWR     0x08 //continue write data
#define UART_CMD_BAUD    0x0B //modify baud rate
#define UART_CMD_ECHO    0x0D //test uart baud
#define UART_CMD_RST     0x10 //reset
#define UART_CMD_JUMP    0x13 //jump to sram, and execute driver
#define UART_CMD_PATCH   0x15 //patch to sram, and execute driver
#define UART_CMD_RETURN  0x19 //return ROM

//@ Uart Responses
//  1. Single  Read  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE  --||-- 2 BYTES  --||-- 4 BYTES --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_SRD --||-- 0x0005   --||-- DATA    --||-- ACK/NCK --||

//  2. Single  Write  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE  --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_SWR --||-- 0x0001   --||-- ACK/NCK --||

//  3. Continue  Read  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE  --||-- 2 BYTES --||-- n BYTES --||--  1 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_CRD --||-- 0x(n+2) --||--   DATA  --||-- CRC(DATA) --||-- ACK/NCK --||

//  4. Continue  Write  Header  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE    --||-- 2 BYTES --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_WR_HD --||-- 0x0001  --||-- ACK/NCK --||

//  5. Continue  Write  Data  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE  --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_CWR --||-- 0x0001   --||-- ACK/NCK --||

//  6.1. Receive  Signal  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE     --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_BAUDRV --||-- 0x0001   --||-- ACK/NCK --||

//  6.2  Modify Baud Rate End  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE      --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_BAUDEND --||-- 0x0001   --||-- ACK/NCK --||

//  7. Echo Rsp
//    ||-- 1 BYTE   --||--  1 BYTE  --||-- 2 BYTES  --||-- 1 BYTE --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_ECHO --||-- 0x0002   --||--  DATA  --||-- ACK/NCK --||

//  8.Reset  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE  --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_RST --||-- 0x0001   --||-- ACK/NCK --||

//  9.Jump  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE   --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_JUMP --||-- 0x0001   --||-- ACK/NCK --||

//  10.Patch  Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE   --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_PATCH --||-- 0x0001   --||-- ACK/NCK --||

//  11. Return ROM Rsp
//    ||-- 1 BYTE   --||--  1 BYTE    --||-- 2 BYTES  --||-- 1 BYTE  --||
//    ||-- RSP_SYNC --||-- RSP_RETURN --||-- 0x0001   --||-- ACK/NCK --||

//  12. Error Rsp
//    ||-- 1 BYTE   --||-- 1 BYTE      --||-- 2 BYTES  --||--  1 BYTE --||
//    ||-- RSP_SYNC --||-- UART_RSP_ERR --||-- 0x0001  --||-- RSP_ERR --||
// -------------------------------------------------------------------------------------

#define UART_RSP_SYNC      0xA5

#define UART_RSP_SRD       0x81
#define UART_RSP_SWR       0x82
#define UART_RSP_CRD       0x84
#define UART_RSP_WR_HD     0x87
#define UART_RSP_CWR       0x88
#define UART_RSP_BAUDRV    0x8B //receive signal to modify baud
#define UART_RSP_BAUDEND   0x8C //modify baud rate end
#define UART_RSP_ECHO      0x8D
#define UART_RSP_RST       0x90
#define UART_RSP_JUMP      0x93
#define UART_RSP_PATCH     0x95
#define UART_RSP_RETURN    0x99

#define CMD_HEAD_LEN     4

#define BODY_LEN_SRD     0x05  // single read = (4addr + 1crc)
#define BODY_LEN_SWR     0x09  // single write = (4addr + 4data + 1crc)
#define BODY_LEN_CRD     0x07  // continue read = (4addr + 2bodylen + 1crc)
#define BODY_LEN_WR_HD   0x07  // continue write header = (4addr + 2bodylen + 1crc)
#define BODY_LEN_CWR_MIN 0x06  // continue write data min = (4addr + 1data + 1crc)
#define BODY_LEN_CWR_MAX 0x105 // continue write data max = (4addr + 256data + 1crc)
#define BODY_LEN_BAUD    0x05  // modify baud rate = (4baud + 1crc)
#define BODY_LEN_ECHO    0x01  // *test echo = (1data)
#define BODY_LEN_RST     0x00  // *reset
#define BODY_LEN_JUMP    0x05  // jump to sram, and execute driver = (4addr + 1crc)
#define BODY_LEN_PATCH   0x05  // patch to sram, and execute driver = (4addr + 1crc)
#define BODY_LEN_RETURN  0x00  // *return ROM

#define MAX_FRAME_LEN    (CMD_HEAD_LEN + BODY_LEN_CWR_MAX)

#define OFFSET_BODY_ADDR 0 // SRD SWR CRD WR_HD CWR JUMP
#define OFFSET_BODY_BAUD 0 // BAUD
#define OFFSET_BODY_ECHO 0 // ECHO
#define OFFSET_BODY_LEN  4 // CRD WR_HD
#define OFFSET_BODY_DATA 4 // SWR CWR 

/*
**********************************************
* user define
**********************************************
**/
#define CMD_TO_RSP(cmd)       ((0x80) + (cmd))
enum cmd
{
    UART_CMD_VCO_FREQ  = 0x30,
    UART_CMD_PLL_LOCK,
    UART_CMD_PLL_1M_LOCK,
    UART_CMD_TX_PA,
    UART_CMD_PA_RAMP,

    UART_CMD_LDO      = 0x35,
    UART_CMD_LDO_DIG,
    UART_CMD_RC32K,
    UART_CMD_RC32K_CAL,
    UART_CMD_XOSC32K,

    UART_CMD_XOSC16M  = 0x3A,
    UART_CMD_CBPF_CAL,
    UART_CMD_RX_LNA,
    UART_CMD_RC16M,
    UART_CMD_DCDC_PWM,

    UART_CMD_VTXD_EXT = 0x3F,

    UART_CMD_TX_GAIN_CAL = 0x40,
    UART_CMD_VDD_PA,
    UART_CMD_DPLL_LOCK,
    UART_CMD_RSSI,
    UART_CMD_TEMP,
    
    UART_CMD_RANDOM_NUM = 0x46,
};

enum rsp
{
    UART_RSP_VCO_FREQ = 0xB0,
    UART_RSP_PLL_LOCK,
    UART_RSP_PLL_1M_LOCK,
    UART_RSP_TX_PA,
    UART_RSP_PA_RAMP,

    UART_RSP_LDO      = 0xB5,
    UART_RSP_LDO_DIG,
    UART_RSP_RC32K,
    UART_RSP_RC32K_CAL,
    UART_RSP_XOSC32K,

    UART_RSP_XOSC16M  = 0xBA,
    UART_RSP_CBPF_CAL,
    UART_RSP_RX_LNA,
    UART_RSP_RC16M,
    UART_RSP_DCDC_PWM,

    UART_RSP_VTXD_EXT = 0xBF,

    UART_RSP_TX_GAIN_CAL = 0xC0,
    UART_RSP_VDD_PA,
    UART_RSP_DPLL_LOCK,
    UART_RSP_RSSI,
    UART_RSP_TEMP,
    
    UART_RSP_RANDOM_NUM = 0xC6,
};

/*********************************************/

/*********************************************/
#define BODY_LEN_VCO          0x01 // pll_freq_ext
#define BODY_LEN_PLL_LOCK     0x01 // pll_di_s
#define BODY_LEN_PLL_1M_LOCK  0x03 // tx/rx mode + pll_di_s + pll_frach
#define BODY_LEN_TX_PA        0x03 // ant_cap_tx + pa_cap + pa_gain
#define BODY_LEN_LDO          0x02 // test_case + trim
#define BODY_LEN_LDO_DIG      0x03 // bias_trim + vref_fine + res_trim
#define BODY_LEN_RC32K        0x03 // ctl + 2B spi_code
#define BODY_LEN_RC32K_CAL    0x00
#define BODY_LEN_XOSC32K      0x01 // enable/disable
#define BODY_LEN_XOSC16M      0x05 // xo16m_lp + xo16m_adj + xo16m_cap_trim + xo16m_ldo_trim + crc
#define BODY_LEN_CBPF_CAL     0x01 // bpf_cent_adj
#define BODY_LEN_RX_LNA       0x0B // ant_cap_rx + rf_rsv + lna_gain + mixl_bias + mixh_bias + bpf_iadj + bpf_gain_adj + mixh_en_cap + mixh_gain_ctrl + mixl_gain_ctrl + crc
#define BODY_LEN_RC16M        0x05 // cco + cl + tc + fvch + crc
#define BODY_LEN_VTXD_EXT     0x01 // pll_vtxd_ext
#define BODY_LEN_TX_GAIN_CAL  0x02 // pll_di_s + pll_frach
#define BODY_LEN_VDD_PA       0x01 // pa_gain
#define BODY_LEN_DPLL_LOCK    0x01 // 0:dpll, 1:dpll2
#define BODY_LEN_TEMP         0x00
#define BODY_LEN_RANDOM_NUM   0x00
/*********************************************/

void userUartParse(void);

#endif // AFE_UART_CMD_H_

