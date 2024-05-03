#include <string.h>
#include "b6x.h"
#include "drvs.h"

#include "chipset.h"
#include "proto.h"
#include "uart_itf.h"

#include "prg_api.h"
#include "flash.h"
#include "rf_test.h"
#include "pwr_test.h"
#include "xtal_test.h"
#include "gpio_test.h"
#include "..\src\trim.h"
#include "trim_test.h"



/// Burner Cmd Parser
static void burner_parser(struct pt_pkt *pkt, uint8_t status)
{
    if (status != PT_OK)
    {
        pt_send_rsp(pkt); // debug
        pt_rsp_status(pkt->code, status);
        return;
    }

    switch (pkt->code)
    {
        case PT_CMD_INFO:
        {
            pt_rsp_info(VER_CHIPSET, *(firmInfo_t *)FSH_ADDR_CODE_INFO, *(firmInfo_t *)FSH_ADDR_CODE_INFO, *(macInfo_t *)FSH_ADDR_CODE_INFO);
        } break;

        case PT_CMD_SYNC:
        {
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

        case PT_CMD_RST:
        {
            pt_rsp_status(PT_RSP_RST, PT_OK);
            WAIT_UART_IDLE(1);

            NVIC_SystemReset();
        } break;
#endif

#if (PT_FLASH_CMD) /* Flash */
        case PT_CMD_ER_FLASH:
        {
            PKT_PARAM(struct pt_cmd_er_fsh);
            prg_flash_erase(param->mode, param->mcnt, &param->maps[0]);
            pt_rsp_status(PT_RSP_ER_FLASH, PT_OK);
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
            
//            GPIO_DAT_SET(GPIO13);
            // Recevied, prepare NEXT
            pt_rsp_status(PT_RSP_WR_FLASH, status);
//            GPIO_DAT_CLR(GPIO13);
//            GPIO_DAT_SET(GPIO13);
            
            uint16_t len_w = pkt->len - 3;
            // Write and Vertify
            if (!prg_flash_write(param->pgidx, len_w, param->data))
            {
                
                pt_rsp_status(PT_RSP_WR_FLASH, PT_ERR_VERIFY);
                
            }
//            GPIO_DAT_CLR(GPIO13);
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

            if (prg_flash_modify(param->addr, pkt->len - 5, param->data))
                pt_rsp_status(PT_RSP_MF_FLASH, PT_OK);
            else
                pt_rsp_status(PT_RSP_MF_FLASH, PT_ERR_VERIFY);
        } break;
#endif

        case PT_CMD_TRIMVAL:
        {
            PKT_PARAM(struct pt_cmd_mf_fsh);
            
            if (param->addr == 0x18000FDC)
            {
                if (RD_32(0x18000FF4) == TRIM_VALID_VALUE)
                {
                   uint32_t data[TRIM_VAL_NUM];
                    
                   memcpy(data, prg_flash_addr(0xFDC), sizeof(TRIM_Typedef)); 
                    
                   fshc_erase(FSH_TRIM_ADDR & 0xFFFF00, FSH_CMD_ER_PAGE);
                    
                   prg_flash_write(((FSH_TRIM_ADDR >> 8) & 0xFFFF), sizeof(TRIM_Typedef), (uint8_t *)data);
                                       
                   pt_rsp_status(PT_RSP_TRIMVAL, PT_OK);                
                }
                else
                {
                    pt_rsp_status(PT_RSP_TRIMVAL, PT_ERR_STATUS);   
                }
            }
            else
            {
                if (prg_flash_modify(param->addr, pkt->len - 5, param->data))
                    pt_rsp_status(PT_RSP_TRIMVAL, PT_OK);
                else
                    pt_rsp_status(PT_RSP_TRIMVAL, PT_ERR_VERIFY);               
            }

            
//            if (RD_32(FSH_TRIM_ADDR + 20) != TRIM_VALID_VALUE)
//            {
//                uint32_t trim_valid_value = TRIM_VALID_VALUE;
//                if (!prg_flash_modify(FSH_TRIM_ADDR + 20, 4, (uint8_t *)&trim_valid_value))
//                {
//                    pt_rsp_status(PT_RSP_WR_FLASH, PT_ERR_VERIFY);
//                }
//            }
        } break;

            /* Test */
#if (PT_TEST_CMD)
        case PT_CMD_TEST_GPIO:
        {
            PKT_PARAM(struct pt_cmd_test_gpio);
            uint8_t state = 0;//gpio_test(&param->masks, param->modes);

            pt_rsp_status(PT_RSP_TEST_GPIO, state);
        } break;

        case PT_CMD_TEST_XTAL:
        {
            PKT_PARAM(struct pt_cmd_test_xtal);
            {
                uint32_t freq = 0;//xtal_calc(param->gpio);
                pt_rsp_xtal_calc(freq);
            }
        } break;

        case PT_CMD_TEST_RF:
        {
            PKT_PARAM(struct pt_cmd_test_rf);
            uint8_t result = rf_test(param->freq, param->cont);

            pt_rsp_status(PT_RSP_TEST_RF, result);
        } break;

        case PT_CMD_TEST_PWR:
        {
            //PKT_PARAM(struct pt_cmd_test_pwr);
            //pwr_enterMode(param->mode);
            pt_rsp_status(PT_RSP_TEST_PWR, PT_OK);
        } break;
        
        case PT_CMD_TEST_TRIM:
        {
            PKT_PARAM(struct pt_cmd_test_trim);
            uint32_t result = ldo_trim_test(param->ft_cmd, param->val);

            pt_rsp_test_trim(param->ft_cmd, result);
        } break;        
#endif
        default:
            break;
    }
}

void chipInit(void)
{
    //FLASH ½âËø BY NEED Delay 12ms
    flash_wr_protect(0);
    
    proto_init(PT_HOST, burner_parser);

    // auto send RSP_SYNC
    pt_rsp_status(PT_RSP_SYNC, PT_OK);
}
