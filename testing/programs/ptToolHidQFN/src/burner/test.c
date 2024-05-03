#include <string.h>
#include "burner.h"
#include "proto.h"
#include "prg_api.h"
#include "xtal_test.h"
#include "gpio_test.h"
#include "pwr_test.h"
#include "rf_test.h"
#include "b6x.h"

//#define TIM_CNT_STD 0x00109C40
#define RF_TEST_CNT 0x64  //100

//enum test_step
//{
//    WORKCURR,
//    XTALVAL,
//    GPIOMSK,
//    RFCHAN,
//};

test_result_t test_result;

void test_run(void)
{
    switch (burner.stepBit)
    {
        case WORKCURR:
        {
            pwr_adcInit();
            pt_cmd_test_pwr(1);
        } break;

        case GPIOMSK:
        {
            uint8_t modes = IO_TEST_SHORTGND | IO_TEST_SHORTVDD | IO_TEST_NEAR;
            pt_cmd_test_gpio(gFirmInfo.gpioMsk, modes);
        } break;

        case XTALVAL:
        {
            xtal_freq(PIN_PWM_CHIP);//burner:PWM chip:CALC
            pt_cmd_test_xtal(XTAL_CALC, gFirmInfo.xtalPio); //PIN_PWM_CHIP
        } break;

        case RFCHAN:
        {
            if (burner.state & BURNER_ON_LINE)
            {
                //@SEE PT_CMD_BURN_STA BATCH
            }
            else
            {
                pt_cmd_test_rf(gFirmInfo.rfChan | 0x80, RF_TEST_CNT);
                rf_test(gFirmInfo.rfChan, RF_TEST_CNT); //burner:TX chip:RX
            }
        } break;

        default:
            break;
    }

    burner.stepBit++;
}

void test_start(void)
{
    if (burner.opCode == OP_TEST)
    {
        for (uint8_t test_i = burner.stepBit; test_i < 4; test_i++)
        {
            if ((gFirmInfo.testCfg >> test_i) & 0x01)
            {
                burner.stepBit = test_i;

                test_run();

                return;
            }
        }
    }

    setBurnerState(BURNER_OK, ER_NULL);
}

void test_workcurr_rsp(uint16_t result)
{
    test_result.workcurr = result;

    uint8_t workcurr = (uint8_t)(((test_result.workcurr * 3300) / 0xFFF)); //mV

    if (workcurr <= (gFirmInfo.workCurr * 10))
    {
        test_start();
    }
    else
    {
        setBurnerState(BURNER_FAIL, ER5_TEST_CURR);
    }
}

void test_xtal_rsp(uint32_t freq)
{
    test_result.freq = freq & 0xFFFFFF;

    uint32_t value = ((test_result.freq >= TIM_CNT_STD) ? (test_result.freq - TIM_CNT_STD) : TIM_CNT_STD - test_result.freq);

    //    value <<= 2; //Hz

    if (value <= (gFirmInfo.xtalVal * 1000) / 150) //kHz -> Hz
    {
        pt_cmd_wr_trim(((freq >> 24) & 0xFF) | 0x80);
    }
    else
    {
        setBurnerState(BURNER_FAIL, ER6_TEST_XTAL);
    }
}

void test_trim_rsp(void)
{
    test_start();
}

void test_rf_rsp(uint8_t result)
{
    if (test_result.rfRxRec == 0)
    {
        //        rf_test(0, 0);//close burner:TX

        test_result.rfRxRec = result;

        if (test_result.rfRxRec > 17 && test_result.rfRxRec < 21 /*test_result.rfRxRec == RF_TEST_CNT*/)
        {
            //            test_run();
            pt_cmd_test_rf(gFirmInfo.rfChan, RF_TEST_CNT);

            send_cmd_flag = false;  //test

            test_result.rfRxRec = rf_test(gFirmInfo.rfChan | 0x80, RF_TEST_CNT);

            //            rf_test(80, 0);//close burner:RX
            //            pt_cmd_test_rf(0, 0);//close chip:TX cmd

            if (test_result.rfRxRec > 17 && test_result.rfRxRec < 21 )
            {

            }
            else
            {
                setBurnerState(BURNER_FAIL, ER8_TEST_RF);
            }
        }
        else
        {
            setBurnerState(BURNER_FAIL, ER8_TEST_RF);
        }
    }
    else
    {
        test_start();
    }
}

void test_gpio_rsp(uint8_t result)
{
    test_result.gpioRes = result;

    if (test_result.gpioRes == IO_TEST_PASS)
    {
        test_start();
    }
    else
    {
        setBurnerState(BURNER_FAIL, ER7_TEST_GPIO);
    }
}

void get_test_result(void)
{


}

