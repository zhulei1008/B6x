#ifndef _PROGRAM_H_
#define _PROGRAM_H_

#include <stdint.h>
#include <stdbool.h>

#include "proto.h"    // define 'struct er_map'
//struct er_map;
//{
//    uint16_t eridx;
//    uint16_t ercnt;
//};

#define PRG_PAGE_SIZE  (0x100)  // <= PAYL_DAT_MAX_SIZE 
#define PRG_SECT_SIZE  (0x1000) // 4KB
#define ERASE_MODE_MAX 4
#define ERASE_BLOCK32_MODE   2
#define ERASE_SECTOR_MODE   1
/// Flash
#define prg_flash_addr(offset)    (uint8_t *)(0x18000000 + (offset))
#define prg_flash_paddr(page_idx) (uint8_t *)(0x18000000 + (page_idx)*0x100)
#define prg_flash_saddr(sect_idx) (uint8_t *)(0x18000000 + (sect_idx)*0x1000)

void prg_flash_erase(uint8_t mode, uint8_t mcnt, struct er_map* maps);

void prg_flash_read(uint16_t pgidx, uint16_t dlen, uint8_t *buff);

bool prg_flash_write(uint16_t pgidx, uint16_t dlen, const uint8_t *data);

bool prg_flash_verify(uint16_t pgidx, uint16_t dlen, const uint8_t *data);

bool prg_flash_modify(uint32_t addr, uint16_t dlen, const uint8_t* data);

void prg_flash_trim(uint8_t cap_val);

/// OTP
void prg_otp_read(uint16_t offset, uint16_t dlen, uint8_t *buff);
bool prg_otp_verify(uint16_t offset, uint16_t dlen, uint8_t *data);
bool prg_otp_write(uint16_t offset, uint16_t dlen, uint8_t *data);

#endif // _PROGRAM_H_
