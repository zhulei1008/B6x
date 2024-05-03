#include <string.h>
#include "batch.h"
#include "proto.h"
#include "chans.h"
#include "prg_api.h"
#include "display.h"
#include "flash.h"
#include "chanfirm.h"

#include "b6x.h"             //test --whl
static void xdelay(uint32_t x)
{
    while (x--)
    {
        __NOP();
        __NOP();
    }
}

/*SYNC-OpCode{{*/
void chanSyncStart(void)
{
    if (batch.gChans)
    {
        //batch.chsSet = batch.gChans;
        batch.chsErr = 0;
        batchBusy(OP_SYNC_INFO);

        batch.chsIdx = __builtin_ctz(batch.chsSet);
        chansSelect(batch.chsIdx);
        pt_cmd_fwcfg();
    }
}

void chanSyncNext(uint8_t status)
{
    if (batch.opCode != OP_SYNC_INFO)
        return;

    if (status) batch.chsErr |= (1 << batch.chsIdx);

    if ((batch.gChans & (1 << batch.chsIdx)) == 0)
    {        
        pt_cmd_action(ACT_OFF);        
    }

    // turn to next chan
    while (++batch.chsIdx <= CHANS_CNT) //"<" will make chansSel == 0; when batch.gChans = 0x7F
    {
        if (batch.chsSet & (1 << batch.chsIdx))
        {
            chansSelect(batch.chsIdx);
            pt_cmd_fwcfg();
            return;
        }
    }

    if (batch.chsErr)
        batchError(PERR_CHAN_SET);
    else
        batchOK();
}
/*}}*/


/*BURN-OpCode{{*/
enum burn_step
{
    STEP_NONE,
    STEP_ERASE,
    STEP_CODE,
    STEP_DATA,
    STEP_INFO,
};

static uint8_t burn_step;
static uint16_t burn_pgidx;

static __inline void firm_goto(uint8_t step)
{
    burn_pgidx = 0;
    burn_step = step;
}

static void firm_erase(void)
{
    pt_rsp_chans(0x01 << batch.chsIdx, CHAN_START);
    chansSelect(batch.chsIdx);
    firm_goto(STEP_ERASE);

    if (batch.opCode == OP_BURN_CHAN)
    {
        pt_cmd_er_firm(FLASH_CHIP, 0, 0);
    }
    else // OP_BURN_FIRM
    {
        struct er_map map;
        uint16_t erase_page = ADDR2PAGE_H(gBchFirm.codeLen) + ADDR2PAGE_H(gBchFirm.dataLen) + 2;

        map.eridx = 1;
        map.ercnt = (uint16_t)(((erase_page + 127) >> 7) & 0xFFFF);
        pt_cmd_er_firm(FLASH_BLOCK32, 1, &map);
    }
}

