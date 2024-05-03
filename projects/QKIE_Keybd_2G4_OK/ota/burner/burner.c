#include <string.h>
#include "burner.h"
#include "proto.h"
#include "ota_itf.h"
#include "sftmr.h"
#include "led.h"
#include "btn.h"

#include "reg_gpio.h"
#include "reg_iopad.h"
#include "chipfirm.h"
#include "prg_api.h"
#include "app.h"
#include "gapc_api.h"
#include "dbg.h"

#define BURNER_MASTER    1
/// Global Environment
sdrvInfo_t gChnSdrv;
firmInfo_t gChnFirm;
macInfo_t  gMacInfo;
testInfo_t gTestRet;
burner_t   gBurner;
retran_t   retran; //Retransmission

/// Retrieve from Flash
static void infoReset(void)
{
    // Retrieve
    memcpy(&gChnFirm, (uint32_t *)FSH_ADDR_FIRM_INFO, sizeof(gChnFirm));
    memcpy(&gMacInfo, (uint32_t *)FSH_ADDR_MAC_INFO,  sizeof(gMacInfo));

    // Zero Clean
//    memset(&gTestRet, 0, sizeof(gTestRet));
    memset(&gBurner,  0, sizeof(gBurner));
    	
    // Valid Check
    if (IS_NONE(gMacInfo.cntCurr))
    {
        gMacInfo.cntCurr = 0; // first run
        gMacInfo.macCurr = gChnFirm.macCFG.nstart;
    }

    if (gChnFirm.firmCFG.bConf & FWCFG_EN(BURN_MAC))
    {
        if ((gChnFirm.macCFG.ncount > 0) && (gChnFirm.macCFG.ncount <= gMacInfo.cntCurr))
        {
            burnerError(PERR_MAC_END);
            return;
        }
    }
}

