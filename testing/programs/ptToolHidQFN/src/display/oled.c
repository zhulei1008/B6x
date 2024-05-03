#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "sys.h"
#include "rcc.h"
#include "iopad.h"
#include "iic.h"
#include "sftmr.h"
#include "font.h"
#include "oled.h"
#include "batch.h"
#include "chans.h"
#include "xtal_test.h"
#include "gpio_test.h"

/*
 * DRIVER FUNCTION
 ****************************************************************************************
 */

#define SH1106             1
#define SSD1306            0
#define DISP_MODE          1  // 1 - Normal, 0 - Inverse

#define OLED_ADR           0x78
#define OLED_CMD           0x00
#define OLED_DAT           0x40

uint8_t OLED_GRAM[8][128]; // 128*64dpi

// Clear in lines(0~7)
void oledClear(uint8_t sl, uint8_t el)
{
    memset(OLED_GRAM[sl], 0, (el - sl + 1) * 128);
}

// Draw Point(x:0~127, y:0~63, t:1-fill 0-clear)
void oledDrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t line = y / 8;
    uint8_t pos = 1 << (y % 8);
#if (DISP_MODE)
    if (t)
        OLED_GRAM[line][x] |= pos; // set
    else
        OLED_GRAM[line][x] &= ~pos; // clr
#else
    if (t)
        OLED_GRAM[line][x] &= ~pos; // clr
    else
        OLED_GRAM[line][x] |= pos; // set
#endif
}

void oledDrawLine(uint8_t y)
{
    uint8_t line = y / 8;
    uint8_t pos = 1 << (y % 8);

    for (uint8_t x = 0; x < 128; x++)
    {
        OLED_GRAM[line][x] |= pos;
    }
}
// Draw Byte(x:0~127, y:0~63-8, v:value, bits:1~8)
void oledDrawByte(uint8_t x, uint8_t y, uint8_t v, uint8_t bits)
{
    uint8_t row = y / 8;
    uint8_t rem = y % 8;

#if (1)
    if (bits + rem <= 8) // one page
    {
        uint8_t msk = (bits == 8) ? 0xFF : ((1 << bits) - 1);

        OLED_GRAM[row][x] &= ~(msk << rem);
        OLED_GRAM[row][x] |= (v << rem);
    }
    else // two pages
    {
        OLED_GRAM[row][x] &= ((1 << rem) - 1);
        OLED_GRAM[row][x] |= ((v & ((1 << (8 - rem)) - 1)) << rem);

        OLED_GRAM[row + 1][x] &= ~((1 << (bits + rem - 8)) - 1);
        OLED_GRAM[row + 1][x] |= (v >> (8 - rem));
    }
#else
    if (rem == 0) // page aligned
    {
        OLED_GRAM[row][x] = v;
    }
    else
    {
        uint8_t lmsk = (1 << rem) - 1;
        uint8_t hmsk = (1 << (8 - rem)) - 1;

        OLED_GRAM[row][x] &= lmsk;
        OLED_GRAM[row][x] |= ((v & hmsk) << rem);

        OLED_GRAM[row + 1][x] &= ~lmsk;
        OLED_GRAM[row + 1][x] |= (v >> (8 - rem)); //((v & ~hmsk) >> (8-rem));
    }
#endif
}

// Show Char(x:0~127-w, y:0~63-h, font:6x12)
#define CHAR_W             6
#define CHAR_H             12
#define CHAR_SIZE          CHAR_W*((CHAR_H + 7)/8) // = 12
void oledShowChar(uint8_t x, uint8_t y, uint8_t chr)
{
    const uint8_t *zmod = asc2_1206[chr - ' '];

    for (uint8_t i = 0; i < CHAR_SIZE; i++)
    {
        uint8_t h = (i < CHAR_W) ? 8 : CHAR_H - 8; // top align

        if (i == CHAR_W)
        {
            x -= CHAR_W;
            y += 8;
        }
        oledDrawByte(x, y, zmod[i], h);
        x++;
    }
}

// Show String: wrapper
void oledShowStr(uint8_t x, uint8_t y, const char *str)
{
    while ((*str >= ' ') && (*str <= '~'))
    {
        oledShowChar(x, y, *str);
        x += CHAR_W;

        str++;
    }
}

