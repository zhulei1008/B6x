#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "display.h"
#include "oled.h"
#include "sftmr.h"
#include "batch.h"
#include "xtal_test.h"
#include "gpio_test.h"

static void showChanResult(void);
static void getChanDetail(uint8_t idx, char *str, char *ret);

static uint8_t gScreen;
static uint8_t gDetail;

static uint32_t dispBlinker(uint8_t id)
{
    if (gScreen == SCRN_INIT)
    {
        dispInfo();
    }

    if (BATCH_IS_IDLE())
    {
        // btIcon
        const uint8_t *btIcon = (OLED_GRAM[0][0] == 0x10) ? BMP_BTLOGO : BMP_BTLOGO_OFF;

        oledShowBMP(/*119*/0, 0, 8, 16, btIcon);
        oledRefresh(0, 1);
    }
    return 50;
}

void dispInit(void)
{
    if (oledInit())
    {
        char verStr[6];
        sprintf(verStr, "V%d.%02d", (VER_BATCH >> 8) & 0x0F, (VER_BATCH >> 0) & 0xFF);

        // Title Bar(line0~15) - Always ON
        oledShowBMP(  0+8, 0, 80, 16, BMP_HYTESETLOGO);
        oledShowStr(88+8,  4, verStr);
        oledDrawLine(15);

        // Welcome Info
        oledShowGBKs(16, 24, 0, 5);  // "量产测试平台"
        oledShowGBKs(24, 48, 6, 10); // "欢迎使用！"

        oledRefresh(0, 7);
        //oledClear(2, 7);
        
        gScreen = SCRN_INIT;
        sfTmrStart(100, dispBlinker);
    }
}

void dispInfo(void)
{
    char cntStr[22];

    gScreen = SCRN_INFO;
    oledClear(2, 7);
    
    sprintf(cntStr, "[%c] Crc:%X", ~gBchFirm.firmCRC ? 'Y' : 'X', gBchFirm.firmCRC);
    oledShowStr(0, 16, cntStr);

    sprintf(cntStr, "[%c] CHs:%d(0x%X)", (batch.chsSet & batch.gChans) == batch.gChans ? 'Y' : 'X', \
                __builtin_popcount(batch.gChans), batch.gChans);
    oledShowStr(0, 28, cntStr);

//    sprintf(cntStr, "[X] Cfg:%X", gBchFirm.macConfig);
//    oledShowStr(0, 40, cntStr);

//    sprintf(cntStr, "[X] Cnt:%d/%d", getCountNum(),
//            ~gBchFirm.macConfig ? (((gBchFirm.macConfig >> 8) & 0xFFFFFF) * __builtin_popcount(batch.gChans)) : 0);
//    oledShowStr(0, 52, cntStr);

    if (BATCH_IS_ONLINE())
    {
        oledShowBMP(109, 20, 18, 14, BMP_USB_LOGO);
    }

    oledRefresh(2, 7);
}

void dispBusy(void)
{
    //if (!BATCH_IS_BUSY()) return;
    if (gScreen == SCRN_INIT) return;
    
    if (gScreen != SCRN_BUSY)
    {
        gScreen = SCRN_BUSY;
        oledClear(2, 7);
        
        // Show Busy...
        if (BATCH_IS_ONLINE())
        {
            oledShowBMP(109, 20, 18, 14, BMP_USB_LOGO);
        }
        
        oledShowGBKs(0, 20, 11, 12); /*"正",11  "在",12*/
        switch (batch.opCode)
        {
            case OP_BURN_CHAN :
                oledShowGBKs(32, 20, 22, 23); /*"更",22  "新",23*/
                break;

            case OP_BURN_FIRM :
                oledShowGBKs(32, 20, 24, 25); /*"配",24  "置",25*/
                break;

            default :
                oledShowGBKs(32, 20, 15, 16); /*"执",15  "行",16*/
                break;
        }
        oledShowStr(64, 24, "...");
        
        // Show Result
        showChanResult();
        oledRefresh(2, 7);
    }
    else
    {
        oledClear(5, 7);
        
        // Update Result
        showChanResult();
        oledRefresh(5, 7);
    }
}

void dispDone(void)
{
    //if (BATCH_IS_BUSY()) return;
    if (gScreen == SCRN_INIT) return;
    
    gScreen = SCRN_DONE;
    oledClear(2, 7);
        
    // Show Done
    if (BATCH_IS_ONLINE())
    {
        oledShowBMP(109, 20, 18, 14, BMP_USB_LOGO);
    }
    
    switch (batch.opCode)
    {
        case OP_BURN_CHAN :
            oledShowGBKs(0, 20, 22, 23); /*"更",22  "新",23*/
            break;

        case OP_BURN_FIRM :
            oledShowGBKs(0, 20, 24, 25); /*"配",24  "置",25*/
            break;

        default :
            oledShowGBKs(0, 20, 15, 16); /*"执",15  "行",16*/
            break;
    }
    oledShowGBKs(32, 20, 18, 19); /*"完",18  "成",19*/
    
    char num_str[8];
    uint8_t set_num  = __builtin_popcount(batch.gChans);
    uint8_t ok_num = set_num - __builtin_popcount(batch.chsSet) - __builtin_popcount(batch.chsErr);
    
    sprintf(num_str, ":%d/%d", ok_num, set_num);
    oledShowStr(64, 24, num_str);
    
    // Show Result
    showChanResult();    
    oledRefresh(2, 7);
}