/// Parser: Host-Cmd & Chip-Rsp
//extern struct rxd_buffer uart1_rxd;
//#include "uart_itf.h"
bool down_flag = false;
static void host_parser(struct pt_pkt *pkt, uint8_t status)
{
    if (status != PT_OK)
    {
        pt_rsp_status(pkt->code, status);
//        uart1_send((uint8_t *)&uart1_rxd, sizeof(struct rxd_buffer));
        burnerFail(PERR_UNKNOWN);        
        return;
    }

    switch (pkt->code)
    {
        case PT_CMD_VERSION:
        {
            pt_rsp_version(VER_BURNER);
        } break;

        case PT_CMD_SYNC:
        {
            burnerSet(PSTA_ONLINE, true);
            pt_rsp_sync(SYNC_OK_BURNER);
        } break;

        case PT_CMD_FWCFG:
        {
            burnerSet(PSTA_ONLINE, true);
            pt_rsp_fwcfg(&gChnFirm, &gMacInfo);
        } break;

        case PT_CMD_ACTION:
        {        
            PKT_PARAM(struct pt_cmd_action); 
            
            if (param->mode == ACT_OFF)
            {
//                ledPlay(LED_OFF);
            }
            else 
            {
                pt_rsp_status(PT_RSP_ACTION, PT_OK);
                
                if (gBurner.state & PSTA_ERROR)
                {
                    burnerFail(gBurner.error);
                }
                else if (!BURNER_IS_BUSY())
                {
                    down_flag = true;
                    chipLoadStart(param->mode);
                }
            }            
        } break;

        case PT_CMD_CHSTA:
        {
            PKT_PARAM(struct pt_cmd_chsta);
            
            if (param->chan < 8)
            {              
                pt_rsp_chsta(param->chan, &gBurner.state);

                if (BURNER_IS_BUSY() && (gBurner.opCode == OP_TEST_CHIP))  //100ms
                {
//                    chipTestNext();
                }
            }
            else
            {
//                uart1_send((uint8_t *)&uart1_rxd, sizeof(struct rxd_buffer));
                burnerFail(PERR_UNKNOWN);
            } 
            
        } break;

        case PT_CMD_CHRET:
        {
            PKT_PARAM(struct pt_cmd_chret);
            
            if (param->chan < 8)
            {
                pt_rsp_chret(param->chan, &gMacInfo, &gTestRet);
            }
            else
            {
//                uart1_send((uint8_t *)&uart1_rxd, sizeof(struct rxd_buffer));
                burnerFail(PERR_UNKNOWN);
            }            
        } break;

        /*Firm Cmd{{*/
        case PT_CMD_ER_FIRM:
        {
            PKT_PARAM(struct pt_cmd_er_fsh);
            
            prg_flash_erase(param->mode, param->mcnt, &param->maps[0]);
            pt_rsp_status(PT_RSP_ER_FIRM, PT_OK);
        } break;

        case PT_CMD_RD_FIRM:
        {
            PKT_PARAM(struct pt_cmd_rd_fsh);

            pt_rsp_rd_firm(param->pgidx, param->length, prg_flash_paddr(param->pgidx));
        } break;

        case PT_CMD_WR_FIRM:
        {
            PKT_PARAM(struct pt_cmd_wr_fsh);

            pt_rsp_status(PT_RSP_WR_FIRM, PT_OK); // Recevied, prepare NEXT

            if (!prg_flash_write(param->pgidx, pkt->len - 3, param->data))
            {
                pt_rsp_status(PT_RSP_WR_FIRM, PT_ERR_VERIFY); // Write & Vertify
            }
        } break;

        case PT_CMD_VF_FIRM:
        {
            PKT_PARAM(struct pt_cmd_vf_fsh);

            if (prg_flash_verify(param->pgidx, pkt->len - 3, param->data))
                pt_rsp_status(PT_RSP_VF_FIRM, PT_OK);
            else
                pt_rsp_status(PT_RSP_VF_FIRM, PT_ERR_VERIFY);
        } break;

        case PT_CMD_MF_FIRM:
        {
            PKT_PARAM(struct pt_cmd_mf_fsh);

            if (prg_flash_modify(param->addr, pkt->len - 5, param->data))
                pt_rsp_status(PT_RSP_MF_FIRM, PT_OK);
            else
                pt_rsp_status(PT_RSP_MF_FIRM, PT_ERR_VERIFY);
        } break;
        /*}}*/

        /*Test_Cmd{{*/

        /*}}*/

        case PT_CMD_SRD:
        /*Flash_Cmd{{*/
        case PT_CMD_ER_FLASH:
        case PT_CMD_WR_FLASH:
        case PT_CMD_RD_FLASH:
        case PT_CMD_VF_FLASH:
        case PT_CMD_MF_FLASH:
        /*}}*/       
        /*OTP_Cmd{{*/
        /*}}*/       
        case PT_CMD_TRIMVAL:
        default:
        {
            pt_send_cmd(pkt); // Carry to chip
        } break;
    }
}

static void chip_parser(struct pt_pkt *pkt, uint8_t status)
{
    pt_code = 0;

//    pt_send_rsp(pkt); // debug
    if ((status != PT_OK) || (pkt->payl[0] != PT_OK))
    {
//        pt_rsp_status(pkt->code, status);
		retran.cnt++;
	
		if (retran.cnt < PKT_RETRAN_MAX)
			pt_send_cmd((pkt_t *)&retran.buff);
		else
			burnerFail(PERR_RSP);
		
        return;        
    }
	
    retran.cnt = 0;
	
    switch (pkt->code)
    {
        /* Chip Firm */
        case PT_RSP_SYNC:
        {              
			chipLoadSync(pkt->payl[0]);            
        } break;
		
		case PT_RSP_RST:
        /* Proto: Chip Burn */		
        case PT_RSP_ER_FLASH:
        case PT_RSP_WR_FLASH:
        case PT_RSP_MF_FLASH:	
        /* Proto: Chip OTP*/
        case PT_RSP_WR_OTP:   
        {
			chipBurnCont(pkt->code, pkt->payl[0]);
        }  break;
        
        /* Proto: Chip Test */
        //case PT_RSP_VERSION:
        default:
        {
            pt_send_rsp(pkt);
        } break;
    }
}