// Show GBK(x:0~127-16, y:0~63-16, id:index, font:16x16)
#define GBK_W             16
#define GBK_H             16
#define GBK_SIZE          GBK_W*((GBK_H + 7)/8) // = 32
void oledShowGBK(uint8_t x, uint8_t y, uint8_t id)
{
    const uint8_t *zmod = Hzk1[id];

    for (uint8_t i = 0; i < GBK_SIZE; i++)
    {
        if (i == GBK_W)
        {
            x -= GBK_W;
            y += 8;
        }
        oledDrawByte(x, y, zmod[i], 8);
        x++;
    }
}

void oledShowGBKs(uint8_t x, uint8_t y, uint8_t sid, uint8_t eid)
{
    for (uint8_t id = sid; id <= eid; id++)
    {
        oledShowGBK(x, y, id);
        x += GBK_W; // next
    }
}

// Show PIC(width:x+w<128, height:y+h<64, pic:w*h)
void oledShowBMP(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *pic)
{
    uint8_t col, rows = (h + 7) / 8;

    for (uint8_t i = 0; i < rows; i++)
    {
        col = x; // reback

        for (uint8_t j = 0; j < w; j++)
        {
            oledDrawByte(col, y, *pic, 8);
            col++;
            pic++;
        }

        y += 8; // next row
    }
}

#define I2C_WAIT_TIME 2
static bool i2cWaitStatus(uint8_t status)
{
    uint32_t oldtick = currTickCnt();

    do
    {
        if (i2cGetStatus() == status)
            return true;
    }
    while ((uint32_t)(currTickCnt() - oldtick) < I2C_WAIT_TIME);

    return false;
}

bool oledWriteBytes(uint8_t type, uint8_t len, const uint8_t *array)
{
    i2cSendStart();
    if (i2cWaitStatus(I2C_MS_START_OK))
    {
        i2cSendData(OLED_ADR);
        i2cClearFlag();
        if (i2cWaitStatus(I2C_MS_SEND_ADDR_W_ACK))
        {
            i2cSendData(type); // cmd:0x00,data:0x40
            i2cClearFlag();
            if (i2cWaitStatus(I2C_MS_SEND_DAT_ACK))
            {
                do
                {
                    i2cSendData(*array++);
                    i2cClearFlag();
                    if (!i2cWaitStatus(I2C_MS_SEND_DAT_ACK))
                    {
                        break;
                    }
                }
                while (--len);

                i2cSendStop();
                return (len == 0);
            }
        }
    }

    i2cSendStop();
    return false;
}

bool oledWriteSingle(uint8_t type, uint8_t val)
{
    return oledWriteBytes(type, 1, &val);
}

// Scroll in SSD1306
void oledScroll(void)
{
    const uint8_t srlcmd[] = {0x2E, 0x27, 0x00, 0x00, 0x07, 0x01, 0x00, 0xFF, 0x2F};

    oledWriteBytes(OLED_CMD, sizeof(srlcmd), srlcmd);
}

// Refresh in lines(0~7): start <= end
void oledRefresh(uint8_t sl, uint8_t el)
{
    uint8_t line[3] =
    {
        0xB0,  // 设置行起始地址
#if (SSD1306)
        0x00,  // 设置低列起始地址
#else
        0x02,  // 设置低列起始地址
#endif
        0x10,  // 设置高列起始地址
    };

    while (sl <= el)
    {
        line[0] = 0xB0 + sl; // line head
        oledWriteBytes(OLED_CMD, 3, line);

        oledWriteBytes(OLED_DAT, 128, OLED_GRAM[sl]);
        sl++; // next line
    }
}

