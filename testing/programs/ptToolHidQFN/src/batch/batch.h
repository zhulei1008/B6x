#ifndef _BATCH_H_
#define _BATCH_H_

#include <stdint.h>
#include <stdbool.h>
#include "ptdefs.h"
#include "chans.h"

extern firmInfo_t gBchFirm;
extern macInfo_t  gBchMacs[CHANS_CNT];
extern testInfo_t gBchTest[CHANS_CNT];

extern batch_t    batch;
extern burner_t   bchns[CHANS_CNT];

void batchInit(void);

void batchSet(uint8_t state, uint8_t details);

#define batchOK()          batchSet(PSTA_OK,    PERR_NO_ERROR)
#define batchFail(error)   batchSet(PSTA_FAIL,  error)
#define batchError(error)  batchSet(PSTA_ERROR, error)
#define batchBusy(opCode)  batchSet(PSTA_BUSY,  opCode)

#define BATCH_IS_BUSY()    (batch.state & PSTA_BUSY)
#define BATCH_IS_IDLE()    !(batch.state & (PSTA_BUSY | PSTA_ERROR))
#define BATCH_IS_ONLINE()  (batch.state & PSTA_ONLINE)

#endif // _BURNER_H_
