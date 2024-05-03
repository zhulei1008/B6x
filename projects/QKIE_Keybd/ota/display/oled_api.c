#include <string.h>
#include <stdbool.h>

#include "iopad.h"
#include "iic.h"
#include "sftmr.h"
#include "oled.h"

/*
 * DRAW&SHOW FUNCTION
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
    const uint8_t *zmod = ASCII_1206[chr - ' '];

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
    const uint8_t *zmod = GBK_16[id];

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

/*
 * DRIVER FUNCTION
 ****************************************************************************************
 */

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

static bool oledWriteBytes(uint8_t type, uint8_t len, const uint8_t *array)
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
        0xB0,  // ��������ʼ��ַ
#if (SSD1306)
        0x00,  // ���õ�����ʼ��ַ
#else
        0x02,  // ���õ�����ʼ��ַ
#endif
        0x10,  // ���ø�����ʼ��ַ
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
        0xAE,  // ����ʾ
#if (SSD1306)
        0x00,  // ---�����е�ַ����λ��00-0F������0 0 0 0 A3 A2 A1 A0��
#else
        0x02,  // �����е�ַ����λ��00-0F������0 0 0 0 A3 A2 A1 A0��
#endif
        0x10,  // �����е�ַ����λ��10-1F������0 0 0 1 A7 A6 A5 A4��
        0x40,  // ָ����ʼ�е�ַ��40-7F������64�У�ÿ��һ���أ�
#if (!SSD1306)
        0xB0,  // ָ����ʼҳ��ַ��B0-B7������8ҳ��ÿҳ8���أ�
#endif

        0x81,  // ���öԱȶ����ݼĴ�����˫�ֽ����
        0xCF,  // �Աȶ����ã�00-FF������256������������Աȶ����ݺ󣬶Աȶ����ݼĴ����Զ��ͷš�

        0xA1,  // A1��������ʾ��A0��ˮƽ��ת��ʾ������ʾ���ݱ�д����ȡʱ���е�ַ����1��
        0xC8,  // C0-C7��������ʾ��C8-CF����ֱ��ת�����ù������ɨ�跽��
        0xA6,  // A6���ط��ԣ�A7�������ԡ��ڲ���д��ʾ����RAM���ݵ�����·�ת��ʾ��/��״̬��

        0xA8,  // ���ö�·���ñ��ʡ������Ĭ�ϵ�64�ֶ�·����ģʽ�л���1��64֮��������·������ʡ�˫�ֽ���������������������Ҫд��������ݡ�
        0x3f,  // ��·�������ݼ���00-3F����3F��64·��·������ʡ�

        0xD3,  // ������ʾƫ��ģʽ��˫�ֽ���������������������Ҫд����ʾƫ��ģʽ���ݡ�
        0x00,  // ƫ������00-3F����00����ƫ�ơ�

        0xd5,  // ������ʾʱ�ӷ�Ƶ��/����Ƶ�ʡ�˫�ֽ���������������������Ҫд����ʾʱ�ӷ�Ƶ��/����Ƶ�����ݡ�
        0x80,  // ��Ƶ��/����Ƶ�����ݣ�00-FF����0x80����Ƶ��Ϊ1������Ƶ��Ϊ+15%

        0xD9,  // ����Ԥ������ڵĳ���ʱ�䡣�����DCLK�ĸ����ơ�POR��2��DCLKs��˫�ֽ���������������������Ҫд��Ԥ������ڵĳ���ʱ�䡣
        0xF1,  // ���ڳ���ʱ�䣨00-FF����0xF1��Ԥ������ڵĳ���ʱ��Ϊ1��DCLK���������ڵĳ���ʱ��Ϊ15��DCLK��

        0xDA,  // ���ù��ú���Ӳ�����á��������������ù����źŰ����ã�˳����������ƥ��OLED���Ӳ�����֡�˫�ֽ���������������������Ҫд�빫�ú���Ӳ������
        0x12,  // ˳��/���ģʽ���ã�02-12����0x02��˳��ģʽ��0x12�����ģʽ��

        0xDB,  // ����VCOMȡ��ѡ�񼶱𡣴�����������ȡ��ѡ��׶����ù������������ѹ��ƽ��˫�ֽ����
        0x40,  // VCOMȡ��ѡ�񼶱����ݣ�00-FF����

#if (SSD1306)
        0x20,  // -Set Page Addressing Mode (0x00/0x01/0x02)
        0x02,  //

        0x8D,  // --set Charge Pump enable/disable
        0x14,  // --set(0x10) disable
#endif

        0xA4,  // �ر�/��������ʾ��0xA4��������ʾ��0xA5��������ʾ������������ȼ���������/������ʾ���
        0xA6,  //  Disable Inverse Display On (0xa6/a7)
        0xAF,  // ����ʾ
    };

    return oledWriteBytes(OLED_CMD, sizeof(init_arr), init_arr);
}
