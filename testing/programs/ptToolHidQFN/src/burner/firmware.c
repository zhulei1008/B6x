#include <string.h>
#include "burner.h"
#include "proto.h"
#include "prg_api.h"
#include "flash.h"

enum
{
    BURN_NONE,
    BURN_CODE,
    BURN_DATA,
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
            uint16_t chip_code_addr = ADDR2PAGE(gFirmInfo.codeAddr);
            pt_cmd_wr_flash(chip_code_addr + firm_pgidx, (uint8_t *)(firm_code_base + firm_pgidx * FSH_PAGE_SIZE), FSH_PAGE_SIZE);
        } break;

        case BURN_DATA:
        {
            uint16_t pages = gFirmInfo.codeLen / FSH_PAGE_SIZE;
            if (gFirmInfo.codeLen % FSH_PAGE_SIZE)
            {
                pages += 1;
            }
            uint32_t firm_data_base = FSH_ADDR_FIRM_BASE + (pages * FSH_PAGE_SIZE); // FSH_ADDR_FIRM_BASE
            uint16_t chip_data_addr = ADDR2PAGE(gFirmInfo.dataAddr);
            pt_cmd_wr_flash(chip_data_addr + firm_pgidx, (uint8_t *)(firm_data_base + firm_pgidx * FSH_PAGE_SIZE), FSH_PAGE_SIZE);

        } break;

        case BURN_INFO:
        {
            uint32_t firm_info = FSH_ADDR_FIRM_INFO; // FSH_ADDR_FIRM_INFO
            uint16_t chip_info_pgidx = 0;
            pt_cmd_wr_flash(chip_info_pgidx, (uint8_t *)(firm_info), FSH_PAGE_SIZE);
        } break;

        default:
            break;
    }

}

static void firmState(uint32_t info_len, uint8_t firm_state)
{
    uint16_t length = FSH_PAGE_SIZE;

    uint16_t pages  = info_len / FSH_PAGE_SIZE;

    if (firm_pgidx == pages)
    {
        length = gFirmInfo.codeLen % FSH_PAGE_SIZE; // last load
    }

    if ((firm_pgidx > pages) || (length == 0))
    {
        firmStateChange(firm_state);

        if ((firm_step == BURN_DATA) && (gFirmInfo.dataLen == 0))
        {
            firmStateChange(BURN_INFO);
        }
    }

    firm_burn();
}

void firm_start(void)
{
    uint8_t emode = FLASH_CHIP;
    uint8_t icnt  = 0;
    uint32_t macEnd = MACC_START_GETSET(1); // start + cnt*incre
    if ((((gFirmInfo.macOffset >> 24) & 0xFF) == 0x18) && ((gmacInfo.macCurrent >= macEnd)))
    {
        setBurnerState(BURNER_FAIL, ER4_MAC_END);

        return;
    }

    pt_cmd_er_flash(emode, icnt, 0);
}

void firm_rsp_er_flash(uint8_t status)
{
    if (status == PT_OK)
    {
        firmStateChange(BURN_CODE);
        firm_burn();
    }
}

void firm_rsp_wr_flash(uint8_t status)
{
    if (status != PT_OK)
    {
        // fail proc
        setBurnerState(BURNER_FAIL, ER1_RSP);
        return;
    }

    firm_pgidx++;

    switch (firm_step)
    {
        case BURN_CODE:
        {
            firmState(gFirmInfo.codeLen, BURN_DATA);
        } break;

        case BURN_DATA:
        {
            firmState(gFirmInfo.dataLen, BURN_INFO);
        } break;

        case BURN_INFO:
        {
            firmStateChange(BURN_NONE);

            if (((gFirmInfo.macOffset >> 24) & 0xFF) == 0x18)
            {
                pt_cmd_wr_mac(gFirmInfo.macOffset, gmacInfo.macCurrent);
            }
            else
            {
                //                pt_cmd_rst();
                if (gFirmInfo.testCfg)
                {
                    burner.opCode = OP_TEST;
                    burner.stepBit = 0;
                    memset(&test_result, 0, sizeof(test_result_t));
                }

                test_start();
                //                setBurnerState(BURNER_OK, ER_NULL);
            }
        } break;

        default:
            break;
    }
}

void firm_rsp_wr_mac(uint8_t status)
{
    gmacInfo.macCurrent += (gFirmInfo.macConfigure & 0xFF);

    if ((status == PT_OK) && prg_flashPageChange(ADDR2PAGE(FSH_ADDR_MAC_INFO), sizeof(macInfo_t) - 4, 4, (uint8_t *)&gmacInfo.macCurrent))
    {
        //        pt_cmd_rst();
        if (gFirmInfo.testCfg)
        {
            burner.opCode = OP_TEST;
            burner.stepBit = 0;
            memset(&test_result, 0, sizeof(test_result_t));
        }

        test_start();
        //        setBurnerState(BURNER_OK, ER_NULL);
    }
    else
    {
        setBurnerState(BURNER_FAIL, ER1_RSP);
    }
}


void firm_rsp_vf_flash(uint8_t status)
{

}

