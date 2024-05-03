#include <string.h>
#include "burner.h"
#include "proto.h"
#include "uart_itf.h"
#include "sftmr.h"
#include "leds.h"
#include "btns.h"

#include "drvs.h"
#include "regs.h"
#include "prg_api.h"
#include "chipfirm.h"

#include "pwm.h"
#include "rf_test.h"
#include "pwr_test.h"
#include "xtal_test.h"
#include "gpio_test.h"
#include "chiptest.h"

/// Global Environment
sdrvInfo_t gChnSdrv;
firmInfo_t gChnFirm;
macInfo_t  gMacInfo;
testInfo_t gTestRet;
burner_t   gBurner;

/// Retrieve from Flash
void infoReset(void)
{
    // Retrieve
    memcpy(&gChnSdrv, (uint32_t *)FSH_ADDR_SDRV_INFO, sizeof(gChnSdrv));
    memcpy(&gChnFirm, (uint32_t *)FSH_ADDR_FIRM_INFO, sizeof(gChnFirm));
    memcpy(&gMacInfo, (uint32_t *)FSH_ADDR_MAC_INFO,  sizeof(gMacInfo));

    // Zero Clean
    memset(&gTestRet, 0, sizeof(gTestRet));
    memset(&gBurner,  0, sizeof(gBurner));

//    gChnFirm.firmCFG.bConf |= BIT(FW_RESET_RUN_POS);//test --whl
    
    // Valid Check
    if (IS_NONE(gChnSdrv.sdrvLen) || IS_NONE(gChnFirm.firmCFG.Word))
    {
        burnerError(PERR_FIRM_CFG);
        return;
    }

    if (IS_NONE(gMacInfo.cntCurr))
    {
        gMacInfo.cntCurr = 0; // first run
//        gMacInfo.macCurr = gChnFirm.macCFG.nstart;
        memcpy(gMacInfo.macCurr, gChnFirm.macCFG.nstart, 6);
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

/// IO-CTRL Init & Clr
static void gpioCtrlInit(void)
{
    CSC->CSC_PIO[PIN_TX2].Word  = 0;
    CSC->CSC_PIO[PIN_RX2].Word  = 0;
    CSC->CSC_PIO[PIN_RST].Word  = 0;
    CSC->CSC_PIO[PIN_VCC_CHIP].Word  = 0;
    CSC->CSC_PIO[PIN_PWM_CHIP].Word  = 0;

    GPIO->DAT_CLR   = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST) | BIT(PIN_VCC_CHIP));
    GPIO->DIR_SET   = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_RST) | BIT(PIN_VCC_CHIP));
}

static void gpioCtrlClr(void)
{  
    CSC->CSC_PIO[PIN_TX2].Word  = 0;
    CSC->CSC_PIO[PIN_RX2].Word  = 0;
    CSC->CSC_PIO[PIN_PWM_CHIP].Word  = 0;

    GPIO->DAT_CLR  = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_PWM_CHIP) | BIT(PIN_VCC_CHIP));
    GPIO->DIR_SET  = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_PWM_CHIP) | BIT(PIN_RST));
}

