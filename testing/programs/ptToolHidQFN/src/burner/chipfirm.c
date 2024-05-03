#include <string.h>
#include "b6x.h"
#include "sftmr.h"
#include "drvs.h"
#include "regs.h"
#include "uart_itf.h"
#include "burner.h"
#include "proto.h"
#include "prg_api.h"
#include "flash.h"
#include "chipfirm.h"

static uint32_t chip_baud(void)
{
    uint8_t rate = gChnFirm.firmCFG.uRate;

    if ((rate > 1) && (rate <= 8))
        return BOOT_UART_BAUD * rate;
//        return baudArray[rate - 1][0];
    else
        return 0;
}

/*LOAD-OpCode{{*/
enum load_step
{
    LOAD_NONE,
    LOAD_BAUD,
    LOAD_SYNC,
    LOAD_CWR,
    LOAD_JUMP,
    LOAD_WAIT,
};

static uint8_t load_step;
static uint8_t load_pgidx;

#define load_goto(step)  load_step = step
//static __inline void load_goto(uint8_t step)
//{
//    load_pgidx = 0;
//    load_step = step;
//}

static bool enter_boot(void)
{
    if (gChnFirm.firmCFG.bConf & BIT(FW_RESET_RUN_POS))
    {
        GPIO->DAT_CLR = (uint32_t)(BIT(PIN_VCC_CHIP) | BIT(PIN_PWM_CHIP));
        GPIO->DIR_SET = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_PWM_CHIP) | BIT(PIN_RST));  //FW_RESET_RUN_POS        
        bootDelayMs(15);
    }

    
    GPIO->DAT_SET = (uint32_t)(BIT(PIN_VCC_CHIP));
    bootDelayMs(15);
    GPIO->DIR_CLR = (uint32_t)(BIT(PIN_RST));  //short


    bootDelayMs(15);
    GPIO->DIR_CLR = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2));
    
    return uart2_init(NULL);
}

static bool load_sdrv(void)
{
    if (load_step == LOAD_BAUD)
    {
        load_goto(LOAD_SYNC);
        pt_cmd_sync();
        return true;
    }

    if (load_step == LOAD_SYNC)
    {
        uint32_t baudrate = chip_baud();

        load_goto(LOAD_CWR);
        load_pgidx = 0;

        if (baudrate)
        {
            // modify chipSet uart1 baudrate
            uint8_t firm_vect[RAM_PAGE_SIZE];

            memcpy(firm_vect, (uint8_t *)(gChnSdrv.sdrvAddr), RAM_PAGE_SIZE);
            memcpy(firm_vect + 0xC0, &baudrate, 4); //0x200036C0

            pt_cmd_cwr(CHIP_SDRV_ADDR, firm_vect, RAM_PAGE_SIZE);
            load_pgidx++;
            return true;
        }
    }

    if (load_step == LOAD_CWR)
    {
        uint8_t sdrv_pages = ADDR2PAGE_H(gChnSdrv.sdrvLen);

        if (load_pgidx < sdrv_pages)
        {
            // load sdrv
            uint16_t offset = load_pgidx * RAM_PAGE_SIZE;

            pt_cmd_cwr(CHIP_SDRV_ADDR + offset, (uint8_t *)(gChnSdrv.sdrvAddr + offset), RAM_PAGE_SIZE);
            load_pgidx++;
            return true;
        }
        /*if (baudrate)
        {
            // modify chipSet uart1 baudrate.
            pt_cmd_swr(0x080000C0, baudrate);
        }*/

        load_goto(LOAD_JUMP);
        load_pgidx = 0;
        pt_cmd_jump(CHIP_SDRV_ADDR);
        return true;
    }

    if (load_step == LOAD_JUMP)
    {
        load_goto(LOAD_WAIT);

        // wait RSP sync 1.76ms
        pt_code = PT_RSP_SYNC;
        pt_time = sftmr_tick();
        return true;
    }

    // finish
    load_goto(LOAD_NONE);
    return false;
}

#define LINE_CTRL_VAL        (LCR_BITS_DFLT | (/*RXEN*/1 << 5))

