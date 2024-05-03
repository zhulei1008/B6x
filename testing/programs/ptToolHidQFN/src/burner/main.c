#include "b6x.h"
#include "drvs.h"
#include "trim.h"
#include "regs.h"

#include "sftmr.h"
#include "proto.h"
#include "burner.h"

#ifndef BIT
#define BIT(pos) (0x01UL << (pos))
#endif

static void sysInit(void)
{    
    // switch syclk to 48M for USB
    rcc_sysclk_set(SYS_CLK_48M);
    
    // enable USB clk and iopad
    rcc_usb_en();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    sftmr_init();
    
    burnerInit();
}

int main(void)
{
    sysInit();

    devInit();
    
    while (1)
    {
        sftmr_schedule();
            
        proto_schedule();
    }
}
