#include <string.h>
#include "proto.h"
#include "pt_env.h"
#include "ota_itf.h"
#include "prf_otas.h"
#include "app.h"

/// Protocol API
extern void pt_sch_init(proto_t *pt, uint8_t *buf, read_fnct read, parse_fnct parse);
extern void pt_sch_proc(proto_t *pt);

/// Proto: Host <--> Local -- UART1
#if (CFG_UART1 || OTA_CHIP)
static proto_t gPT2Host;
static uint8_t gHostBuf[PKT_MAX_SIZE];
#endif

/// Proto: Local <--> Chip -- OTA
#if (OTA_HOST)
static proto_t gPT2Chip;
static uint8_t gChipBuf[PKT_MAX_SIZE];
#endif

void proto_init(uint8_t chnl, parse_fnct parse)
{
#if (CFG_UART1)
    if (chnl == PT_HOST)
    {
        uart1_init();
        pt_sch_init(&gPT2Host, gHostBuf, uart1_read, parse);
    }
#endif

#if (OTA_CHIP)
    if (chnl == PT_HOST)
    {
        pt_sch_init(&gPT2Host, gHostBuf, ota_read, parse);
    }
#endif

#if (OTA_HOST)	
      else //if ((chnl == PT_CHIP) || (chnl == PT_CHAN))	
     {
//        if (chnl == PT_CHAN) uart2_init();
        pt_sch_init(&gPT2Chip, gChipBuf, ota_read, parse);
     }
#endif
}

void proto_schedule(void)
{
#if (CFG_UART1)
    pt_sch_proc(&gPT2Host);
    uart1_proc();
#endif

#if (OTA_CHIP)
    pt_sch_proc(&gPT2Host);
#endif

#if (OTA_HOST)
    pt_sch_proc(&gPT2Chip);
#endif
}