bool oledInit(void)
{
    i2cSoftReset();
    ioSelI2c(PIN_SCL_I2C, PIN_SDA_I2C);
    i2cClkSet(3, 1);  // sampleClk:2MHz=(16MHz/2^3), sclClk:100KHz=(16MHz/(2^3 *(1+1)*10))
    i2cCtrl(1);

    const uint8_t init_arr[] =
    {
        0xAE,  // 关显示
#if (SSD1306)
        0x00,  // ---设置列地址低四位（00-0F）。（0 0 0 0 A3 A2 A1 A0）
#else
        0x02,  // 设置列地址低四位（00-0F）。（0 0 0 0 A3 A2 A1 A0）
#endif
        0x10,  // 设置列地址高四位（10-1F）。（0 0 0 1 A7 A6 A5 A4）
        0x40,  // 指定起始行地址（40-7F），共64行（每行一像素）
#if (!SSD1306)
        0xB0,  // 指定起始页地址（B0-B7），共8页（每页8像素）
#endif

        0x81,  // 启用对比度数据寄存器。双字节命令。
        0xCF,  // 对比度设置（00-FF），共256个级别。设置完对比度数据后，对比度数据寄存器自动释放。

        0xA1,  // A1：正常显示，A0：水平反转显示。当显示数据被写入或读取时，列地址增加1。
        0xC8,  // C0-C7：正常显示，C8-CF：垂直翻转。设置公共输出扫描方向。
        0xA6,  // A6：关反显，A7：开反显。在不重写显示数据RAM内容的情况下反转显示开/关状态。

        0xA8,  // 设置多路复用比率。此命令将默认的64种多路传输模式切换到1到64之间的任意多路传输比率。双字节命令，设置完这条命令后需要写入比率数据。
        0x3f,  // 多路定量数据集（00-3F）。3F：64路多路传输比率。

        0xD3,  // 设置显示偏移模式。双字节命令，设置完这条命令后需要写入显示偏移模式数据。
        0x00,  // 偏移量（00-3F）。00：不偏移。

        0xd5,  // 设置显示时钟分频比/振荡器频率。双字节命令，设置完这条命令后需要写入显示时钟分频比/振荡器频率数据。
        0x80,  // 分频比/振荡器频率数据（00-FF）。0x80：分频比为1，振荡器频率为+15%

        0xD9,  // 设置预充电周期的持续时间。间隔以DCLK的个数计。POR是2个DCLKs。双字节命令，设置完这条命令后需要写入预充电周期的持续时间。
        0xF1,  // 周期持续时间（00-FF）。0xF1：预充电周期的持续时间为1个DCLK，掉电周期的持续时间为15个DCLK。

        0xDA,  // 设置公用焊盘硬件配置。此命令用于设置公共信号板配置（顺序或替代）以匹配OLED面板硬件布局。双字节命令，设置完这条命令后需要写入公用焊盘硬件配置
        0x12,  // 顺序/替代模式设置（02-12）。0x02：顺序模式。0x12：替代模式。

        0xDB,  // 设置VCOM取消选择级别。此命令用于在取消选择阶段设置公共焊盘输出电压电平。双字节命令。
        0x40,  // VCOM取消选择级别数据（00-FF）。

#if (SSD1306)
        0x20,  // -Set Page Addressing Mode (0x00/0x01/0x02)
        0x02,  //

        0x8D,  // --set Charge Pump enable/disable
        0x14,  // --set(0x10) disable
#endif

        0xA4,  // 关闭/打开整个显示。0xA4：正常显示。0xA5：整个显示。此命令的优先级高于正常/反向显示命令。
        0xA6,  //  Disable Inverse Display On (0xa6/a7)
        0xAF,  // 开显示
    };

    return oledWriteBytes(OLED_CMD, sizeof(init_arr), init_arr);
}


/*
 * Display FUNCTION
 ****************************************************************************************
 */

#define OLED_Refresh_PAGE() oledRefresh(2, 7);

static uint32_t btIconBlinker(uint8_t id)
{
    if (!(batch.state & BATCH_BUSY))
    {
        const uint8_t *btIcon = (OLED_GRAM[0][119] == 0x00) ? BMP_BLULOGO : BMP_BLULOGO_OFF;

        oledShowBMP(119, 0, 8, 16, btIcon);
        oledDrawLine(15);
        oledRefresh(0, 1);
    }

    return 50;
}

static uint32_t enterInfoScreen(uint8_t id)
{
    // todo
    oledInfoPage();

    sfTmrStart(50, btIconBlinker);
    return 0;
}

void oledPlay(void)
{

}