/// Button

/// Timer
static uint32_t burner_timer(uint8_t id)
{
    if (gBurner.opCode == OP_ONLINE_MD) return 0; //
    
    uint16_t timeOut = 2000; //TEST:5s FLASH:3*10 ms OTP:226ms

    if ((pt_code) && ((uint32_t)(currTickCnt() - pt_time) > timeOut))
    {
        pt_code = 0;

        burnerFail(PERR_TIME_OUT);
    }

    return 30; // 30*10 ms
}

void burnerInit(void)
{
    proto_init(PT_HOST, host_parser);
    proto_init(PT_CHIP, chip_parser);
    sfTmrStart(10, burner_timer);
       
    ledInit(LED_IDLE);

    infoReset();
}


void burnerSet(uint8_t state, uint8_t detail)
{
    if (state > PSTA_ERROR)
    {
        // Set|Clear Bit
        if (detail)
            gBurner.state |= state;
        else
            gBurner.state &= ~state;
        return;
    }

    if ((gBurner.state & state) == state)
    {
        return; // repeat state
    }

    gBurner.state |= state;

    switch (state)
    {
        case PSTA_BUSY:
        {
            gBurner.state &= ~(PSTA_OK | PSTA_FAIL);
            gBurner.opCode = detail;
            gBurner.opStep = 0;

            ledPlay(LED_OTA);
        } break;

        case PSTA_OK:
//            xdelay(1000*40); //wait chip write flash,before close vcc
//			debug("OTA_OK\r\n");
        case PSTA_FAIL:
        {
            gBurner.state &= ~PSTA_BUSY;
            gBurner.error  = detail;
            gBurner.opCode = OP_NULL;
            gBurner.opStep = 0;
                       
//            if ((state == PSTA_OK) && (gChnFirm.firmCFG.bConf & BIT(FW_RESET_RUN_POS))) //FW_RESET_RUN_POS
//            {   

//            }
			if(app_env.state == APP_CONNECTED)  gapc_disconnect(TASK_ID(GAPC, app_env.curidx));
			
<<<<<<< .mine
            if (state == PSTA_OK)
            {
                debug("OTA_OK\r\n");
                ledPlay(LED_OK);
            }
            else
            {
                debug("OTA_Fail\r\n");
                ledPlay(LED_FAIL);
            }
||||||| .r2573
            ledPlay((state == PSTA_OK) ? LED_OK : LED_FAIL);
			
            debug("OTA_Fail\r\n");
=======
			if (state == PSTA_OK)
			{
				debug("OTA_OK\r\n");
				ledPlay(LED_OK);
			}
			else
			{
				debug("OTA_Fail\r\n");
				ledPlay(LED_FAIL);
			}
			
//            ledPlay((state == PSTA_OK) ? LED_OK : LED_FAIL);			
//            debug("OTA_Fail\r\n");
//			debug(state == PSTA_OK ? "OTA_OK\r\n" : "OTA_Fail\r\n");
			
>>>>>>> .r2579
        } break;

        case PSTA_ERROR:
        {
            gBurner.error  = detail;
            ledPlay(LED_ERROR);
        } break;

        default:
            break;
    }
}

void burnerEvt(uint8_t state, uint8_t opcode)
{
    if (opcode == OP_LOAD_SDRV)
    {
        burnerSet(PSTA_SYNCED, true);
        if (gBurner.opCode == OP_BURN_CHIP)
        {
            chipBurnStart();
        }
    }

    if (opcode == OP_BURN_CHIP)
    {
		//xdelay(1000*40); //wait chip write flash,before close vcc
		
		burnerOK();
    }
}
