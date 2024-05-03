#include <string.h>
#include "b6x.h"
#include "proto.h"
#include "prg_api.h"
#include "rf_test.h"
#include "led.h"
#include "sftmr.h"
#include "btn.h"
#include "gpio.h"
#include "batch.h"
#include "reg_uart.h"
#include "uart_itf.h"
#include "reg_gpio.h"
#include "beep.h"
#include "display.h"
#include "chanfirm.h"

firmInfo_t gBchFirm;
macInfo_t  gBchMacs[CHANS_CNT];
testInfo_t gBchTest[CHANS_CNT];

batch_t    batch;
burner_t   bchns[CHANS_CNT];

static uint8_t infoReset(void)
{
    // Retrieve
    memcpy(&gBchFirm, (uint32_t *)FSH_ADDR_FIRM_INFO, sizeof(gBchFirm));
    memcpy(&gBchMacs, (uint32_t *)FSH_ADDR_MAC_INFO,  sizeof(gBchMacs));

    // Zero Clean
    memset(&gBchTest, 0, sizeof(gBchTest));
    memset(&bchns,    0, sizeof(bchns));

    memset(&batch,    0, sizeof(batch));
    batch.gChans = gBchFirm.firmCFG.bChns;
    batch.chsSet = chansDetect();

    // Valid Check
    if (IS_NONE(gBchFirm.firmCFG.Word))
    {
        batchError(PERR_FIRM_CFG);
        
        return PERR_FIRM_CFG;
    }

    if ((batch.gChans == 0) || ((batch.chsSet & batch.gChans) != batch.gChans))
    {
        batchError(PERR_CHAN_SET);
        
        return PERR_CHAN_SET;
    }

    return PERR_NO_ERROR;
}

/// Host Cmd Parser
static void host_parser(struct pt_pkt *pkt, uint8_t status)
{
    //    batch.state |= BATCH_PC_CONNECTED;
    if (status != PT_OK)
    {
        pt_rsp_status(pkt->code, status);
        return;
    }
    
    switch (pkt->code)
    {
#if defined(CFG_CARRY)
        // Boot: carry to burner
        case PT_CMD_SRD:
        case PT_CMD_BAUD:
        {
            pt_send_cmd(pkt);
        } break;

        case PT_CMD_RST:
        {
            pt_rsp_status(PT_RSP_RST, PT_OK);
            WAIT_UART_IDLE(1);

            NVIC_SystemReset();
        } break;

        // Flash: carry to burner
        case PT_CMD_ER_FLASH:
        case PT_CMD_WR_FLASH:
        case PT_CMD_RD_FLASH:
        case PT_CMD_VF_FLASH:
        {
            pt_send_cmd(pkt);
        } break;

        // Test: carry to burner
        case PT_CMD_TEST_GPIO:
        case PT_CMD_TEST_XTAL:
        case PT_CMD_TEST_RF:
        case PT_CMD_TEST_PWR:
        {
            pt_send_cmd(pkt);
        } break;
#endif

        /* Proto: Burn & Test */
        case PT_CMD_SYNC:
        {
            batchSet(PSTA_ONLINE, true);

            pt_rsp_sync(SYNC_OK_BATCH);
        } break;

        case PT_CMD_VERSION:
        {
            pt_rsp_version(VER_BATCH);
        } break;

        case PT_CMD_ER_FIRM:
        {
            PKT_PARAM(struct pt_cmd_er_fsh);

            prg_flash_erase(param->mode, param->mcnt, &param->maps[0]);
            pt_rsp_status(PT_RSP_ER_FIRM, PT_OK);
        } break;

        case PT_CMD_WR_FIRM:
        {
            PKT_PARAM(struct pt_cmd_wr_fsh);
            // Recevied, prepare NEXT
            pt_rsp_status(PT_RSP_WR_FIRM, PT_OK);
            // Write and Vertify
            if (!prg_flash_write(param->pgidx, pkt->len - 3, param->data))
            {
                pt_rsp_status(PT_RSP_WR_FIRM, PT_ERR_VERIFY);
            }
        } break;

#if (CFG_READ)
        case PT_CMD_RD_FIRM:
        {
            PKT_PARAM(struct pt_cmd_rd_fsh);

            pt_rsp_rd_firm(param->pgidx, param->length, prg_flash_paddr(param->pgidx));
        } break;

        case PT_CMD_VF_FIRM:
        {
            PKT_PARAM(struct pt_cmd_vf_fsh);
            if (prg_flash_verify(param->pgidx, pkt->len - 3, param->data))
            {
                pt_rsp_status(PT_RSP_VF_FIRM, PT_OK);
            }
            else
            {
                pt_rsp_status(PT_RSP_VF_FIRM, PT_ERR_VERIFY);
            }
        } break;

        case PT_CMD_MF_FIRM:
        {

        } break;
#endif

        /* Proto: Batch*/
        case PT_CMD_CHANS:
        {
            PKT_PARAM(struct pt_cmd_chans);

            if (param->chns > 0)
            {
                infoReset();
                
                batchSet(PSTA_ONLINE, true);
                
                chanBurnStart(param->chns, param->mode);
            }
            else
            {
                pt_rsp_chans(chansDetect(), CHAN_FOUND);
            }
        } break;

        case PT_CMD_CHSTA:
        {
            PKT_PARAM(struct pt_cmd_chsta);

            chansSelect(param->chan);

            if (batch.gChans & (0x01 << param->chan))
                pt_rsp_chsta(param->chan, &bchns[param->chan].state);
            else
                pt_rsp_status(PT_RSP_CHSTA, PT_ERR_STATUS);
        } break;

        case PT_CMD_CHRET:
        {

        } break;

        default:
            break;
    }
}