static void getChanResult(uint8_t idx, char *str)
{
    const char *const RET_STR[] =
    {
        "OK",
        "--",
        "E0",
        "E1",
        "E2",
        "E3",
        "E4",
        "E5",
        "E6",
        "E7",
        "E8",
        "E9",
        "..",
    };
    //const char* stastr = "OK";
    uint8_t ret_idx = 0;

    if (batch.state & BATCH_ON_LINE)
    {
        if ((batch.chsSet >> idx) & 0x01)
        {
            ret_idx = 14;
        }
        else
        {
            if ((batch_gChanConect >> idx) & 0x01)
            {
                ret_idx = getBurnerFirmResult(idx);
            }
            else
            {
                ret_idx = 3;
            }
        }

        sprintf(str, "%d:%s", idx + 1, RET_STR[ret_idx - 2]);
    }
    else
    {
        if ((batch.chsSet >> idx) & 0x01)
        {
            ret_idx = 12;
        }
        else
        {
            ret_idx = getChipFirmResult(idx);
        }

        sprintf(str, "%d:%s", idx + 1, RET_STR[ret_idx]);
    }
}

void infoUpData(void)
{
    uint8_t x, y;
    char chanStr[6];  
    
    x = 14; y = 40; // chan1~4
    for (uint8_t i = 0; i < 8; i++)
    {
        if (i == 4)
        {
            x = 14; y = 52; // chan5~8
        }

        getChanResult(i, chanStr);
        oledShowStr(x, y, chanStr);
        x += 30; // 5*6
    }
}

void oledHomePage(void)
{
    if (oledInit())
    {
        char verStr[6];
        sprintf(verStr, "V%d.%02d", (BATCH_VERSION >> 8) & 0x0F, (BATCH_VERSION >> 0) & 0xFF);

        // Title Bar(line0~15) - Always ON
        oledShowBMP(  0, 0, 88, 16, BMP_TESETLOGO);
        oledShowStr(88,  4, verStr);
        oledDrawLine(15);

        // Welcome Info
        oledShowGBKs(16, 24, 0, 5);  // "量产测试平台"
        oledShowGBKs(24, 48, 6, 10); // "弘亿微电子"

        oledRefresh(0, 7);
        oledClear(2, 7);

        sfTmrStart(100, enterInfoScreen);
    }
}

void oledInfoPage(void)
{
    char cntStr[22];
    uint8_t chs = chansDetect();

    sprintf(cntStr, "[%c] Crc:%X", ~gFirmInfo.codeCRC ? 'Y' : 'X', gFirmInfo.codeCRC);
    oledShowStr(0, 16, cntStr);

    sprintf(cntStr, "[%c] CHs:%d(0x%X)", (chs & batch_gChanConect) == batch_gChanConect ? 'Y' : 'X', __builtin_popcount(batch_gChanConect),
            batch_gChanConect);
    oledShowStr(0, 28, cntStr);

    sprintf(cntStr, "[X] Cfg:%X", gFirmInfo.macConfigure);
    oledShowStr(0, 40, cntStr);

    sprintf(cntStr, "[X] Cnt:%d/%d", getCountNum(),
            ~gFirmInfo.macConfigure ? (((gFirmInfo.macConfigure >> 8) & 0xFFFFFF) * __builtin_popcount(batch_gChanConect)) : 0);
    oledShowStr(0, 52, cntStr);

    if (batch.state & BATCH_ON_LINE)
    {
        oledShowBMP(109, 20, 18, 14, BMP_USB_LOGO);
    }

    OLED_Refresh_PAGE();
    oledClear(2, 7);
}

static uint8_t err_idx = 0;

