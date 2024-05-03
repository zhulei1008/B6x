#include <string.h>
#include "proto.h"
#include "pt_env.h"
#include "uart_itf.h"
/// Protocol API
extern void pt_sch_init(proto_t *pt, uint8_t *buf, read_fnct read, parse_fnct parse);
extern void pt_sch_proc(proto_t *pt);

/// Proto: Host <--> Local -- UART1
#if (CFG_UART1)
static proto_t gPT2Host;
static uint8_t gHostBuf[PKT_MAX_SIZE];
#endif

/// Proto: Local <--> Chip -- UART2
#if (CFG_UART2)
static proto_t gPT2Chip;
static uint8_t gChipBuf[PKT_MAX_SIZE];
#endif

void proto_init(uint8_t chnl, parse_fnct parse)
{
#if (CFG_UART1)
    if (chnl == PT_HOST)
    {
        #ifdef ROLE_BURNER // 主机
            usbdInit(); 
            pt_sch_init(&gPT2Host, gHostBuf, usbd_read, parse);                 
        #else
            uart1_init(NULL);
            pt_sch_init(&gPT2Host, gHostBuf, uart1_read, parse);            
        #endif
    }
#endif

#if (CFG_UART2)
    else //if ((chnl == PT_CHIP) || (chnl == PT_CHAN))
    {
        #if (USBHIDHOST)
            uart2_init(HOST_UART_BAUD);
        #else
            if (chnl == PT_CHAN)  uart2_init(NULL);
        #endif
           
        
        pt_sch_init(&gPT2Chip, gChipBuf, uart2_read, parse);
    }
#endif
}

extern void usbdTest(void);

void proto_schedule(void)
{
#if (CFG_UART1)
    pt_sch_proc(&gPT2Host);
    #ifdef ROLE_BURNER // 主机
        usbdTest();
    #else
        uart1_proc();    
    #endif
#endif

#if (CFG_UART2)
    pt_sch_proc(&gPT2Chip);
#endif
}