void chipLoadStart(uint8_t action)
{
    if (action >= ACT_MAX) return;

    uint32_t baudrate = chip_baud();
    burnerBusy(action == ACT_ONLINE ? OP_ONLINE_MD : OP_BURN_CHIP);
    
    if (!enter_boot())
    {
        burnerFail(PERR_TIME_OUT);
        return;    
    }

    if (baudrate)
    {
        uint32_t baudChip = (BRR_DIV(baudrate, 16M) | (uint32_t)LINE_CTRL_VAL << 16);  
        
        load_goto(LOAD_BAUD);
                
        uart_wait(UART2_PORT);
        
        bootDelayMs(10);  // 
        
        // Modify Boot Baudrate
        UART2->LCR.RXEN = 0;  //UART is invalid of MODIFY_BAUD in busy.211118 --whl

        pt_cmd_baud(baudChip);
        uart_wait(UART2_PORT);
        bootDelayMs(10);  // 1ms      
        
        UART_MODIFY_BAUD(2, baudrate);
        UART2->LCR.RXEN = 1;  //211118 --whl
        
        pt_cmd_baud(baudChip);
        
        GPIO_DAT_SET(GPIO04);
        GPIO_DAT_CLR(GPIO04);
    }
    else
    {
        load_goto(LOAD_SYNC);

        pt_cmd_sync();
    }
}

void chipLoadSync(uint8_t code)
{
    if ((code == SYNC_OK_CHIP) && (load_step == LOAD_WAIT))
    {
        load_goto(LOAD_NONE);

        burnerEvt(PERR_NO_ERROR, OP_LOAD_SDRV);
    }
    else if ((code == SYNC_OK_BOOT) && (load_step == LOAD_SYNC))
    {
        load_sdrv();
    }
}

void chipLoadCont(uint8_t rsp, uint8_t status)
{
    if (rsp == PT_RSP_BAUDEND)
    {
        if (load_step != LOAD_BAUD) return; // filter repeat
    }
    /*else if (rsp == PT_RSP_CWR)
    {
        if (load_step != LOAD_CWR) return;
    }
    else if (rsp == PT_RSP_JUMP)
    {
        if (load_step != LOAD_JUMP) return;
    }*/

    if (status == PT_OK)
    {
        load_sdrv();
    }
    else
    {
        load_goto(LOAD_NONE);
        burnerFail(PERR_RSP);
    }
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

static void firm_earse(void)
{
    firm_goto(STEP_ERASE);

    if (gChnFirm.firmCFG.bConf & BIT(FW_ERASE_ALL_POS))
    {
        pt_cmd_er_flash(FLASH_CHIP, 0, NULL);    
    }
    else
    {
        uint8_t cnts = 0;
        struct er_map maps[3];
        
        // INFO PAGE0
//        maps->eridx = ADDR2PAGE(FSH_ADDR_CODE_INFO);
//        maps->ercnt = 1;
        if (!IS_NONE(gChnFirm.codeAddr))
        {
            // CODE PAGEx
            maps[cnts].eridx = ADDR2PAGE(gChnFirm.codeAddr);
            maps[cnts].ercnt = ADDR2PAGE_H(gChnFirm.codeLen);
            cnts++;    
        }
        
        if (!IS_NONE(gChnFirm.dataAddr))
        {      
            // DATA PAGEx
            maps[cnts].eridx = ADDR2PAGE(gChnFirm.dataAddr);
            maps[cnts].ercnt = ADDR2PAGE_H(gChnFirm.dataLen);
            cnts++;            
        }
        
//        if (!IS_NONE(gChnFirm.libAddr))
//        {           
//            // BLELIB PAGEx
//            maps[cnts].eridx = ADDR2PAGE(gChnFirm.libAddr);
//            maps[cnts].ercnt = ADDR2PAGE_H(gChnFirm.libLen);
//            cnts++;  
//        }            
        
        pt_cmd_er_flash(FLASH_PAGE, cnts, maps);       
    }
}

static bool firm_burn(void)
{
    uint16_t step_pages;
    uint16_t dest_pgidx;
    uint8_t *src_pgptr;

    if (burn_step == STEP_CODE)
    {
        step_pages = ADDR2PAGE_H(gChnFirm.codeLen);
        if (burn_pgidx < step_pages)
        {
            dest_pgidx = ADDR2PAGE(gChnFirm.codeAddr) + burn_pgidx;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_BASE + burn_pgidx * FSH_PAGE_SIZE);

            pt_cmd_wr_flash(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }

        // step over to next
        firm_goto(STEP_DATA);
    }

    if (burn_step == STEP_DATA)
    {
        step_pages = ADDR2PAGE_H(gChnFirm.dataLen);
        if (burn_pgidx < step_pages)
        {
            dest_pgidx = ADDR2PAGE(gChnFirm.dataAddr) + burn_pgidx;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_BASE + (burn_pgidx + ADDR2PAGE_H(gChnFirm.codeLen)) * FSH_PAGE_SIZE);

            pt_cmd_wr_flash(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }

        // step over to next
        firm_goto(STEP_INFO);
    }

    if (burn_step == STEP_INFO)
    {
        if (burn_pgidx == 0/*InfoPage*/)
        {
            dest_pgidx = ADDR2PAGE(FSH_ADDR_CODE_INFO); // 0;
            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_INFO);

            pt_cmd_wr_flash(dest_pgidx, FSH_PAGE_SIZE, src_pgptr);
            burn_pgidx++;
            return true;
        }
        else if (burn_pgidx == 1/*macPage*/)
        {
            if (gChnFirm.firmCFG.bConf & FWCFG_EN(BURN_MAC))
            {
                pt_cmd_mf_flash(gChnFirm.macCFG.offset, 6, (uint8_t *)&gMacInfo.macCurr);
                burn_pgidx++;
                return true;
            }
        }
    }

    // step over to finish
    firm_goto(STEP_NONE);
    return false;
}