/// Parser: Host-Cmd & Chip-Rsp
//extern struct rxd_buffer uart1_rxd;
//#include "uart_itf.h"
static void host_parser(struct pt_pkt *pkt, uint8_t status)
{
    if (status != PT_OK)
    {
        pt_rsp_status(pkt->code, status);
//        uart1_send((uint8_t *)&uart1_rxd, sizeof(struct rxd_buffer));
        burnerFail(PERR_UNKNOWN);        
        return;
    }
    
//    uart2_send((uint8_t *)pkt, pkt->len + PKT_HDR_SIZE);
    
    switch (pkt->code)
    {
        case PT_CMD_INFO:
        {
            pt_rsp_info(VER_BURNER, *(firmInfo_t *)FSH_ADDR_CODE_INFO, gChnFirm, gMacInfo);
        } break;

        case PT_CMD_SYNC:
        {
            burnerSet(PSTA_ONLINE, true);
            
            pt_rsp_sync(SYNC_OK_BURNER);
//            pt_rsp_sync(SYNC_OK_CHIP);
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
                leds_play(LED_OFF);
            }
            else 
            {
                pt_rsp_status(PT_RSP_ACTION, PT_OK);
                
                if (gBurner.state & PSTA_ERROR)
                {
                    burnerFail(gBurner.error);
                }
                else /*if (!BURNER_IS_BUSY())*/
                {
                    burnerOK();
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

//                if (BURNER_IS_BUSY() && (gBurner.opCode == OP_TEST_CHIP))  //100ms
//                {
//                    chipTestNext();
//                }
            }
            else
            {
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
        
        
        //CHIP
        case PT_CMD_ER_FLASH:
        {
            PKT_PARAM(struct pt_cmd_er_fsh);
            prg_flash_erase(param->mode, param->mcnt, &param->maps[0]);
            pt_rsp_status(PT_RSP_ER_FLASH, PT_OK);
        } break;

        case PT_CMD_RD_FLASH:
        {
            PKT_PARAM(struct pt_cmd_rd_fsh);
            if (gBurner.opCode != OP_ONLINE_MD)
            {
                pt_rsp_rd_flash(param->pgidx, param->length, prg_flash_paddr(param->pgidx));
            }
            else
                pt_send_cmd(pkt); // Carry to chip
            
        } break;

        case PT_CMD_WR_FLASH:
        {
            PKT_PARAM(struct pt_cmd_wr_fsh);

            // Recevied, prepare NEXT
            pt_rsp_status(PT_RSP_WR_FLASH, status);

            uint16_t len_w = pkt->len - 3;
            // Write and Vertify
            if (!prg_flash_write(param->pgidx, len_w, param->data))
            {
                pt_rsp_status(PT_RSP_WR_FLASH, PT_ERR_VERIFY);
            }
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
        
        /*Test_Cmd{{*/
//        case PT_CMD_TEST_XTAL:
//        {
//            PKT_PARAM(struct pt_cmd_test_xtal);

//            if (param->mode == XTAL_CALC)
//            {
//                xtal_freq(PIN_PWM_CHIP); // burner:PWM chip:CALC
//            }
//            pt_send_cmd(pkt);
//        } break;

//        case PT_CMD_TEST_RF:
//        {
//            PKT_PARAM(struct pt_cmd_test_rf);

//            uint8_t rfchn = param->freq;
//            
//            gChnFirm.testCFG.rfChanl = rfchn;
//            pt_cmd_test_rf(rfchn | RF_BIT_RX, RF_TEST_CNT);

//            rf_test(rfchn, RF_TEST_CNT);//burner:TX chip:RX
//        } break;

//        case PT_CMD_TEST_PWR:
//        {
//            pwr_adcInit();
//            pt_send_cmd(pkt);
//        } break;
        /*}}*/

        /*Boot_Cmd{{*/
        case PT_CMD_BAUD:
        {
            pt_rsp_status(PT_RSP_BAUDRV, PT_OK);
            WAIT_UART_IDLE(2);

            // modify chipset uart1 baudrate
            if (gBurner.state & PSTA_SYNCED)
            {
                pt_send_cmd(pkt);
                WAIT_UART_IDLE(2);
            }

            PKT_PARAM(struct pt_cmd_baud);
            UART_MODIFY_BAUD(2, param->baud);

            gChnFirm.firmCFG.uRate = param->baud / 115200;
            pt_rsp_status(PT_RSP_BAUDEND, PT_OK);
        } break;
        /*}}*/

        case PT_CMD_SRD:
        /*Flash_Cmd{{*/
//        case PT_CMD_ER_FLASH:
//        case PT_CMD_WR_FLASH:
//        case PT_CMD_RD_FLASH:
//        case PT_CMD_VF_FLASH:
//        case PT_CMD_MF_FLASH:
        /*}}*/
        case PT_CMD_TRIMVAL:
        case PT_CMD_TEST_GPIO:
        
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
    if (status != PT_OK)
    {
//        pt_rsp_status(pkt->code, status);  // USB HID Need Connected
        burnerFail(PERR_RSP);
        return;        
    }
    
    switch (pkt->code)
    {
        /* Boot: Chip Load */
        case PT_RSP_SYNC:
        {  
            chipLoadSync(pkt->payl[0]);            
        } break;

        case PT_RSP_CWR:
        case PT_RSP_BAUDEND:
        case PT_RSP_JUMP:
        { 
            chipLoadCont(pkt->code, pkt->payl[0]);
        } break;

        /* Proto: Chip Burn */
        case PT_RSP_ER_FLASH:
        case PT_RSP_WR_FLASH:
        case PT_RSP_MF_FLASH:
        {
            if (gBurner.opCode != OP_ONLINE_MD)                
            {
                chipBurnCont(pkt->code, pkt->payl[0]);
                break;
            }
        } 
        /* Proto: Chip Test */
//        case PT_RSP_TEST_GPIO:
//        case PT_RSP_TEST_XTAL:
//        case PT_RSP_TRIMVAL:
//        case PT_RSP_TEST_RF:
//        case PT_RSP_TEST_PWR:
//        {
//            chipTestCont(pkt->code, pkt->payl);            
//        } break;
        case PT_RSP_BAUDRV:
        {
            pt_code = PT_CMD_BAUD;  // wait for PT_RSP_BAUDEND(boot)
        }break;
        //case PT_RSP_VERSION:
        default:
        {
            pt_send_rsp(pkt);
        } break;
    }
}

/// Button
static void key_event(uint8_t id, uint8_t evt)
{
    if (evt == BTN_PRESS/*BTN_CLICK*/)
    {
        if (BURNER_IS_IDLE())
        {
            chipLoadStart(ACT_RUN);
        }
    }
}

/// Timer
static tmr_tk_t burner_timer(uint8_t id)
{
    if (gBurner.opCode == OP_ONLINE_MD) return 0; //
    
    uint16_t timeOut = (gBurner.opCode == OP_TEST_CHIP) ? 500 : 3; // 3*10 ms

    if ((pt_code) && ((tmr_tk_t)(sftmr_tick() - pt_time) > timeOut))
    {
        pt_code = 0;
       
        burnerFail(PERR_TIME_OUT);
    }

    return 3; // 3*10 ms
}

void burnerInit(void)
{
    //FLASH ½âËø BY NEED Delay 12ms
    flash_wr_protect(0);
    
    gpioCtrlInit();
    
//    uart2_init(BOOT_UART_BAUD);
    
    proto_init(PT_HOST, host_parser);
    proto_init(PT_CHIP, chip_parser);
    sftmr_start(10, burner_timer);

    btns_init();
    btns_conf(key_event);
    
    leds_init();
    leds_play(LED_READY);
    
    infoReset();
    
    GPIO_DAT_CLR(GPIO04);
    GPIO_DIR_SET(GPIO04);
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

            leds_play(LED_BUSY);
        } break;

        case PSTA_OK:
            bootDelayMs(20); //wait chip write flash,before close vcc
        case PSTA_FAIL:
        {
            gBurner.state &= ~PSTA_BUSY;
            gBurner.error  = detail;
            gBurner.opCode = OP_NULL;
            gBurner.opStep = 0;

            gpioCtrlClr(); 
            
            if ((state == PSTA_OK) && (gChnFirm.firmCFG.bConf & BIT(FW_RESET_RUN_POS))) //FW_RESET_RUN_POS
            {   
                bootDelayMs(20); //wait close vcc full
                GPIO->DIR_CLR = (uint32_t)(BIT(PIN_TX2) | BIT(PIN_RX2) | BIT(PIN_PWM_CHIP) | BIT(PIN_RST));
                GPIO->DAT_SET = (uint32_t)(BIT(PIN_VCC_CHIP));
            }
            
            leds_play((state == PSTA_OK) ? LED_OK : LED_FAIL);
        } break;

        case PSTA_ERROR:
        {
            gBurner.error  = detail;
            leds_play(LED_ERROR);
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
        else if (gBurner.opCode == OP_ONLINE_MD)
        {
            pt_rsp_sync(SYNC_OK_CHIP);
        }
    }

    if (opcode == OP_BURN_CHIP)
    {
        if (gChnFirm.firmCFG.tConf)
        {
//            chipTestStart();
        }
        else
        {
            //xdelay(1000*40); //wait chip write flash,before close vcc
            burnerOK();
        }
    }
}