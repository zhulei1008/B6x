#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_
#include "app.h"
/*
 * DEFINES
 ****************************************************************************************
 */

/// Macro for Number of keyboard row&column
#define KEY_ROW_NB            (8)
#define KEY_COL_NB            (16)

/// Macro for ghost-key detection (0-disable, 1-enable)
#define KEY_GHOST             (1)
/// Macro for key debounce (0-disable, else (1 << n) - 1)
#define KEY_DEBOUNCE          (0x00)
/// Reports Description @see hid_report_map
#define REP_IDX_KB             0
#define REP_LEN_KB             8  // 8-bytes
#define REP_IDX_MULTI          2
#define REP_LEN_MULTI          3  // 24-bits
#define REP_IDX_SLEEP          3
#define REP_LEN_SLEEP          1  // 8-bits

#define INT_MODE 			   1
#define UART_MODE 			   2
#define MAIN_MODE 			   3
#define FUN_MODE 			   MAIN_MODE
enum media_key_bits
{
    // byte0
    MK0_BIT0_LIGHT_DN        = 0x01,
    MK0_BIT1_LIGHT_UP        = 0x02,
    MK0_BIT2_WWW_SEARCH      = 0x04,
    MK0_BIT3_CUT             = 0x08,
    MK0_BIT4_COPY            = 0x10,
    MK0_BIT5_PASTE           = 0x20,
    MK0_BIT6_PREV_TRK        = 0x40,
    MK0_BIT7_START_PAUSE     = 0x80,
    // byte1
    MK1_BIT0_NEXY_TRK        = 0x01,
    MK1_BIT1_MUTE            = 0x02,
    MK1_BIT2_VOL_DN          = 0x04,
    MK1_BIT3_VOL_UP          = 0x08,
    MK1_BIT4_WWW_BACK        = 0x10,
    MK1_BIT5_EMAIL           = 0x20,
    MK1_BIT6_VIRKB           = 0x40,
    MK1_BIT7_HOME            = 0x80,
    // byte2
    MK2_BIT0_BROWSER         = 0x01,
    MK2_BIT1_CALCAULATOL     = 0x02,
    MK2_BIT2_COMPUTER        = 0x04,
    MK2_BIT3_STOP            = 0x08,
    MK2_BIT4_FAVORITES       = 0x10,
    MK2_BIT2_AND_VIRKB       = 0x20,
    MK2_BIT6_MUSIC           = 0x40,
    MK2_BIT7_POWER           = 0x80, 
};

enum sleep_key_bits
{
    // byte0
    //SK0_BIT0_ONOFF = 0x01,
    SK0_BIT0_SLEEP  = 0x02,
    SK0_BIT1_WAKEUP = 0x04,
};

enum key_state
{
    KS_RELEASE,
    KS_PRESS,
};

enum key_type
{
    KT_NONE,
    KT_LOCAL,
    
    KT_GHOST,
    KT_KEYBOARD,
    KT_MEDIA,
    KT_POWER,
};
#define KeyRowMask    ((uint32_t)0x00751090)
#define KeyColumnMask ((uint32_t)0x7e8aed04)
#define KeyMapMask    ((uint32_t)0x817512fb)


struct key_env_tag
{
    uint8_t sys;
    uint8_t state;
    
    bool fnFlag;
    bool capsFlag;

    // count of repeat
    uint8_t repCnt;
    // count of kb key
    uint8_t keyCnt;
    // record multi-key
    uint8_t mkCode;
    
    // current report
    uint8_t keyTyp;
    uint8_t report[8];
    
    // backup report
    uint8_t keyTyp0;
    uint8_t report0[8];
    
    #if (KEY_GHOST)
    uint8_t rowCnt[KEY_ROW_NB];

    #endif
    
    #if (KEY_GHOST || KEY_DEBOUNCE)
    uint8_t colSta[KEY_COL_NB];

    #endif
    
    #if (KEY_DEBOUNCE)
    uint8_t shake[KEY_ROW_NB][KEY_COL_NB];
    #endif
};

void kb_init(void);
void kb_scan(void);
bool keyboard_proc(void);
void keyboard_data_proc(void);
void Extend_Power_Pin_Init(void);
void PowerOff_8bit_MCU(void);
void PowerOn_8bit_MCU(void);
void Keyboard_Scan_Init(void);
void kb_sleep(void);
#endif  //_KEYBOARD_
