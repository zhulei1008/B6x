#include <string.h>
#include "batch.h"
#include "proto.h"
#include "prg_api.h"
#include "chans.h"
#include "oled.h"
#include "flash.h"

//Version
burner_Info_t burner_Info[8];

uint8_t getBurnerFirmResult(uint8_t chan_i)
{
    if (!(burner_Info[chan_i].burner.state & BURNER_ON_LINE))
    {
        return VERSION_ER_SYNC;
    }

    if (batch.opCode != UP_BURNER_DATE)
    {
        if (burner_Info[chan_i].codeCRC != gFirmInfo.codeCRC)
        {
            return VERSION_ER0_CODECRC;
        }

        if (burner_Info[chan_i].macStart != MACC_START_GETSET(chan_i))
        {
            return VERSION_ER2_MAC_START;
        }

        if (burner_Info[chan_i].macConfigure != gFirmInfo.macConfigure)
        {
            return VERSION_ER3_MAC_CONFIG;
        }
    }

    return VERSION_OK;
}

void get_firm_crc(uint8_t idx)
{
    if (batch.opCode != DOWN_CHIP_STATE)
    {
        burner_Info[idx].codeCRC = 0;

        pt_cmd_firm_crc();
    }
}


enum
{
    BURN_NONE,
    BURN_CODE,
    BURN_DATA,
    //    BURN_MACC,
    BURN_INFO,
};

uint8_t firm_step;
uint16_t firm_pgidx;

static void firmStateChange(uint8_t firm_state)
{
    firm_pgidx = 0;
    firm_step = firm_state;
}

void firm_burn(void)
{
    switch (firm_step)
    {
        case BURN_CODE:
        {

            uint32_t firm_code_base = FSH_ADDR_FIRM_BASE; // FSH_ADDR_FIRM_BASE
            uint16_t firm_code_addr = ADDR2PAGE(FSH_ADDR_FIRM_BASE);

            if (batch.opCode == UP_BURNER_DATE)
            {
                firm_code_addr = ADDR2PAGE(FSH_ADDR_CODE_BASE);
            }

            pt_cmd_wr_firm(firm_code_addr + firm_pgidx, (uint8_t *)(firm_code_base + firm_pgidx * FSH_PAGE_SIZE), FSH_PAGE_SIZE);
        } break;

        case BURN_DATA:
        {
            uint32_t firm_data_base = FSH_ADDR_FIRM_BASE; // FSH_ADDR_FIRM_BASE
            uint16_t firm_data_addr = ADDR2PAGE(FSH_ADDR_SDRV_BASE);
            pt_cmd_wr_firm(firm_data_addr + firm_pgidx, (uint8_t *)(firm_data_base + (firm_pgidx + ADDR2PAGE_H(gFirmInfo.codeLen))*FSH_PAGE_SIZE), FSH_PAGE_SIZE);

            //          uint16_t pages = gFirmInfo.codeLen / FSH_PAGE_SIZE;
            //          if (gFirmInfo.codeLen % FSH_PAGE_SIZE)
            //          {
            //              pages += 1;
            //          }
            //          uint32_t firm_data_base = FSH_ADDR_FIRM_BASE + pages*FSH_PAGE_SIZE; // FSH_ADDR_FIRM_BASE
            //          uint16_t firm_data_addr = ADDR2PAGE(gFirmInfo.dataAddr);
            //          pt_cmd_wr_firm(firm_data_addr + firm_pgidx, (uint8_t *)(firm_data_base + firm_pgidx*FSH_PAGE_SIZE), FSH_PAGE_SIZE);
        } break;

        case BURN_INFO:
        {
            uint16_t firm_info_pgidx = ADDR2PAGE(FSH_ADDR_FIRM_INFO);

            if (batch.opCode == UP_BURNER_DATE)
            {
                firm_info_pgidx = ADDR2PAGE(FSH_ADDR_CODE_INFO);
            }

            if (((gFirmInfo.macOffset >> 24) & 0xFF) == 0x18)
            {
                uint8_t firm_info[FSH_PAGE_SIZE] = {0}; // FSH_ADDR_FIRM_INFO
                memcpy(firm_info, (uint8_t *)FSH_ADDR_FIRM_INFO, FSH_PAGE_SIZE);

                firmInfo_t *parm = (firmInfo_t *)firm_info;

                parm->macStart = MACC_START_GETSET(chansSel);

                pt_cmd_wr_firm(firm_info_pgidx, firm_info, FSH_PAGE_SIZE);
            }
            else
            {
                pt_cmd_wr_firm(firm_info_pgidx, (uint8_t *)(FSH_ADDR_FIRM_INFO), FSH_PAGE_SIZE);
            }
        } break;

        default:
            break;
    }

}