void dispDetail(void)
{
    //if (BATCH_IS_BUSY()) return;
    if (gScreen == SCRN_INIT) return; // || !batch.chsErr
    
    if (gScreen != SCRN_DETAIL)
    {
        gScreen = SCRN_DETAIL;
        gDetail = 0;
    }
    
    char chanStr[20];
    uint8_t y = 16;
    
    oledClear(2, 7);
    
    for (uint8_t i = gDetail; i < 8; i++)
    {
        if ((batch.gChans >> i) & 0x01)//&& ((bchns[i].state & PSTA_FAIL)) 
        {
            if (y == 64) // reback
            {
                gDetail = i;
                oledRefresh(2, 7);
                return;
            }

            getChanDetail(i, chanStr, NULL);
            oledShowStr(8, y, chanStr);
            y += 12;
        }
    }

    if (gDetail) gDetail = 0;

    oledRefresh(2, 7);
}

// Result Format("%d:%s") d: 0~7, s: -- .. OK E0~Ef 
static void getChanResult(uint8_t idx, char *str)
{
    str[0] = '1' + idx; str[1] = ':'; str[4] = '\0';
    
    if (!(batch.gChans & (1 << idx)))
    {
        str[2] = '-'; str[3] = '-'; /* -- OFF */
        return;
    }

    if (batch.chsSet & (1 << idx))
    {
        str[2] = '.'; str[3] = '.'; /* .. Busy */
        return;
    }

    if (!(batch.chsErr & (1 << idx)))
    {
        str[2] = 'O'; str[3] = 'K'; /* OK */
        return;
    }
    
    str[2] = 'E'; sprintf(&str[3], "%x", bchns[idx].error);/* E0~Ef */
    //str[3] = '0' + bchns[idx].error - 1; 
    return;
}

static void showChanResult(void)
{
    char r_str[5]; //eg. "1:OK" "7:--"    
    uint8_t x = 14, y = 40; // chan1~4
    
    for (uint8_t i = 0; i < 8; i++)
    {
        if (i == 4)
        {
            x = 14; y = 52; // chan5~8
        }

        getChanResult(i, r_str);
        oledShowStr(x, y, r_str);
        x += 30; // 5*6
    }
}

static void getChanDetail(uint8_t idx, char *str, char *ret)
{
    char headStr[20];

    //@see pt_error
    const char *const RET_STR[] =
    {    
        "OK",
        "XX",
        
        "CHAN",
        "CFG",
        
        "BURN",
//        "CODE",        
        "RSP",
        "TIMEOUT",
//        "BOOT",
        "MACEND",
        
        "RUN",
        
        "TEST",
        "CURR",
        "XTAL",
        "GPIO",
        "RF",
        "UNKNOWN",
    };

    uint8_t error = bchns[idx].error;

    sprintf(headStr, "%d# %s", idx + 1, RET_STR[error]);

    switch (error)
    {
        case PERR_NO_ERROR :
        {
            sprintf(&headStr[5], "-%s%s%s",(gBchFirm.firmCFG.tConf & (1 << TEST_GPIO)) ? "IO " : "",(gBchFirm.firmCFG.tConf & (1 << TEST_XTAL)) ? "XL " : "",(gBchFirm.firmCFG.tConf & (1 << TEST_RF)) ? "RF " : "");
            
            if (gBchFirm.firmCFG.tConf & (1 << TEST_PWR))
            {
                uint8_t pwrCurr = ADC_TO_CURR(gBchTest[idx].pwrVadc, 1); //mV  10 ou

                sprintf(str, "%s%d.%dmA", headStr, pwrCurr / 10, pwrCurr % 10);                
            }
            else
            {   
                sprintf(str, "%s", headStr);
            }                                
        } break;
        
        case PERR_TEST_PWR :
        {
            uint8_t pwrCurr = ADC_TO_CURR(gBchTest[idx].pwrVadc, 1); //mV  10 ou

            sprintf(str, "%s-%d.%dmA", headStr, pwrCurr / 10, pwrCurr % 10);
        } break;

        case PERR_TEST_XTAL :
        {
            if (gBchTest[idx].xtalCnt != 0)
            {
                uint32_t value = ((gBchTest[idx].xtalCnt >= TIM_CNT_STD) ? (gBchTest[idx].xtalCnt - TIM_CNT_STD) : TIM_CNT_STD - 
                                  gBchTest[idx].xtalCnt) * 150;

                sprintf(str, "%s-%d.%02dKHz", headStr, value / 1000, (value % 1000) / 10);
            }
            else
            {
                sprintf(str, "%s-%s", headStr, "Connect?");
            }
        } break;

        case PERR_TEST_GPIO :
        {
            char *shortStr = (gBchTest[idx].gpioRes == IO_TEST_SHORTGND ? "GND" : (gBchTest[idx].gpioRes == IO_TEST_SHORTVDD ? 
                              "VDD" : "NEAR"));
            sprintf(str, "%s-%s", headStr, shortStr);
        } break;

        case PERR_TEST_RF :
        {
            sprintf(str, "%s-%d/20", headStr, gBchTest[idx].rfRxRec);
        } break;

        default:
        {
            sprintf(str, "%s", headStr);
        } break;
    }
}
