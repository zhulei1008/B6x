#include <string.h>
#include <stdbool.h>
#include "b6x.h"
#include "gpio.h"
#include "reg_gpio.h"
#include "reg_uart.h"
#include "reg_iopad.h"
#include "proto.h"
#include "burner.h"
#include "led.h"
#include "uart_itf.h"
#include "sftmr.h"

uint8_t sdrv_pgidx = 0;

static void xdelay(uint16_t x)
{
    while (x--)
    {
        __NOP();
        __NOP();
    }
}

void gpioControlInit(void)
{
    IOMCTL->PIOA[PIN_TX2].Word  = 0;
    IOMCTL->PIOA[PIN_RX2].Word  = 0;
    IOMCTL->PIOA[PIN_RST].Word  = 0;
    IOMCTL->PIOA[PIN_EN_PWR_CHIP].Word  = 0;
    IOMCTL->PIOA[PIN_PWM_CHIP].Word  = 0;

#if SC1000_QCR // IO_PWR 0630
    IOMCTL->PIOA[PIN_ADC_CHIP].Word  = 0x60; //Max Driver Strength
    IOMCTL->PIOA[PIN_PWM_CHIP].Word  = 0x60; //Max Driver Strength

    GPIOA->CLR    = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST) | BIT(PIN_EN_PWR_CHIP) | BIT(PIN_ADC_CHIP) | BIT(PIN_PWM_CHIP));
    GPIOA->DIRSET = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST) | BIT(PIN_EN_PWR_CHIP) | BIT(PIN_ADC_CHIP) | BIT(PIN_PWM_CHIP));

#else
    GPIOA->CLR    = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST) | BIT(PIN_EN_PWR_CHIP));
    GPIOA->DIRSET = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST) | BIT(PIN_EN_PWR_CHIP));
#endif
}

void gpioControlClear(void)
{
    //    return;

    IOMCTL->PIOA[PIN_TX2].Word  = 0;
    IOMCTL->PIOA[PIN_RX2].Word  = 0;
    IOMCTL->PIOA[PIN_PWM_CHIP].Word  = 0;

#if SC1000_QCR // IO_PWR 0630

    GPIOA->CLR    = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_ADC_CHIP) | BIT(PIN_PWM_CHIP));
    GPIOA->DIRSET = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST));

#else
    GPIOA->CLR    = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_PWM_CHIP) | BIT(PIN_EN_PWR_CHIP));
    GPIOA->DIRSET = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_PWM_CHIP) | BIT(PIN_RST));
#endif
}

static void enter_boot(void)
{
#if SC1000_QCR // IO_PWR 0630
    GPIOA->SET = (uint32_t)(BIT(PIN_ADC_CHIP) | BIT(PIN_PWM_CHIP));
#else
    GPIOA->SET = (uint32_t)(BIT(PIN_EN_PWR_CHIP));
#endif

    //@see gpioControlClear

    //    xdelay(45); //no cap (5)
    xdelay(1000 * 30); //15ms    0.1uF/0.5ms
    //    GPIOA->SET    = (uint32_t)(0x01 << PIN_RST);
    GPIOA->DIRCLR = (uint32_t)(BIT(PIN_RST));  //short

    //    GPIOA->CLR    = (uint32_t)(0x01 << PIN_RST);
    //    IOMCTL->PIOA[PIN_RST].IE  = 1;
    //    IOMCTL->PIOA[PIN_RST].PUD  = 1;
    //  ioPuPdSel(PIN_RST,IO_PDEN);

    //    xdelay(1000*13); //(1000/0.5ms) 6.5ms + 2.5ms
    xdelay(1000 * 30); //15ms
    uart2_init();
}

void sdrv_sync(void)
{
    enter_boot();

#if (0)
    pt_cmd_baud(921600);
    WAIT_UART_IDLE();

    UART2->LCR.BRWEN = 1;
    // 921600bps
    UART2->BRR = 0x11;
    UART2->LCR.BRWEN = 0;
    xdelay(1200);
#else

    //    gFirmInfo.chipBaud = 921600;
    uint32_t uart2_baud = gFirmInfo.chipBaud;

    if ((uart2_baud == 0xFFFFFFFF) || (uart2_baud == 0x00))
    {
        pt_cmd_sync();
        return ;
    }

    // Modify Boot Baudrate
    pt_cmd_baud(uart2_baud);
    WAIT_UART_IDLE(2);

    UART_MODIFY_BAUD(2, uart2_baud);
    pt_cmd_baud(uart2_baud);

#endif
}

void sdrv_rsp_baudend(uint8_t status)
{
    if (status == PT_OK)
    {
        pt_cmd_sync();
    }
}

void sdrv_rsp_sync(void)
{
    sdrv_pgidx = 0; //start load

    pt_cmd_cwr(CHIP_SDRV_ADDR, (uint8_t *)(FSH_ADDR_SDRV_BASE), RAM_PAGE_SIZE);
}

void sdrv_rsp_cwr(uint8_t status)
{
    uint16_t length = RAM_PAGE_SIZE;
    uint16_t pages  = gCodeInfo.sdrvLen / RAM_PAGE_SIZE;

    if (status == PT_OK)
    {
        sdrv_pgidx++; // next load
    }
    else
    {
        // repeat load
    }

    if (sdrv_pgidx == pages)
    {
        length = gCodeInfo.sdrvLen % RAM_PAGE_SIZE; // last load
    }

    if ((sdrv_pgidx > pages) || (length == 0))
    {
        if ((gFirmInfo.chipBaud != 0xFFFFFFFF) && (gFirmInfo.chipBaud != 0x00))
        {
            // modify chipSet uart1 baudrate.
            pt_cmd_swr(0x080000C0, gFirmInfo.chipBaud);
        }

        pt_cmd_jump(CHIP_SDRV_ADDR);
    }
    else
    {
        uint16_t offset = sdrv_pgidx * RAM_PAGE_SIZE;
        pt_cmd_cwr(CHIP_SDRV_ADDR + offset, (uint8_t *)(FSH_ADDR_SDRV_BASE + offset), length);
    }
}

void sdrv_rsp_jump(uint8_t status)
{
    // todo
    if (status == PT_OK)
    {
        if (burner.opCode & DOWN_CHIP_FIRM)
        {
            // wait RSP sync 1.76ms
            send_cmd_flag = true;
            send_cmd_time = currTickCnt();

            setBurnerState(BURNER_CHIPSRAM_SYNCED, ER_NULL);
        }
        else
        {
            setBurnerState(BURNER_OK, ER_NULL);
        }
    }
    else
    {
        setBurnerState(BURNER_FAIL, ER1_RSP);
    }

}
