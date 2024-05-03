#include <stdint.h>
#include <string.h>
#include "b6x.h"
#include "flash.h"
#include "prg_api.h"
#include "gpio.h"
#include "fshc.h"
#include "..\src\trim.h"
#include "string.h"
#include "trim_test.h"

#define PAGE_SIZE 0x100  //256Byte
#define SECT_SIZE 0x1000 //4kByte

void flash_wr_protect(uint8_t sta)
{
    // Ð´±£»¤0x000-0xFFF (4KB)
    // write en singal
    fshc_en_cmd(FSH_CMD_WR_EN);
    // write state enable for write flash state
    fshc_en_cmd(FSH_CMD_WR_STA_EN);  // No FT enable
    // send write sta cmd
    fshc_wr_sta(FSH_CMD_WR_STA, 1, sta);
    
    bootDelayMs(12);    
}

void prg_flash_erase(uint8_t mode, uint8_t mcnt, struct er_map* maps)
{	
	if (mode == FLASH_CHIP)
	{
        uint32_t trim_data[PAGE_SIZE/4];
        
        GLOBAL_INT_DISABLE();        
        fshc_read(FSH_TRIM_OFFSET, trim_data, PAGE_SIZE/4, FSH_CMD_RD);       
		fshc_erase(0, FCM_ERCHIP_BIT | FSH_CMD_ER_CHIP);  // 256K&521K
        GLOBAL_INT_RESTORE();
//        if (trim_data[TRIM_VALID_IDX] == TRIM_VALID_VALUE)
        {
           fshc_write(FSH_TRIM_OFFSET, trim_data, PAGE_SIZE/4, FSH_CMD_WR);             
        }
        return;
	}
    
    if (mode & 0x80)
    {
        mode &= 0x7F;
        GLOBAL_INT_DISABLE();
        fshc_erase(0, FSH_CMD_ER_PAGE); // Erase Page0
        GLOBAL_INT_RESTORE();
    }
    
    uint16_t er_size = (mode == FLASH_SECTOR ? SECT_SIZE:PAGE_SIZE);
    
    for (uint8_t i = 0; i < mcnt; i++)
    {
        for (uint16_t cnt_i = 0; cnt_i < maps[i].ercnt; cnt_i++)
        {
            if (mode == FLASH_PAGE)
            {
                if ((((maps[i].eridx + cnt_i) % 0x10) == 0) && ((maps[i].ercnt - cnt_i) >= 0x10))
                {
                    GLOBAL_INT_DISABLE();
                    fshc_erase(((maps[i].eridx + cnt_i)/0x10)*SECT_SIZE, FSH_CMD_ER_SECTOR);
                    GLOBAL_INT_RESTORE();
                    cnt_i += 0x0F;
                    continue;
                }
            }
            
            GLOBAL_INT_DISABLE();
            if (mode == FLASH_PAGE)
                fshc_erase((maps[i].eridx + cnt_i)*er_size, FSH_CMD_ER_PAGE);
            else
                fshc_erase((maps[i].eridx + cnt_i)*er_size, FSH_CMD_ER_SECTOR);    
            GLOBAL_INT_RESTORE();            
        }
    }
}

void prg_flash_read(uint16_t pgidx, uint16_t dlen, uint8_t* buff)
{
    uint32_t new_data[PAGE_SIZE/4];
    
	fshc_read(pgidx*PAGE_SIZE, new_data, dlen/4, FSH_CMD_RD);
    
    memcpy(buff, (uint8_t*)new_data, dlen);
}

//bool prg_flash_verify(uint16_t pgidx, uint16_t dlen, const uint8_t *data)
//{
//	uint8_t *addr = prg_flash_paddr(pgidx);
//    uint8_t r_data[PAGE_SIZE];
//    uint8_t result;
//    
//    prg_flash_read(pgidx, dlen, r_data);

//    result = memcmp(r_data, data, dlen);

//	return (result == 0);
//}

bool prg_flash_verify(uint16_t pgidx, uint16_t dlen, const uint8_t *data)
{
	uint8_t *addr = prg_flash_paddr(pgidx);

	return (memcmp(addr, data, dlen) == 0);
}

bool prg_flash_write(uint16_t pgidx, uint16_t dlen, const uint8_t* data)
{	
    uint32_t new_data[PAGE_SIZE/4];
    
	uint32_t offset = pgidx*PAGE_SIZE;
    
	GPIO_DAT_CLR(GPIO13);
    memcpy((uint8_t*)new_data, data, dlen);
    GLOBAL_INT_DISABLE();
	fshc_write(offset, new_data, dlen/4, FSH_CMD_WR);
    GLOBAL_INT_RESTORE();
    GPIO_DAT_SET(GPIO13);
    
	return (prg_flash_verify(pgidx, dlen, data));
}

bool prg_flash_modify(uint32_t addr, uint16_t dlen, const uint8_t* data)
{
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
