#ifndef _BURNER_H_
#define _BURNER_H_

#include <stdint.h>
#include "ptdefs.h"

extern sdrvInfo_t gChnSdrv;
extern firmInfo_t gChnFirm;
extern macInfo_t  gMacInfo;
extern testInfo_t gTestRet;

extern burner_t gBurner;

void burnerInit(void);

void burnerSet(uint8_t state, uint8_t detail);

void burnerEvt(uint8_t state, uint8_t opcode);

#define burnerOK()          burnerSet(PSTA_OK,    PERR_NO_ERROR)
#define burnerFail(error)   burnerSet(PSTA_FAIL,  error)
#define burnerError(error)  burnerSet(PSTA_ERROR, error)
#define burnerBusy(opCode)  burnerSet(PSTA_BUSY,  opCode)

#define BURNER_IS_BUSY()    (gBurner.state & PSTA_BUSY)
#define BURNER_IS_IDLE()    !(gBurner.state & (PSTA_BUSY | PSTA_ERROR))
#define BURNER_IS_ONLINE()  (gBurner.state & PSTA_ONLINE)
#endif // _BURNER_H_
