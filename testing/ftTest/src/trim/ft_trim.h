//@ IC divice: trim

#ifndef _FT_TRIM_H_
#define _FT_TRIM_H_

#include <stdint.h>


#define TRIM_VAL_NUM            6 // VAL00~VAL05



// TRIM 0A: Flash XorKey
typedef union
{
    struct
    {
        uint32_t XOSC16M_CAP_TRIM_VAILD:  4;  // bit[3:0]
        uint32_t XOSC16M_CAP_TRIM:        6;  // bit[9:4]
        uint32_t Reserved:               22;  // bit[31:10]  
    };
    uint32_t Word;
} TRIM_VAL00_Typedef;

typedef union
{
    struct
    {
        uint32_t RANDOM_SEED;         // bit[31:0]
    };
    uint32_t Word;
} TRIM_VAL01_Typedef;


typedef union
{
    struct
    {     
        uint32_t LDO_TX_TRIM:       3; // bit[2:0]  
        uint32_t LDO_RX_TRIM:       3; // bit[5:3]       
        uint32_t BPF_CAL_CODE_EXT:  6; // bit[11:6]
        uint32_t BG_RES_TRIM:       5; // bit[16:12]    
        uint32_t BG_BIAS_TRIM:      2; // bit[18:17]  
        uint32_t MIC_TRIM:          7; // bit[25:19]        
        uint32_t Reserved:          6; // bit[31:26] add 6vp 1118
    };
    uint32_t Word;
} TRIM_VAL02_Typedef;

typedef union
{
    struct
    {
        uint32_t RC16M_FREQ_TRIM_VAILD:   4; // bit[3:0]
        uint32_t RC16M_FREQ_TRIM:         6; // bit[9:4]
        uint32_t RC16M_FREQ_VALUE:       12; // bit[21:10]  // rc16m freq value
                                             // [21:17] integer(8~24MHz), [16:10] decimal((0~99)*10KHz)
        uint32_t Reserved:                2; // bit[23:22]
        uint32_t LDO_XOSC_TR_VAILD:       4; // bit[27:24]
        uint32_t LDO_XOSC_TR:             4; // bit[31:28]        
    };
    uint32_t Word;
} TRIM_VAL03_Typedef;


typedef union
{
    struct
    {        
        uint32_t VOLTAGE_TRIM_VALID:    4;   // bit[3:0]
		uint32_t CORELDO_TRIM_RUN:      5;   // bit[8:4]
		uint32_t CORELDO_TRIM_DP: 		5;   // bit[13:9]
		uint32_t AONLDO_TRIM_RUN:       4;   // bit[17:14]
		uint32_t AONLDO_TRIM_OFF:       4;   // bit[21:18]		
        uint32_t Reserved:              4;   // bit[25:22]
        uint32_t BK_BOD_TRIM:           3;   // bit[28:26]
        uint32_t LDO_LVD_SEL:           3;   // bit[31:29]                
    };
    uint32_t Word;
} TRIM_VAL04_Typedef;


typedef union
{
    struct
    {
        uint32_t TRIM_VALID_FLAG;         // bit[31:0]
    };
    uint32_t Word;
} TRIM_VAL05_Typedef;

typedef struct
{
    TRIM_VAL00_Typedef  VAL00;
    TRIM_VAL01_Typedef  VAL01;
    TRIM_VAL02_Typedef  VAL02;
    TRIM_VAL03_Typedef  VAL03;
    TRIM_VAL04_Typedef  VAL04;
    TRIM_VAL05_Typedef  VAL05;

} TRIM_Typedef;

void ft_coreldo_init(void);

void ft_coreldo_trim_adj(uint8_t cmd);



void ft_aonldo_init(void);

void ft_aonldo_trim_adj(uint8_t cmd);

void ft_trim_download_test(void);

#endif // _TRIM_H_

