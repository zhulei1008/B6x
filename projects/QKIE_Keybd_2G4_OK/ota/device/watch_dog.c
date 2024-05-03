#include "hydrvs.h"
#include "watch_dog.h"

void wwdgInit(void)
{
    wwdgUnlock(1);
    // WWDG Count Clock APB
    wwdgClockSel(0);
    wwdgFeed(WWDG_WINDOW_CNT);
    wwdgOpen();
    wwdgRSTEnable();
}

void wwdgProc(void)
{
    uint32_t wwdg_cnt = getWWDGCnt();

    if (wwdg_cnt < WWDG_TRIG_FEED)
    {
        wwdgFeed(WWDG_WINDOW_CNT);
    }
}

void resetInit(void)
{
    uint32_t rst_flag = resetStaGet();

    if (rst_flag & RST_STA_BY_WWDG_MASK)
    {
        //        debug("System Reset By WWDG\r\n");

        // Clear Reset Flag
        //        resetStaClear(RST_STA_BY_WWDG_MASK);
    }

    //    debug("RST: 0x%02x\r\n", rst_flag);
    resetStaClear(0xFFFFFFFF);
}
