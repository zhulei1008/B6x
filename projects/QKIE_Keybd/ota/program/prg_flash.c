#include <stdint.h>
#include <string.h>
#include "flash.h"
#include "prg_api.h"
#include "gpio.h"
#include "user_api.h"
#include "drvs.h"
#include "regs.h"
#include "cfg.h"
#if (DBG_CHIPSET)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<CHIP>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif
#define PAGE_SIZE 0x100  //256Byte
#define SECT_SIZE 0x1000 //4kByte

//void flashReadSec(uint32_t offset, uint16_t len, uint8_t *buff);
//void flashSecErase(uint32_t offset);
//void flashWriteSec(uint32_t offset, uint32_t len, uint8_t* data, uint16_t timeout);
#if (1)

const uint8_t ERASE_CMD[ERASE_MODE_MAX] =
{
    /*ERASE_PAGE    = */0x81,
    /*ERASE_SECTOR  = */0x20,
    /*ERASE_BLOCK32 = */0x52,
    /*ERASE_BLOCK64 = */0xD8,
};

const uint8_t PER_SIZE[ERASE_MODE_MAX] =
{
    8,   // Page Size 256Bytes
    12,  // Sector Size 4KBytes
    15,  // Block Size 32KBytes
    16,  // Block Size 64KBytes
};

__SRAMFN void flashMultiErase(uint8_t erase_mode, uint32_t idx, uint32_t cnt)
{
    GLOBAL_INT_DISABLE();
    while (SYSCFG->ACC_CCR_BUSY);
    
    for (uint16_t i = 0; i < cnt; i++)
    {
        fshc_erase((idx << PER_SIZE[erase_mode]), ERASE_CMD[erase_mode]);
        
        ++idx;
    }
    GLOBAL_INT_RESTORE();
}
#endif

void prg_flash_erase(uint8_t mode, uint8_t mcnt, struct er_map* maps)
{	
	if (mode == FSH_CMD_ER_CHIP)
	{
		//flashChipErase(); //zhushuai
        return;
	}
    
    if (mode & 0x80)
    {
        mode &= 0x7F;
        flash_erase_write(0, NULL); // Erase Page0 //zhushuai
    }
    
    for (uint8_t i = 0; i < mcnt; i++)
    {
        flashMultiErase(mode, maps[i].eridx, maps[i].ercnt);//zhushuai
    }
}

void prg_flash_read(uint16_t pgidx, uint16_t dlen, uint8_t* buff)
{
    uint32_t new_data[PAGE_SIZE/4];
    
	fshc_read(pgidx*PAGE_SIZE, new_data, dlen/4, FSH_CMD_RD);
    
    memcpy(buff, (uint8_t*)new_data, dlen);
}

bool prg_flash_verify(uint16_t pgidx, uint16_t dlen, const uint8_t *data)
{
	uint8_t *addr = prg_flash_paddr(pgidx);
	return (memcmp(addr, data, dlen) == 0);
}

bool prg_flash_write(uint16_t pgidx, uint16_t dlen, const uint8_t* data)
{	
    uint32_t new_data[PAGE_SIZE/4];
    
	uint32_t offset = pgidx*PAGE_SIZE;
    
//	GPIO_DAT_CLR(GPIO13);
    memcpy((uint8_t*)new_data, data, dlen);
    GLOBAL_INT_DISABLE();
	fshc_write(offset, new_data, dlen/4, FSH_CMD_WR);
    GLOBAL_INT_RESTORE();
//    GPIO_DAT_SET(GPIO13);
    
	return (prg_flash_verify(pgidx, dlen, data));
}

bool prg_flash_modify(uint32_t addr, uint16_t dlen, const uint8_t* data)
{
//	__DATA_ALIGNED(4) uint8_t old_data[PAGE_SIZE];//zhushuai
//    uint16_t cidx = addr & 0xFF;
//    uint16_t pgidx = (addr >> 8) & 0xFFFF;
//    uint32_t offset = pgidx*PAGE_SIZE;
//	
//	fshc_read(offset, (uint32_t*)old_data,PAGE_SIZE>>2,FSH_CMD_RD);//zhushuai
//	memcpy(&old_data[cidx], data, dlen);

//	flash_erase_write(offset,NULL);//zhushuai
//    return (prg_flash_write(pgidx, PAGE_SIZE, old_data));
 	uint32_t old_data[PAGE_SIZE/4];
    uint16_t cidx = addr & 0xFF;
    uint16_t pgidx = (addr >> 8) & 0xFFFF;
    uint32_t offset = pgidx*PAGE_SIZE;
	
	fshc_read(offset, old_data, PAGE_SIZE/4, FSH_CMD_RD);
	memcpy((uint8_t *)&old_data[cidx/4], data, dlen);
    GLOBAL_INT_DISABLE();
	fshc_erase(offset, FSH_CMD_ER_PAGE);
    GLOBAL_INT_RESTORE();
    return (prg_flash_write(pgidx, PAGE_SIZE, (uint8_t*)old_data));
}


//#define FLASH_OTP_TRIM_INFO_ADDR 0x1000

//void prg_flash_trim(uint8_t cap_val)
//{
//    uint8_t trim_info[16];
//    
//    flashReadSec(FLASH_OTP_TRIM_INFO_ADDR, 16, trim_info);
//    if ((trim_info[0] & 0x3F) == cap_val)
//    {
//        return; // same
//    }
//    
//    flashSecErase(FLASH_OTP_TRIM_INFO_ADDR);
//    trim_info[0] = cap_val | 0x80;
//    flashWriteSec(FLASH_OTP_TRIM_INFO_ADDR, 16, trim_info, 2000);
//}
