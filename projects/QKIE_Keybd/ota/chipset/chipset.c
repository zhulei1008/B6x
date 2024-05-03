#include <string.h>
#include "drvs.h"
//#include "reg_rfip.h"
#include "utils.h"
#include "chipset.h"
#include "proto.h"
#include "user_api.h"
#include "prg_api.h"
#include "flash.h"
#include "prf_otas.h"
#include "prf_api.h"
#include "regs.h"
#include "ota_itf.h"
#include "user_api.h"
#if (DBG_CHIPSET)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<CHIP>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#if (OTA_CHIP || OTA_HOST)
#define FLASH_CODE_LEN                (0x18000004)
#define FLASH_CODE_LEN_OFFSET         (0x00000004)
#define FLASH_CODE_ADDR               (0x18000008)

#define FIRM_BIN_FLASH_ADDR1          (0x18004000)
#define FIRM_BIN_FLASH_ADDR2          (0x18020000)

#define FIRM_BIN_IDX_BLOCK(addr)      (((addr - 0x18000000) >> 15))
#define FIRM_BIN_IDX_SECTOR(addr)      (((addr - 0x18000000) >> 12))
#define FLIB_BIN_IDX_PAGE(addr, idx)  (((addr - 0x18000000) >> 8) + idx)

firmInfo_t gChipFirm;
chip_t gChip;
static uint32_t firm_addr;
static uint16_t g_curr_pgidx;
uint8_t ota_is_ok;
extern void flashMultiErase(uint8_t erase_mode, uint32_t idx, uint32_t cnt);
void Reset_Handler (void);

extern struct rxd_buffer data_rxd;

// 获取当前执行code烧录地址, 用来确定需要更新的code烧录地址
void firm_bin_init(void)
{
    uint32_t flash_info = read32((uint32_t *)FLASH_CODE_ADDR);
    
    if (flash_info == FIRM_BIN_FLASH_ADDR1)
    {
        firm_addr = FIRM_BIN_FLASH_ADDR2;
    }
    
    if (flash_info == FIRM_BIN_FLASH_ADDR2)
    {
        firm_addr = FIRM_BIN_FLASH_ADDR1;
    }
    DEBUG("firm_addr:0x%x",firm_addr);
    g_curr_pgidx = 0;
}

// 先擦除需要烧录的地址
void firm_bin_erase(void)
{ 
    // erase flash
    if (firm_addr == FIRM_BIN_FLASH_ADDR1)
    {
        // erase Code1
        flashMultiErase(ERASE_SECTOR_MODE, FIRM_BIN_IDX_SECTOR(FIRM_BIN_FLASH_ADDR1), 4);//zhushuai//4*4=16K,0x18004000-0x18008000               
        flashMultiErase(ERASE_BLOCK32_MODE, FIRM_BIN_IDX_BLOCK(FIRM_BIN_FLASH_ADDR1+0x4000), 3);//zhushuai 0x18008000-0x20000000//96k
        //Total size=96k+16k
    }
    else if (firm_addr == FIRM_BIN_FLASH_ADDR2)
    {
        // erase Code2
        flashMultiErase(ERASE_BLOCK32_MODE, FIRM_BIN_IDX_BLOCK(FIRM_BIN_FLASH_ADDR2), 4);//zhushuai 4*32k=128k
        //Total size = 128k
    }

    pt_rsp_status(PT_RSP_ER_FLASH, PT_OK);
}

// 修改boot跳转info页
void firm_bin_end(uint32_t lens)
{
    uint8_t status = PT_OK;

    if (lens)
    {
        uint8_t modify_data[8] = {0};
        write32p(modify_data, lens);
        write32p(modify_data+4, firm_addr);
        
        gChip.state = PSTA_OK;
        ota_is_ok =1;
        if (!prg_flash_modify(FLASH_CODE_LEN_OFFSET, 8, modify_data))
        {
            status = PT_ERR_VERIFY;
            gChip.state = PSTA_FAIL;
            DEBUG("PT_ERR_VERIFY\r\n");
            ota_is_ok =0;
        }
    }

    pt_rsp_status(PT_RSP_MF_FLASH, status);
    
    g_curr_pgidx = 0;
}

/// Retrieve from Flash
static void infoReset(void)
{
    // Retrieve for Upgrade failed
    memcpy(&gChipFirm, (uint32_t *)FSH_ADDR_CODE_INFO, sizeof(gChipFirm));
    debugHex((uint8_t*)&gChipFirm,sizeof(gChipFirm));
    // Zero Clean
    memset(&gChip,  0, sizeof(gChip));    
    memset(&data_rxd,0,sizeof(data_rxd));
//    // Valid Check
}

/// Recover from Sram
void otaEnd(void)
{
	if (gChip.state & PSTA_FAIL)
	{
		// Upgrade failed  Recover important data		
		while (!prg_flash_modify(FSH_ADDR_CODE_INFO, sizeof(gChipFirm),(uint8_t *)&gChipFirm));
		// Recover Other important data
	}
	
	NVIC_SystemReset();
}

