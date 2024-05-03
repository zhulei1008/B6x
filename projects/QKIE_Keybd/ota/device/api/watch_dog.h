#ifndef _WATCH_DOG_H_
#define _WATCH_DOG_H_

#define WWDG_WINDOW_CNT      16000000
#define WWDG_TRIG_FEED       6000000

void wwdgInit(void);

void wwdgProc(void);

void resetInit(void);

#endif  // _WATCH_DOG_H_
