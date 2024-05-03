#include <string.h>
#include "batch.h"
#include "proto.h"
#include "chans.h"
#include "prg_api.h"
#include "oled.h"

//test_result_t test_result[8];

void chip_firms_start(void)
{
    if (batch.opCode == DOWN_CHIP_CMD)
    {
        for (uint8_t i = 0; i < CHANS_CNT; i++)
        {
            if ((batch.chsSet >> i) & 0x01)
            {
                batch.chsSet &= (~(0x01 << i));

                chansSelect(i);
                pt_cmd_dw_firm();
                return;
            }
        }

        batch.opCode = DOWN_CHIP_STATE;
        batch.chsSet = batch_gChanConect;

        get_burner_state();
    }
}

uint8_t getChipFirmResult(uint8_t chan_i)
{
    if (burner_Info[chan_i].burner.state & BURNER_OK)
    {
        return ER_NULL;
    }

    if (!((batch_gChanConect >> chan_i) & 0x01) || !(burner_Info[chan_i].burner.state & BURNER_ON_LINE))
    {
        return ER_XX;
    }

    if (burner_Info[chan_i].burner.state & BURNER_ERR_FIRM)
    {
        return ER0_NO_CODE;
    }

    if (burner_Info[chan_i].burner.state & FAIL_MASK)
    {
        return (burner_Info[chan_i].burner.state & FAIL_MASK) >> FAIL_BIT;
    }

    return ER9_UNKNOWN;
}

uint32_t getCountNum(void)
{
    uint32_t count_num = 0;

    //    memcpy(&gmacInfo, (uint32_t *)FSH_ADDR_MAC_INFO, sizeof(gmacInfo));

    if (!IS_NONE(gFirmInfo.macConfigure))
    {
        for (uint8_t chan_i = 0; chan_i < 8; chan_i++)
        {
            if (((gmacInfo.chan >> chan_i) & 0x01) && !(batch.state & (BATCH_ERR_FIRM | BATCH_ERR_CHAN)))
            {
                uint32_t macStart = MACC_START_GETSET(chan_i);
                count_num += (gmacInfo.macCurrent[chan_i] -  macStart) / (gFirmInfo.macConfigure & 0xFF);
            }
        }
    }

    return count_num;
}

void get_burner_state(void)
{
    //    if (((batch.state == BATCH_STATE_IDLE) || ((batch.state & BATCH_NO_FIRM) && (!(batch.state & (BATCH_PC_CONNECTED | BATCH_BUSY))))) && (get_burner_crc_bit == 0))
    if ((batch.opCode == GET_BURNER_STATE) || (batch.opCode == DOWN_CHIP_STATE))
    {
        for (uint8_t i = 0; i < CHANS_CNT; i++)
        {
            chansSel++;

            if (chansSel >= CHANS_CNT)
            {
                chansSel = 0;
            }

            if (batch.chsSet & (0x01 << chansSel))
            {
                chansSelect(chansSel);
                pt_cmd_burn_sta(chansSel);
                return;
            }
            //            else
            //            {
            //                burner[i].state = 0;
            //            }
        }

        //        batch.chsSet = batch_gChanConect;

        if (batch.opCode == DOWN_CHIP_STATE)
        {
            //            batch.opCode = READ_MACC;
            //            get_burner_macc();
        }
        else
        {
            batch.opCode = OP_NULL;
        }
    }
}

void get_burner_macc(uint8_t idx)
{
    if (batch.opCode == DOWN_CHIP_STATE)
    {
        burner_Info[idx].macCurrent = 0;

        pt_cmd_rd_macc();
    }
}

void get_burner_macc_change(void)
{
    if (!(batch.state & BATCH_ERR_CHAN) && (burner_Info[chansSel].macCurrent != gmacInfo.macCurrent[chansSel]))
    {
        uint32_t macEnd = MACC_START_GETSET(chansSel + 1);// start + cnt*incre

        if ((burner_Info[chansSel].macCurrent > gmacInfo.macCurrent[chansSel]) && burner_Info[chansSel].macCurrent <= macEnd)
        {
            prg_flashPageChange(ADDR2PAGE(FSH_ADDR_MAC_INFO), 4 * chansSel, 4, (uint8_t *)&burner_Info[chansSel].macCurrent);
            memcpy(&gmacInfo, (uint32_t *)FSH_ADDR_MAC_INFO, sizeof(gmacInfo));
        }
        else
        {
            setBatchState(BATCH_ERR_CHAN);
        }
    }

    oledDonePage();

    if (!batch.chsSet)
    {
        setBatchState(BATCH_OK);
    }
}
