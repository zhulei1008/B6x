#include <string.h>
#include "hyb5x.h"
#include "sftmr.h"
#include "gpio.h"
#include "reg_gpio.h"
#include "reg_uart.h"
#include "reg_iopad.h"
#include "ota_itf.h"
#include "burner.h"
#include "proto.h"
#include "flash.h"
#include "chipfirm.h"
#include "prg_api.h"

//static void xdelay(uint16_t x)
//{
//    while (x--)
//    {
//        __NOP();
//        __NOP();
//    }
//}

/*LOAD-OpCode{{*/
enum load_step
{
    LOAD_NONE,
//    LOAD_BAUD,
    LOAD_SYNC,
//    LOAD_CWR,
//    LOAD_JUMP,
//    LOAD_WAIT,
};

static uint8_t load_step;
//static uint8_t load_pgidx;

#define load_goto(step)  load_step = step

void chipLoadStart(uint8_t action)
{
    if (action >= ACT_MAX) return;

    burnerBusy(OP_BURN_CHIP);
    
	load_goto(LOAD_SYNC);

	pt_cmd_sync();
}

void chipLoadSync(uint8_t code)
{
    if (code == SYNC_OK_CHIP)
    {
        load_goto(LOAD_NONE);

        burnerEvt(PERR_NO_ERROR, OP_LOAD_SDRV);
    }
    else if ((code == SYNC_OK_BOOT) && (load_step == LOAD_SYNC))
    {
//        load_sdrv();
    }
}

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
    struct er_map ermap[2] = {0};
	
	firm_goto(STEP_ERASE);
	
	if (!IS_NONE(gChnFirm.codeAddr))
	{
		ermap[0].eridx = ADDR2PAGE(gChnFirm.codeAddr);
		ermap[0].ercnt = gChnFirm.codeLen/PAGE_SIZE;	
	}
	
	if (!IS_NONE(gChnFirm.dataAddr))
	{
		ermap[1].eridx = ADDR2PAGE(gChnFirm.dataAddr); 
		ermap[1].ercnt = gChnFirm.dataLen/PAGE_SIZE;	
	}		
	
	pt_cmd_er_flash(FLASH_PAGE, 2, ermap);
}

static bool firm_burn(void)
{
    uint16_t step_pages;
    uint16_t dest_pgidx;
    uint8_t *src_pgptr;
      
    if (burn_step == STEP_CODE)
    {
        step_pages = ADDR2PAGE_H(gChnFirm.codeLen);
        
		//FLASH
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
    
//    if (burn_step == STEP_OTP)
//    {              
//        //For Test        
//        gChnFirm.otpOffset = ((gChnFirm.dataAddr - 0x15000) >> 8) & 0xFFFF;         
//        gChnFirm.otpLen = gChnFirm.dataLen & 0xFFFF;
//        
//        step_pages = ADDR2PAGE_H(gChnFirm.otpLen);
//        if (burn_pgidx < step_pages)
//        {            
//            uint16_t dest_offset = gChnFirm.otpOffset + (burn_pgidx * OTP_PAGE_SIZE)/4;
//            uint16_t dest_len    = (step_pages - burn_pgidx - 1) ? OTP_PAGE_SIZE : (gChnFirm.otpLen - burn_pgidx * OTP_PAGE_SIZE);
//            src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_BASE + (burn_pgidx + (ADDR2PAGE_H(gChnFirm.codeLen))/* + ADDR2PAGE_H(gChnFirm.dataLen)*/) * OTP_PAGE_SIZE);
//            
//            pt_cmd_wr_otp(dest_offset, dest_len, src_pgptr);
//            burn_pgidx++;
//            return true;            
//        } 
        
        // step over to next
//        firm_goto(STEP_INFO);        
//    }
    
    if (burn_step == STEP_INFO)
    {
        if (burn_pgidx == 0/*InfoPage*/)
        {
//            if (gChnFirm.firmCFG.bConf & BIT(FW_BURN_OTP_POS))
//            {
//                uint16_t dest_offset = 0x7D8;
//                uint16_t dest_len = 4*4;
////                src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_INFO);
//                gChnFirm.magicCode = 0xAA55A001; // code type, RAM JUMP PATCH    
//                gChnFirm.codeLen   = (gChnFirm.codeLen + 3)/4; //0x72B;   // code word length otpInit1();
//                gChnFirm.codeAddr  = 0x00;  // position in storage    
//                gChnFirm.sramAddr  = 0x8000000;  // sram addr, when need copy to RAM    otpWriteAndProgram1(0x7D8,
////                gChnFirm.magicCode = userAddr  = 0x00;  // sram addr, only mirror user OTP-data(1K)    otpWriteAndProgram1();
//                
//                pt_cmd_wr_otp(dest_offset, dest_len, (uint8_t *)&gChnFirm);                           
//            }
//            else //FLASH
            {
//                dest_pgidx = ADDR2PAGE(FSH_ADDR_CODE_INFO); // 0;
//                src_pgptr = (uint8_t *)(FSH_ADDR_FIRM_INFO);

//                pt_cmd_wr_flash(dest_pgidx, FSH_PAGE_SIZE, src_pgptr); 
				
				pt_cmd_mf_flash(0x00, sizeof(gChnFirm), (uint8_t *)&gChnFirm);				
            }
            
            burn_pgidx++;
            return true;
        }
        else if (burn_pgidx == 1/*macPage*/)
        {
            if (gChnFirm.firmCFG.bConf & FWCFG_EN(BURN_MAC))
            {
                pt_cmd_mf_flash(gChnFirm.macCFG.offset, 4, (uint8_t *)&gMacInfo.macCurr);
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
            if ((rsp == PT_RSP_MF_FLASH) && (gChnFirm.firmCFG.bConf  & FWCFG_EN(BURN_MAC)))
            {
                gMacInfo.macCurr += gChnFirm.macCFG.ndelta;
            }
            prg_flash_modify(FSH_ADDR_MAC_INFO, sizeof(gMacInfo), (uint8_t *)&gMacInfo);
			
//			pt_cmd_rst();//Slave Reset Auto by disconnect
			
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