void chipBurnStart(void)
{
    if (gChnFirm.firmCFG.bConf & FWCFG_EN(BURN_MAC))
    {
        if ((gChnFirm.macCFG.ncount > 0) && (gChnFirm.macCFG.ncount <= gMacInfo.cntCurr))
        {
            burnerFail(PERR_MAC_END);
            return;
        }
    }

    burnerBusy(OP_BURN_CHIP);

    firm_earse();
}


/**
 ****************************************************************************************
 * @brief Read a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.
 ****************************************************************************************
 */
uint16_t read16p(const void *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}

/**
 ****************************************************************************************
 * @brief Write a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
void write16p(const void *ptr16, uint16_t value)
{
    uint8_t *ptr=(uint8_t*)ptr16;

    *ptr++ = value&0xff;
    *ptr = (value&0xff00)>>8;
}

void chipBurnCont(uint8_t rsp, uint8_t status)
{
    if (gBurner.opCode != OP_BURN_CHIP)
        return;

    if (rsp == PT_RSP_ER_FLASH)
    {
        if (burn_step != STEP_ERASE) return;

        firm_goto(STEP_CODE);
    }
    else // (rsp == PT_RSP_WR_FLASH)
    {
        if (burn_step < STEP_CODE) return;
    }

    if (status == PT_OK)
    {
        if (!firm_burn())
        {
            // no More, finish to SAVE and TEST
            gMacInfo.cntCurr++;
            if (rsp == PT_RSP_MF_FLASH)// && (gChnFirm.firmCFG.bConf  & FWCFG_EN(BURN_MAC))
            {
//                gMacInfo.macCurr += gChnFirm.macCFG.ndelta;                
                uint32_t mac_old =  RD_32(&gMacInfo.macCurr[0]);
                WR_32(&gMacInfo.macCurr[0], mac_old + gChnFirm.macCFG.ndelta);
                
                if (mac_old > RD_32(&gMacInfo.macCurr[0]))
                {
                    write16p(&gMacInfo.macCurr[4], RD_32(&gMacInfo.macCurr[4])+1);
                }
            }
            prg_flash_modify(FSH_ADDR_MAC_INFO, sizeof(gMacInfo), (uint8_t *)&gMacInfo);

            burnerEvt(PERR_NO_ERROR, OP_BURN_CHIP);
        }
    }
    else
    {
        firm_goto(STEP_NONE);
        burnerFail(PERR_RSP);
    }
}
/*}}*/