static bool firm_burn0(void)
{
    // burner Updata: 0x0000(info) 0x1000~0x5000(code:burner.bin) 0x5000~0x8000(data:chipset.bin)
    uint16_t step_pages;
    uint16_t dest_pgidx;
    uint8_t *src_pgptr;

    if (burn_step == STEP_CODE)
    {
        step_pages = ADDR2PAGE_H(gBchFirm.codeLen);
        if (burn_pgidx < step_pages)
        {
            dest_pgidx = ADDR2PAGE(FSH_ADDR_CODE_BASE) + burn_pgidx;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_BASE + burn_pgidx * FSH_PAGE_SIZE);

            pt_cmd_wr_firm(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }

        // step over to next
        firm_goto(STEP_DATA);
    }

    if (burn_step == STEP_DATA)
    {
        step_pages = ADDR2PAGE_H(gBchFirm.dataLen);
        if (burn_pgidx < step_pages)
        {
            dest_pgidx = ADDR2PAGE(/*FSH_ADDR_SDRV_BASE*/gBchFirm.dataAddr) + burn_pgidx;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_BASE + (burn_pgidx + ADDR2PAGE_H(gBchFirm.codeLen)) * FSH_PAGE_SIZE);

            pt_cmd_wr_firm(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }

        // step over to next
        firm_goto(STEP_INFO);
    }

    if (burn_step == STEP_INFO)
    {
        if (burn_pgidx < 1/*InfoPage*/)
        {
            dest_pgidx = ADDR2PAGE(FSH_ADDR_CODE_INFO); // 0;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_INFO);

            pt_cmd_wr_firm(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }
    }

    // step over to finish
    firm_goto(STEP_NONE);
    return false;
}

static uint32_t macc_start_getset(uint8_t cidx)
{
    uint32_t mac_start = gBchFirm.macCFG.nstart;
    
    if (gBchFirm.macCFG.ncount != 0xFFFF)
    {
        mac_start += ((gBchFirm.macCFG.ncount * gBchFirm.macCFG.ndelta) * cidx);
    }
    
    return mac_start;
}     
static bool firm_burn1(void)
{
    // chipset Firmware: 0x8000(info) 0x8200~0x40000(code+data)
    uint16_t step_pages;
    uint16_t dest_pgidx;
    uint8_t *src_pgptr;

    if (burn_step == STEP_CODE)
    {
        step_pages = ADDR2PAGE_H(gBchFirm.codeLen) + ADDR2PAGE_H(gBchFirm.dataLen);
        if (burn_pgidx < step_pages)
        {
            dest_pgidx = ADDR2PAGE(FSH_ADDR_FIRM_BASE) + burn_pgidx;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_BASE + burn_pgidx * FSH_PAGE_SIZE);

            pt_cmd_wr_firm(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }

        // step over to next
        firm_goto(STEP_INFO);
    }

    if (burn_step == STEP_INFO)
    {
        if (burn_pgidx < 1/*InfoPage*/)
        {
            dest_pgidx = ADDR2PAGE(FSH_ADDR_FIRM_INFO);

            if (!(gBchFirm.firmCFG.bConf & FWCFG_EN(BURN_MAC)))
            {
                src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_INFO);
            }
            else
            {
                uint8_t firm_info[FSH_PAGE_SIZE];
                firmInfo_t *param = (firmInfo_t *)firm_info;

                // update macStart
                memcpy(firm_info, (uint8_t *)FSH_ADDR_FIRM_INFO, FSH_PAGE_SIZE);
                param->macCFG.nstart = macc_start_getset(batch.chsIdx);//MACC_START_GETSET(batch.chsIdx);//0;

                src_pgptr = firm_info;
            }

            pt_cmd_wr_firm(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }
    }

    // step over to finish
    firm_goto(STEP_NONE);
    return false;
}

void chanBurnStart(uint8_t chans, uint8_t mode)
{
    if (chans == 0) return;

    batch.gChans = chans;
    batch.chsSet = chans;
    batch.chsErr = 0;
    batchBusy(mode == CHNS_UPDATE ? OP_BURN_CHAN : OP_BURN_FIRM);

    batch.chsIdx = __builtin_ctz(batch.chsSet);
    firm_erase();
}

void chanBurnNext(uint8_t status)
{
    if ((batch.opCode != OP_BURN_FIRM) && (batch.opCode != OP_BURN_CHAN))
        return;

    if (status)
    {
        batch.chsErr |= (1 << batch.chsIdx); 
        batch.chsSet &= (1 << batch.chsIdx);        
    }
    
    batch.chsSet &= ~(1 << batch.chsIdx); //211116 --whl
     
    pt_rsp_chans(1 << batch.chsIdx, status + 2);

    // turn to next chan
    while (++batch.chsIdx <= CHANS_CNT)
    {
        if (batch.chsSet & (1 << batch.chsIdx))
        {
            dispDone();  //211116 --whl
            firm_erase();
            return;
        }
    }

    // complete to finish
    firm_goto(STEP_NONE);

    if (batch.chsErr)
        batchFail(PERR_BURN_FAIL);
    else
        batchOK();
}

void chanBurnCont(uint8_t rsp, uint8_t status)
{
    if ((batch.opCode != OP_BURN_FIRM) && (batch.opCode != OP_BURN_CHAN))
        return;

    if (rsp == PT_RSP_ER_FIRM)
    {
        if (burn_step != STEP_ERASE) return;

        firm_goto(STEP_CODE);
    }
    else // (rsp == PT_RSP_WR_FIRM)
    {
        if (burn_step < STEP_CODE) return;
    }

    if (status == PT_OK)
    {
        bool more = (batch.opCode == OP_BURN_FIRM) ? firm_burn1() : firm_burn0();

        if (more) return;
    }

    // error or finish to next
    chanBurnNext(status);
}
/*}}*/


/*RUN-OpCode{{*/
//#include "uart_itf.h"
//extern uint8_t chansSel;

void chanRunStart(uint8_t mode)
{
    uint8_t chans = mode ? batch.chsErr : batch.gChans;

    if (chans)
    {
        batch.chsSet = chans;
        batch.chsErr = 0;
        batchBusy(OP_RUN_START);

        batch.chsIdx = __builtin_ctz(batch.chsSet);
        
//        uart2_send((uint8_t *)&batch.chsIdx, 1); //test
//        uart2_send((uint8_t *)&chansSel, 1); //test
        
        chansSelect(batch.chsIdx);        
        pt_cmd_action(ACT_RUN);
    }
}

void chanRunNext(uint8_t status)
{
    if (batch.opCode != OP_RUN_START)
        return;

    if (status)
    {
        batch.chsErr |= (1 << batch.chsIdx);
        batch.chsSet &= (1 << batch.chsIdx);
    }

    // turn to next chan
    while (++batch.chsIdx <= CHANS_CNT)
    {
        if (batch.chsSet & (1 << batch.chsIdx))
        {
            chansSelect(batch.chsIdx);
            pt_cmd_action(ACT_RUN);
            return;
        }
    }

    // complete to next step or all fail
    if (batch.chsSet)
    {
        batch.opCode = OP_RUN_STATE;

        batch.chsIdx = __builtin_ctz(batch.chsSet);
        chansSelect(batch.chsIdx);
        pt_cmd_chsta(batch.chsIdx);
    }
    else
    {
        batchFail(PERR_RUN_FAIL);
    }
}

void chanRunState(uint8_t state)
{
    if (batch.opCode != OP_RUN_STATE)
        return;

    if (state)
    {
        if (state & PSTA_FAIL)
            batch.chsErr |= (1 << batch.chsIdx);

        batch.chsSet &= ~(1 << batch.chsIdx);
        dispDone();
    }
    else
    {
        if (batch.chsSet)
        {
            for (uint8_t i = 0; i < CHANS_CNT; i++)
            {
                if (++batch.chsIdx == CHANS_CNT)
                    batch.chsIdx = 0;

                if (batch.chsSet & (1 << batch.chsIdx))
                {
                    chansSelect(batch.chsIdx);
                    pt_cmd_chsta(batch.chsIdx);
                    return;
                }
            }
        }
        else
        {
            batchOK();
            
//            xdelay(1000*200);     //test --whl                        
//            if (BATCH_IS_IDLE() && batch.chsErr == 0)
//            {
//                chanRunStart(0);
//            }
        }
    }
}
/*}}*/
