#include <string.h>
#include "burner.h"
#include "proto.h"
#include "prg_api.h"

#include "b6x.h"
#include "pwm.h"
#include "chiptest.h"
#include "xtal_test.h"
#include "gpio_test.h"
#include "pwr_test.h"
#include "rf_test.h"


#define PWR_MODE           (CURR_WORK)
#define GPIO_MODE          (IO_TEST_SHORTGND | IO_TEST_SHORTVDD | IO_TEST_NEAR)
#define ONLINE_MODE        (gBurner.opCode == OP_ONLINE_MD)

void test_run(uint8_t status)
{
    uint8_t idx = gBurner.opStep;
    if (status == PERR_NO_ERROR)
    {
        idx++; // try to next
    }

    for (; idx < TEST_MAX; idx++)
    {
        if (gChnFirm.firmCFG.tConf & (1 << idx)) // find next step
            break;
    }

    if (idx >= TEST_MAX)
    {
        burnerOK(); // finish
        return;
    }
    gBurner.opStep = idx;

    switch (idx)
    {
        case TEST_PWR:
        {
            pwr_adcInit();
            pt_cmd_test_pwr(PWR_MODE);
        } break;

        case TEST_GPIO:
        {
            pt_cmd_test_gpio(gChnFirm.testCFG.gpioMsk, GPIO_MODE);
        } break;

        case TEST_XTAL:
        {
            xtal_freq(PIN_PWM_CHIP);//burner:PWM chip:CALC
            pt_cmd_test_xtal(XTAL_CALC, gChnFirm.testCFG.xtalPio); //PIN_PWM_CHIP
        } break;

        case TEST_RF:
        {
            if (!(gBurner.state & PSTA_ONLINE))
            {
                uint8_t rfchn = gChnFirm.testCFG.rfChanl;

                pt_cmd_test_rf(rfchn | RF_BIT_RX, RF_TEST_CNT);
                rf_test(rfchn, RF_TEST_CNT); //burner:TX chip:RX
            }
            else
            {
                // move to BATCH Start @see PT_CMD_CHSTA
            }
        } break;

        default:
            break;
    }
}

static uint8_t test_ret_gpio(uint8_t result)
{
    if (ONLINE_MODE) 
    {
        pt_rsp_status(PT_RSP_TEST_GPIO, result);
        return PERR_XX_STATE;
    }
    
    gTestRet.gpioRes = result;

    return (result == IO_TEST_PASS) ? PERR_NO_ERROR : PERR_TEST_GPIO;
}

static uint8_t test_ret_pwr(void)
{
    uint16_t value = pwr_adcGet();
    
    if (ONLINE_MODE)
    {
        pt_rsp_pwr_value(value);
        return PERR_XX_STATE;
    }   
    //pt_rsp_pwr(value);
    gTestRet.pwrVadc = value;
    //Not Float need to multiply by 10.  211108 --whl
    return (ADC_TO_CURR(value, 1) <= (gChnFirm.testCFG.pwrCurr * 10)) ? PERR_NO_ERROR : PERR_TEST_PWR;
}

static uint8_t test_ret_xtal(void *value)
{
    pwmStop(CTMR3, PWM_CH_D);

    if (ONLINE_MODE)
    {
        pt_rsp_xtal_calc(*(uint32_t *)value);
        return PERR_XX_STATE;
    }
    
    struct xtal_val *pVal = (struct xtal_val *)value;

    gTestRet.xtalCap = pVal->trim;
    
    if (pVal->arr == TIM_ARR_STD)
    {
        gTestRet.xtalCnt = pVal->cnt;

        uint16_t offset = (pVal->cnt >= TIM_CNT_STD) ? (pVal->cnt - TIM_CNT_STD) : (TIM_CNT_STD - pVal->cnt);
        if (offset <= KHZ_TO_XTAL(gChnFirm.testCFG.xtalVal))
        {
            pt_cmd_trimval(pVal->trim | 0x80);
            return PERR_XX_STATE; // goto trim
        }
    }
    else
    {
        gTestRet.xtalCnt = 0;// invalid
    }

    return PERR_TEST_XTAL;
}

static uint8_t test_ret_rf(uint8_t result)
{   
    if (result == RF_BIT_TX_RET) //rf test over
    {
        if ((gTestRet.rfRxRec >= RF_PKT_RES_MIN) && (gTestRet.rfRxRec <= RF_PKT_CNT))
        {
            return PERR_NO_ERROR;
        }
        
        return PERR_TEST_RF;
    }
    
    gTestRet.rfRxRec = result;  //chip rfRxRec result

    if ((result >= RF_PKT_RES_MIN) && (result <= RF_PKT_CNT))
    {
        uint8_t rfchn = gChnFirm.testCFG.rfChanl;

        pt_cmd_test_rf(rfchn, RF_TEST_CNT); // chip:TX burner:RX
        gTestRet.rfRxRec = rf_test(rfchn | RF_BIT_RX, RF_TEST_CNT);
        
        return PERR_XX_STATE; //Wait second rsp to clear opcode        
    }
  
    return PERR_TEST_RF;
}


void chipTestStart(void)
{
    gBurner.opCode = OP_TEST_CHIP;
    gBurner.opStep = 0;
    memset(&gTestRet, 0, sizeof(gTestRet));
    test_run(1);
}

void chipTestNext(void)
{
    if ((gBurner.opCode == OP_TEST_CHIP) && (gBurner.opStep == TEST_RF))
    {
//        gBurner.opStep++;
        uint8_t rfchn = gChnFirm.testCFG.rfChanl;

        pt_cmd_test_rf(rfchn | RF_BIT_RX, RF_TEST_CNT);
        rf_test(rfchn, RF_TEST_CNT); //burner:TX chip:RX
    }
}

void chipTestCont(uint8_t rsp, uint8_t *payl)
{
    uint8_t status = PERR_NO_ERROR;   
    if (payl == NULL) return;

    switch (rsp)
    {
        case PT_RSP_TEST_GPIO:
        {
            status = test_ret_gpio(((struct pt_rsp_test_gpio *)payl)->modes);                     
        } break;

        case PT_RSP_TEST_XTAL:
        {
            status = test_ret_xtal(payl);         
        } break;

        case PT_RSP_TRIMVAL:
        {
            // next step
        } break;

        case PT_RSP_TEST_RF:
        {
            uint8_t  result = ((struct pt_rsp_test_rf *)payl)->result;
                      
            status = test_ret_rf(result);
            
            if (ONLINE_MODE)
            {                
                result != RF_BIT_TX_RET ? pt_rsp_status(PT_RSP_TEST_RF, result) : pt_rsp_status(PT_RSP_TEST_RF, gTestRet.rfRxRec);
                status = PERR_XX_STATE;
            } 
                       
        } break;

        case PT_RSP_TEST_PWR:
        {                      
            status = test_ret_pwr();  
        } break;

        default:
            break;
    }               
    
    if (status >= PERR_TEST_FAIL)
    {
        burnerFail(status); // Fail to exit
    }
    else if (status == PERR_NO_ERROR)
    {
        test_run(status);
    }
}
