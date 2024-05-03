#include "b6x.h"
#include "drvs.h"
#include "trim.h"
#include "reg_rcc.h"
#include "reg_aon.h"
#include "reg_rfip.h"
#include "reg_gpio.h"
#include "reg_iopad.h"
#include "reg_syscfg.h"
#include "flash.h"

#include "sftmr.h"
#include "proto.h"
#include "batch.h"

#ifndef BIT
#define BIT(pos) (0x01UL << (pos))
#endif

// 32pin double bonding pad
#define BOND_SWCLK_PAD      21
#define BOND_SWDIO_PAD      22
#define BOND_PA25_PAD       26

static void bondPadInit(void)
{
    // High impedance state
    GPIOA->DIRCLR = (BIT(BOND_SWCLK_PAD) | BIT(BOND_SWDIO_PAD) | BIT(BOND_PA25_PAD) | BIT(PIN_BEEP));
    IOMCTL->PIOA[BOND_SWCLK_PAD].Word = 0;
    IOMCTL->PIOA[BOND_SWDIO_PAD].Word = 0;
    IOMCTL->PIOA[BOND_PA25_PAD].Word  = 0;

    // beepInit: PIO07 PullUp(dflt) -> HighZ
    IOMCTL->PIOA[PIN_BEEP].Word  = 0;
    GPIOA->SET    = BIT(PIN_BEEP);
}

static void sysInit(void)
{
    // enable aon
    RCC->AAHBCEN.Word = 0x01;

    // bit0-rcc, 1-syscfg, 3-iopad, 8-csc, 16-gpio,
    // bit28-aahb, 29-apb1, 30-apb2, 31-apb3
    RCC->AHBRST.Word = 0x00010008;
    RCC->AHBRST.Word = 0x00;
    RCC->AHBCEN.Word = 0xF001010B;

    // bit16-uart1, 17-uart2
    RCC->APB1RST.Word = 0x00030000;
    RCC->APB1RST.Word = 0x00;
    RCC->APB1CEN.Word = 0x00030000;

    // bit4-dma, 13-iic, 28-qspi
    RCC->APB2RST.Word = 0x2010;
    RCC->APB2RST.Word = 0x00;
    RCC->APB2CEN.Word = 0x10002010;

    // bit29-rf
    RCC->APB3CEN.Word = 0x20000000;

    // click reset copy flash to sram
    AON->BKUPAON00 = 0;

    // sysBandgapAdj
    AON->MISCAONCTRL.LPBG_EN_V33 = 1;
    // voltage 1.2v  output enable, set default value 0
    //    SYSCFG->LPBGCTRL.VTR = 0x25;   //(MIN:0x00 - MAX:0xFF)
    //    SYSCFG->LPBGCTRL.VTST = 1;

    rf->reg08.bg_res_trimh = 0x34;   //VDD_DIG > 1.2V

    bondPadInit();
    GLOBAL_INT_START();
}

static void devInit(void)
{
    if (!flashInit())
    {
        //batch.state |= BATCH_FLASHINIT_FAILED;
        //return ;
    }

    // Load ft program trim
    TRIM_REG_INIT();

    sfTmrInit();

    batchInit();
}

int main(void)
{
    sysInit();

    devInit();

    while (1)
    {
        sfTmrPolling();

        proto_schedule();
    }
}