static void firmState(uint16_t page_num, uint8_t firm_state)
{
    if (firm_pgidx >= page_num)
    {
        firmStateChange(firm_state);
    }

    firm_burn();
}

void firm_start(void)
{
    if (batch.opCode == DOWN_BURNER_FIRM || batch.opCode == UP_BURNER_DATE)
    {
        for (uint8_t i = 0; i < CHANS_CNT; i++)
        {
            if ((batch.chsSet >> i) & 0x01)
            {
                pt_rsp_chan_set(0x01 << i, VERSION_START);

                batch.chsSet &= (~(0x01 << i));

                if (burner_Info[i].burner.state & BURNER_ON_LINE)
                {
                    if (batch.state & BATCH_ERR_FIRM)
                    {
                        pt_rsp_chan_set(0x01 << i, VERSION_ER0_CODECRC);
                    }
                    else
                    {
                        //                        if (getBurnerFirmResult(i))
                        //                        {
                        //                            pt_rsp_chan_set(0x01 << i, VERSION_OK);
                        //                        }
                        //                        else
                        //                        {
                        chansSelect(i);

                        if (batch.opCode == UP_BURNER_DATE)
                        {
                            //UP ChipSet
                            pt_cmd_er_firm(FLASH_CHIP, 0, 0);
                        }
                        else
                        {
                            struct erinfo_firm erase_info;
                            erase_info.pgidx = 1; // /1000;

                            uint16_t erase_page = ADDR2PAGE_H(gFirmInfo.codeLen) + ADDR2PAGE_H(gFirmInfo.dataLen) + 2;

                            erase_info.pgcnt = (uint16_t)(((erase_page + 127) >> 7) & 0xFFFF);

                            pt_cmd_er_firm(FLASH_BLOCK32, 1, /*(uint8_t *)*/&erase_info);
                        }

                        return;
                        //                        }
                    }
                }
                else
                {
                    pt_rsp_chan_set(0x01 << i, VERSION_ER_SYNC);
                }

            }
        }

        firmStateChange(BURN_NONE);

        setBatchState(BATCH_OK);
        //        batch.state &= (~ (BATCH_BUSY | BATCH_BTN_BURNER_FIRM));
        //        get_burner_crc_bit = batch_gChanConect;
        //        get_burner_macc_bit = batch_gChanConect;
        //        batch.opCode = READ_CRC;
        //        batch.chsSet = batch_gChanConect;
    }
}

void firm_rsp_er(uint8_t status)
{
    if (status == PT_OK)
    {
        firmStateChange(BURN_CODE);
        firm_burn();
    }
    else
    {
        setBatchState(BATCH_FAIL);
    }
}

void firm_rsp_wr(uint8_t status)
{
    if (status != PT_OK)
    {
        // fail proc
        setBatchState(BATCH_FAIL);
        return;
    }

    firm_pgidx++;

    switch (firm_step)
    {
        case BURN_CODE:
        {
            if (batch.opCode == UP_BURNER_DATE)
            {
                firmState((ADDR2PAGE_H(gFirmInfo.codeLen)), BURN_DATA);

            }
            else
            {
                firmState((ADDR2PAGE_H(gFirmInfo.codeLen) + ADDR2PAGE_H(gFirmInfo.dataLen)), BURN_INFO);
            }
        } break;

        case BURN_DATA:
        {
            firmState(ADDR2PAGE_H(gFirmInfo.dataLen), BURN_INFO);
        } break;

        case BURN_INFO:
        {
            if (batch.opCode == UP_BURNER_DATE)
            {
                pt_rsp_chan_set(0x01 << chansSel, VERSION_OK);

                oledDonePage();
                firm_start();
            }
            else
            {
                get_firm_crc(chansSel);
            }
            //            pt_rsp_chan_set(0x01 << chansSel, VERSION_OK);
            //            firm_start();
        } break;

        default:
            break;
    }
}

void firm_result_done(void)
{
    if (batch.opCode == DOWN_BURNER_FIRM)
    {
        pt_rsp_chan_set(0x01 << chansSel, getBurnerFirmResult(chansSel));

        oledDonePage();
        firm_start();
    }
    else
    {
        if ((gmacInfo.chan >> chansSel) & 0x01)
        {
            if (getBurnerFirmResult(chansSel) != VERSION_OK)
            {
                setBatchState(BATCH_ERR_CHAN);
            }
        }
        else
        {
            pt_cmd_chan_set(0, 0);
        }
    }
}

void firm_rsp_vf(uint8_t status)
{

}