/// Burner Cmd Parser
static void burner_parser(struct pt_pkt *pkt, uint8_t status)
{
    if (status != PT_OK)
    {
        // pt_send_rsp(pkt); // debug
        pt_rsp_status(pkt->code, status);
        return;
    }

    switch (pkt->code)
    {
        case PT_CMD_VERSION:
        {
            pt_rsp_version(VER_CHIPSET);
            
        } break;

        case PT_CMD_SYNC:
        {
            firm_bin_init();
            
			gChip.state |= PSTA_SYNCED;
            pt_rsp_sync(SYNC_OK_CHIP);
        } break;

#if (PT_BOOT_CMD) /* Boot */
        case PT_CMD_SRD:
        {
            PKT_PARAM(struct pt_cmd_srd);
            pt_rsp_srd(RD_32(param->addr));
        } break;

        case PT_CMD_CWR:
        {
            PKT_PARAM(struct pt_cmd_cwr);
            memcpy((void *)(param->addr), param->data, pkt->len - 5);
            pt_rsp_status(PT_RSP_CWR, PT_OK);
        } break;

        case PT_CMD_BAUD:
        {
            pt_rsp_status(PT_RSP_BAUDRV, PT_OK);
            WAIT_UART_IDLE(1);

            PKT_PARAM(struct pt_cmd_baud);
            UART_MODIFY_BAUD(1, param->baud);
            pt_rsp_status(PT_RSP_BAUDEND, PT_OK);
        } break;

#endif
//        case PT_CMD_RST:
//        {
//			otas_env_t *otas_env = PRF_ENV_GET(OTAS, otas);
//			
//            pt_rsp_status(PT_RSP_RST, PT_OK);
//            while(otas_env->nb_pkt == OTAS_NB_PKT_MAX);
//			
//			//OTA
//			if (gChip.state & PSTA_FAIL)
//			{
//				NVIC_SystemReset();
//			}
//            NVIC_SystemReset();
//        } break;
		
#if (PT_FLASH_CMD) /* Flash */
        case PT_CMD_ER_FLASH:
        {
//            PKT_PARAM(struct pt_cmd_er_fsh);

//            prg_flash_erase(param->mode, param->mcnt, &param->maps[0]);
//            pt_rsp_status(PT_RSP_ER_FLASH, PT_OK);

              uint32_t code_addr = ((pkt->payl[3]<<8)|pkt->payl[2])*0x100;
              code_addr +=0x18000000;
              DEBUG("code_addr:0x%x",code_addr);
              if(code_addr==firm_addr)
              {
                  sys_Timeout_Cnt = 0;
                  firm_bin_erase();
                  
              }
              else
              {
                  ota_is_ok = 0;
                  pt_rsp_status(PT_RSP_ER_FLASH, PT_ERROR);
              }
             // debugHex(pkt->payl,pkt->len);
            //
        } break;

#if (CFG_READ)
        case PT_CMD_RD_FLASH:
        {
            PKT_PARAM(struct pt_cmd_rd_fsh);          
            
            pt_rsp_rd_flash(param->pgidx, param->length, prg_flash_paddr(param->pgidx));
        } break;
#endif

        case PT_CMD_WR_FLASH:
        {
            PKT_PARAM(struct pt_cmd_wr_fsh);

            // Write and Vertify
            if (!prg_flash_write(FLIB_BIN_IDX_PAGE(firm_addr, g_curr_pgidx), pkt->len - 3, param->data))
            {
                flash_erase_write(FLIB_BIN_IDX_PAGE(firm_addr, g_curr_pgidx),NULL);//For Retransmission //zhushuai
				
				pt_rsp_status(PT_RSP_WR_FLASH, PT_ERR_VERIFY);
            }
			else
			{
				pt_rsp_status(PT_RSP_WR_FLASH, status);
			}
            
            g_curr_pgidx++;
        } break;

        case PT_CMD_VF_FLASH:
        {
            PKT_PARAM(struct pt_cmd_vf_fsh);

            if (prg_flash_verify(param->pgidx, pkt->len - 3, param->data))
            pt_rsp_status(PT_RSP_VF_FLASH, PT_OK);              
            else
            pt_rsp_status(PT_RSP_VF_FLASH, PT_ERR_VERIFY);
        } break;

        case PT_CMD_MF_FLASH:
        {
            PKT_PARAM(struct pt_cmd_mf_fsh);
            
            uint32_t code_lens = read32p(((uint8_t*)pkt)+12);
            firm_bin_end(code_lens);
            
        } break;
#endif

//        case PT_CMD_TRIMVAL:
//        {
//            PKT_PARAM(struct pt_cmd_trimval);
//            uint8_t cap_val = param->cap & 0x7F;

//            if (cap_val < 0x40)
//            {
//                rf->reg18.xo16m_cap_trim = cap_val; // set

//                if (param->cap & 0x80)
//                    prg_flash_trim(cap_val); // save
//            }
//            else
//            {
//                cap_val = rf->reg18.xo16m_cap_trim; // get
//            }
//            pt_rsp_trimval(cap_val);
//        } break;
			
        default:
            break;
    }
}
__SRAMFN void flash_wr_protect(uint8_t sta)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
    // 写保护0x000-0xFFF (4KB)
    // write en singal
    fshc_en_cmd(FSH_CMD_WR_EN);
    // write state enable for write flash state
//    fshc_en_cmd(FSH_CMD_WR_STA_EN);
    // send write sta cmd
    fshc_wr_sta(FSH_CMD_WR_STA, 1, sta);

    GLOBAL_INT_RESTORE();
    
    bootDelayMs(12);       
}
void chipInit(void)
{
    ota_is_ok = 0;
    flash_wr_protect(0);
//    bootDelayMs(12);
    proto_init(PT_HOST, burner_parser);

    // auto send RSP_SYNC
//    pt_rsp_status(PT_RSP_SYNC, PT_OK);
	
	// Retrieve for Upgrade failed
	infoReset();
}
#endif
