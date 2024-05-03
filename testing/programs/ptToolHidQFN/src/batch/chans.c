#include "b6x.h"
#include "reg_gpio.h"
#include "reg_iopad.h"
#include "chans.h"

#define IO_MAX_DRIVE_STR   ((uint32_t)0x00000060)
#define IO_PULL_DOWN       ((uint32_t)0x00000080)
#define IO_PULL_UP         ((uint32_t)0x00000100)
#define IO_INPUT_BIT       ((uint32_t)0x00000200)
#define IO_OUTPUT_CFG      (IO_MAX_DRIVE_STR | IO_PULL_UP)

#define BIT_CS             (0x01UL << PIN_BT_CS)
#define BIT_S0             (0x01UL << PIN_BT_S0)
#define BIT_S1             (0x01UL << PIN_BT_S1)
#define BIT_S2             (0x01UL << PIN_BT_S2)
#define BIT_RST            (0x01UL << PIN_RST)

#define BT_EN(bit)         GPIOA->CLR = bit
#define BT_DIS(bit)        GPIOA->SET = bit

#define BT_LOW(bits)        GPIOA->CLR = bits
#define BT_HIGH(bits)       GPIOA->SET = bits


static uint8_t chansSel;

static void nopDelay(uint16_t x)
{
    while (x--)
    {
        __NOP();
        __NOP();
    }
}

void chansInit(void)
{
    //IOMCTL->PIOA[PIN_BT_S0].Word = IO_OUTPUT_CFG;
    //IOMCTL->PIOA[PIN_BT_S1].Word = IO_OUTPUT_CFG;
    //IOMCTL->PIOA[PIN_BT_S2].Word = IO_OUTPUT_CFG;
    //IOMCTL->PIOA[PIN_BT_CS].Word = IO_OUTPUT_CFG;

    IOMCTL->PIOA[PIN_RST].Word = IO_INPUT_BIT;
    GPIOA->SET = BIT_RST; // Input&Output

    // default select channel0
    GPIOA->CLR = (BIT_CS | BIT_S0 | BIT_S1 | BIT_S2);
    // output direction
    GPIOA->DIRSET = (BIT_CS | BIT_S0 | BIT_S1 | BIT_S2 | BIT_RST);
    chansSel = 0;

    chansReset();
}
//#include "iopad.h"
//#include "uart_itf.h"

void chansSelect(uint8_t idx)
{   
//    GPIOA->DIRSET = (0x01UL << PIN_BEEP);  //test for chansSel == 0;
    
    if ((chansSel == idx) || (idx >= CHANS_CNT))  return; // already or exceed             

#if (1)
    // pin data-out
    uint32_t dt = GPIOA->DT;

    dt &= ~(BIT_CS | BIT_S0 | BIT_S1 | BIT_S2);
    if (idx & 0x01) dt |= BIT_S0;
    if (idx & 0x02) dt |= BIT_S1;
    if (idx & 0x04) dt |= BIT_S2;
    GPIOA->DT = dt;
    
//   GPIOA->DIRCLR = (0x01UL << PIN_BEEP);
#else
    
    BT_DIS(BIT_CS);

    if (idx & 0x01)
        BT_HIGH(BIT_S0);
    else
        BT_LOW(BIT_S0);

    if (idx & 0x02)
        BT_HIGH(BIT_S1);
    else
        BT_LOW(BIT_S1);

    if (idx & 0x04)
        BT_HIGH(BIT_S2);
    else
        BT_LOW(BIT_S2);

    BT_EN(BIT_CS);
#endif

    chansSel = idx;
}

void chansReset(void)
{
    //GPIOA->DIRSET = BIT_RST;
    GPIOA->CLR = BIT_RST;
    for (uint8_t i = 0; i < CHANS_CNT; i++)
    {
        chansSelect(i);
        nopDelay(20);
    }
    GPIOA->SET = BIT_RST;
    //GPIOA->DIRCLR = BIT_RST;
}

uint8_t chansDetect(void)
{
    uint8_t chans = 0;

    for (uint8_t i = 0; i < CHANS_CNT; i++)
    {
        // keep output high to change
        //GPIOA->DIRSET = BIT_RST;
        chansSelect(i);

        // pulldown to input detect
        GPIOA->DIRCLR = BIT_RST;
        IOMCTL->PIOA[PIN_RST].PUD = 1;
        nopDelay(16);
        chans |= (((GPIOA->PIN >> PIN_RST) & 0x01) << i);
        IOMCTL->PIOA[PIN_RST].PUD = 0;
        GPIOA->DIRSET = BIT_RST;
    }

    return chans;
}