static void chan_parser(struct pt_pkt *pkt, uint8_t status)
{
    pt_code = 0;

    if (status != PT_OK)
    {
        uart1_send(&status, 1);
        uart1_send((uint8_t *)pkt, PKT_HDR_SIZE);
    }

    switch (pkt->code)
    {
        case PT_RSP_SYNC:
        {

        } break;

        case PT_RSP_VERSION:
        {

        } break;

        case PT_RSP_FWCFG:
        {
            PKT_PARAM(struct pt_rsp_fwcfg);

            chanSyncNext(PT_OK);
        } break;

        case PT_RSP_CHRET:
        {
            PKT_PARAM(struct pt_rsp_chret);
            memcpy(&(gBchMacs[param->chan]), param->pmac, sizeof(macInfo_t));
            memcpy(&(gBchTest[param->chan]), param->ptst, sizeof(testInfo_t)); 
        } break;

        case PT_RSP_ER_FIRM:
        case PT_RSP_WR_FIRM:
        {
            chanBurnCont(pkt->code, pkt->payl[0]);
        } break;

        case PT_RSP_ACTION:
        {
//            chanSyncNext(PT_OK); //ACT_OFF
            chanRunNext(PT_OK);
        } break;

        case PT_RSP_CHSTA:
        {
            PKT_PARAM(struct pt_rsp_chsta);
            memcpy(&bchns[param->chan].state, param->psta, 4);

            if (!(param->psta[0] & PSTA_BUSY))
            { 
                pt_cmd_chret(param->chan);
                
                chanRunState(param->psta[0] & (PSTA_OK | PSTA_FAIL));                
                //get_burner_macc(param->chan);
            }
        } break;

        default:
            break;
    }
}

static void key_event(uint8_t id, uint8_t evt)
{
    if (BATCH_IS_ONLINE())
    {
        return;
    }

    switch (evt)
    {
        case BTN_CLICK:
        {
            if (BATCH_IS_IDLE())
            {
                if (id == 0/*KEY_START*/)
                {
                    chanRunStart(0);
                }
                else /*KEY_OLED*/
                {
                    dispDetail();
                }
            }
            
//            beepTone(TONE_BTN);
        } break;

        case BTN_DOUBLE:
        {
            //chansReset();
            NVIC_SystemReset();
        } break;

        case BTN_LONG:
        {
            if (BATCH_IS_IDLE())
            {
                if (id == 0/*KEY_START*/)
                {
                    chanRunStart(1);
                }

                beepTone(TONE_BTN);
            }
        } break;

        default:
            break;
    }
}

#define TK_TIME  _MS(100)

static uint32_t batch_timer(uint8_t id)
{
    if ((pt_code) && ((uint32_t)(currTickCnt() - pt_time) > 3))
    {
        // cmd timeout
        pt_code = 0;
        bchns[batch.chsIdx].state = 0;

        chanRunNext(PT_ERR_TIMEOUT);
    }

    chanRunState(0);

    return TK_TIME;
}

void batchInit(void)
{
    //beepInit(); replace in bondPadInit
    chansInit();
    
    // Batch Task
    proto_init(PT_HOST, host_parser);
    proto_init(PT_CHAN, chan_parser);
    sfTmrStart(TK_TIME, batch_timer);

    // UI operation
    btnInit(key_event);
    ledInit(LED_READY);
    dispInit();
    beepTone(TONE_PWR);

    // Retrieve & Start
    if (infoReset() == PERR_NO_ERROR)
    {
        chanSyncStart();
    }
}

void batchSet(uint8_t state, uint8_t detail)
{
    if (state > PSTA_ERROR)
    {
        // Set|Clear Bit
        if (detail)
            batch.state |= state;
        else
            batch.state &= ~state;
        return;
    }
    
    if ((batch.state & state) == state)
    {
        return; // repeat state
    }

    batch.state |= state;

    switch (state)
    {
        case PSTA_BUSY:
        {
            batch.state &= ~(PSTA_OK | PSTA_FAIL);
            batch.opCode = detail;

            ledPlay(LED_BUSY);
            dispBusy();
        } break;

        case PSTA_OK:
        case PSTA_FAIL:
        {
            dispDone();
            
            batch.state &= ~PSTA_BUSY;
            batch.opCode = OP_NULL;
            batch.error  = detail;

            if (state == PSTA_OK)
                ledPlay(LED_READY);
            else
                ledPlay(LED_FAIL);           
        } break;

        case PSTA_ERROR:
        {
            batch.error  = detail;
            ledPlay(LED_ERROR);
        } break;

        default:
            break;
    }
}