void oledDonePage(void)
{
    err_idx = 0;

    char cntStr[10];

    bool busy = (batch.state & BATCH_BUSY) != 0 ? true : false;

    infoUpData();

    if (batch.state & BATCH_ON_LINE)
    {
        oledShowBMP(109, 20, 18, 14, BMP_USB_LOGO);
    }

    if (busy)
    {
        oledShowGBKs(0, 20, 11, 12); /*"正",11  "在",12*/

        switch (batch.opCode)
        {
            case UP_BURNER_DATE :
                oledShowGBKs(32, 20, 22, 23); /*"更",22  "新",23*/
                break;

            case DOWN_BURNER_FIRM :
                oledShowGBKs(32, 20, 24, 25); /*"配",24  "置",25*/
                break;

            default :
                oledShowGBKs(32, 20, 15, 16); /*"执",15  "行",16*/
                break;
        }
        oledShowStr(64, 24, "...");
    }
    else
    {
        uint8_t set_num, fail_num = 0;

        set_num = __builtin_popcount(batch_gChanConect);

        for (uint8_t i = 0; i < CHANS_CNT; i++)
        {
            // BURNER_DOWMNLOAD_OLD:0, fail, 1, ok
            if ((batch_gChanConect >> i) & 0x01)
            {
                if (batch.state & BATCH_ON_LINE)
                {
                    if (!getBurnerFirmResult(i))
                    {
                        fail_num++;
                    }
                }
                else
                {
                    if (burner_Info[i].burner.state & FAIL_MASK)
                    {
                        fail_num++;
                    }
                }
            }
        }

        switch (batch.opCode)
        {
            case UP_BURNER_DATE :
                oledShowGBKs(0, 20, 22, 23); /*"更",22  "新",23*/
                break;

            case DOWN_BURNER_FIRM :
                oledShowGBKs(0, 20, 24, 25); /*"配",24  "置",25*/
                break;

            default :
                oledShowGBKs(0, 20, 15, 16); /*"执",15  "行",16*/
                break;
        }

        oledShowGBKs(32, 20, 18, 19); /*"完",18  "成",19*/

        sprintf(cntStr, ":%d/%d", set_num - fail_num, set_num);
        oledShowStr(64, 24, cntStr);
    }

    OLED_Refresh_PAGE();
    oledClear(2, 7);
}




static void getChanErrDetail(uint8_t idx, char *str)
{
    char headStr[12];

    const char *const RET_STR[] =
    {
        //        "OK",
        //        "--",
        "CODE",
        "RSP",
        "TIMEOUT",
        "BOOT",
        "MACEND",
        "CURR",
        "XTAL",
        "GPIO",
        "RF",
        "UNKNOWN",
    };
    //const char* stastr = "OK";

    uint8_t err_idx = getChipFirmResult(idx);

    sprintf(headStr, "%d:%s", idx + 1, RET_STR[err_idx - 2]);

    switch (err_idx)
    {
        case ER5_TEST_CURR :
        {
            uint8_t workcurr = (uint8_t)(((burner_Info[idx].test_result.workcurr * 3300) / 0xFFF)); //mV  10 ou

            sprintf(str, "%s-%d.%dmA", headStr, workcurr / 10, workcurr % 10);
        } break;

        case ER6_TEST_XTAL :
        {
            uint32_t value = ((burner_Info[idx].test_result.freq >= TIM_CNT_STD) ? (burner_Info[idx].test_result.freq - TIM_CNT_STD) : TIM_CNT_STD -
                              burner_Info[idx].test_result.freq) * 150;

            sprintf(str, "%s-%d.%02dKHz", headStr, value / 1000, (value % 1000) / 10);
        } break;

        case ER7_TEST_GPIO :
        {
            char *shortStr = (burner_Info[idx].test_result.gpioRes == IO_TEST_SHORTGND ? "GND" : (burner_Info[idx].test_result.gpioRes == IO_TEST_SHORTVDD ?
                              "VDD" : "NEAR"));
            sprintf(str, "%s-%s", headStr, shortStr);
        } break;

        case ER8_TEST_RF :
        {
            sprintf(str, "%s-%d/20", headStr, burner_Info[idx].test_result.rfRxRec);
        } break;

        default:
        {
            sprintf(str, "%s", headStr);
        } break;
    }
}

void infoErrData(void)
{
    //    static uint8_t idx = 0;

    char chanStr[20];

    uint8_t x = 8, y = 16;

    for (uint8_t i = err_idx; i < 8; i++)
    {
        if (((batch_gChanConect >> i) & 0x01) && ((burner_Info[i].burner.state & FAIL_MASK)))
        {
            if (y == 64) // reback
            {
                err_idx = i;
                return;
            }

            getChanErrDetail(i, chanStr);
            oledShowStr(x, y, chanStr);
            y += 12;
        }
    }

    if (err_idx) err_idx = 0;
}

void oledDetailPage(void)
{
    infoErrData();

    OLED_Refresh_PAGE();
    oledClear(2, 7);
}
