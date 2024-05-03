#include <string.h>
#include <stdbool.h>
#include "b6x.h"
#include "gpio.h"
#include "reg_iopad.h"
#include "proto.h"
#include "prg_api.h"

#define PRG_SRAM_ADDR  0x08000000

struct sram_code_tag
{
    uint32_t addr;
    uint16_t size; // < 40KB
    uint8_t  pages;
    uint8_t  remain;
};

struct sram_env_tag
{
    uint8_t step;
    uint8_t page;
};

struct sram_code_tag gSramCode;
struct sram_env_tag  gSramEnv;

static void prg_sram_enter(uint8_t step, uint16_t tcnt)
{
    gSramEnv.step = step;
}

bool prg_sram_burn(void)
{
    uint16_t offset = (uint16_t)gSramEnv.page * PRG_PAGE_SIZE;
    uint16_t length = (gSramEnv.page < gSramCode.pages) ? PRG_PAGE_SIZE : gSramCode.remain;
    
    if ((gSramEnv.page > gSramCode.pages) || (length == 0))
    {
        pt_cmd_jump(PRG_SRAM_ADDR);
        prg_sram_enter(PRG_JUMP, 1);
    }
    else
    {
        pt_cmd_cwr(PRG_SRAM_ADDR+offset, (uint8_t *)(gSramCode.addr+offset), length);
        prg_sram_enter(PRG_BURN, 1);
    }
    gSramEnv.page++;
}

bool prg_sram_info(void)
{
    gSramCode.addr = RD_32(0x18000000 + 8);
    gSramCode.size = RD_32(0x18000000 + 4);
    gSramCode.pages = gSramCode.size / PRG_PAGE_SIZE;
    gSramCode.remain = gSramCode.size % PRG_PAGE_SIZE;
}
#define DELAY_BOOT 4
static void enter_boot(void)
{
    IOMCTL->PIOA[PIN_TX2].Word  = 0;
    IOMCTL->PIOA[PIN_RX2].Word  = 0;
	IOMCTL->PIOA[PIN_RST].Word = 0;
	
	gpioDirSet(PIN_TX2);
	gpioDirSet(PIN_RX2);
	gpioDirSet(PIN_RST);
    
    gpioDataClr(PIN_RST);
    gpioDataClr(PIN_TX2);
	gpioDataClr(PIN_RX2);
	
	sfTmrDelay10Ms(DELAY_BOOT); //10ms
	
    //gpioDataSet(PIN_RST);
	gpioDirClr(PIN_RST);	
	sfTmrDelay10Ms(DELAY_BOOT); //10ms
    gpioDataSet(PIN_TX2);
	gpioDataSet(PIN_RX2);
	sfTmrDelay10Ms(DELAY_BOOT); //10ms

    uart2_init();
	sfTmrDelay10Ms(DELAY_BOOT); //10ms
}

void prg_sram_sync(void)
{
    enter_boot();

    // read_rom to sync
    pt_cmd_srd(0x200);
    
    gSramEnv.page = 0;
    prg_sram_enter(PRG_SYNC, 1);
}

void prg_sram_rsp_timout(void)
{

}

void prg_sram_rsp_handler(uint8_t status)
{
    if (status != PT_OK)
    {
        prg_sram_enter(PRG_IDLE, 1);
    }
    else
    {
        if (gSramEnv.step == PRG_JUMP)
        {
        
        }
        else
        {
            prg_sram_burn();
        }
    }
}


void prg_sram_proc(void)
{
    
}
