#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "xtal_test.h"


#if (CFG_TEST)

/// CLK Output
void xtal_freq(uint8_t pad)
{
//    CSC->CSC_PIO[pad].Word = 0;
//    iom_ctrl(pad, IOM_SEL_TIMER);

//    pwm_tmr_cfg_t tmr_conf;
//    tmr_conf.psc = 159; 
//    tmr_conf.arr = 12499;
//    pwm_init(PWM_CTMR, &tmr_conf);
//    
//    pwm_chnl_cfg_t chnl_conf;
//    chnl_conf.duty = 50;
//    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT; // DMA_EN
//    chnl_conf.ccmr = PWM_CCMR_MODE1;    
//    pwm_chnl_set(PWM_CTMR_CH3, &chnl_conf); // 62.5ms
//    pwm_start(PWM_CTMR);
}

/// CLK Calculate
enum CALC_STEP
{
    CALC_INIT,
    CALC_TRIG,
    CALC_BUSY,
    CALC_END
};

uint8_t  exti_pad;
volatile uint8_t calc_step;
volatile uint8_t  xtal_tmarr;
volatile uint16_t xtal_tmcnt;
bool first_xtal_init;          // 211104 --whl
/* EXTI{{ */
enum EXTI_IRQ
{
    EXTI_IRQ_EN        = 0x01,
    EXTI_IRQ_DIS       = 0x02,
    EXTI_IRQ_CLR       = 0x04,
};

enum TRIG_MODE
{
    TRIG_RISING_EDGE   = 0x01,
    TRIG_FALLING_EDGE  = 0x02,
    TRIG_SOFTWARE_IRQ  = 0x04,
    TRIG_ADTE_ENABLE   = 0x08,
};

void extiTrigCfg(uint8_t pad, uint8_t mode)
{
    // TRIG mode
    if (mode & TRIG_RISING_EDGE)
    {
        EXTI->RTS.Word |= (1UL << pad);
    }
    if (mode & TRIG_FALLING_EDGE)
    {
        EXTI->FTS.Word |= (1UL << pad);
    }
    if (mode & TRIG_SOFTWARE_IRQ)
    {
        EXTI->SWI.Word |= (1UL << pad);
    }
    if (mode & TRIG_ADTE_ENABLE)
    {
        EXTI->ADTE.Word |= (1UL << pad);
    }

    // INT conf
    EXTI->ICFG.Word |= (1UL << pad);
}

void extiTrigIrq(uint8_t pad, uint8_t mode)
{
    if (mode & EXTI_IRQ_CLR)
        EXTI->ICR.Word |= (1UL << pad);
    if (mode & EXTI_IRQ_DIS)
        EXTI->IDR.Word |= (1UL << pad);
    if (mode & EXTI_IRQ_EN)
        EXTI->IER.Word |= (1UL << pad);
}
/*}}*/

/*IRQ_Hdl{{*/
void EXTI_IRQHandler(void)
{
    uint32_t irq_sta = EXTI->RIF.Word;

    if (irq_sta & (1UL << exti_pad))
    {
        //extiTrigIrq(exti_pad, EXTI_IRQ_DIS);
        EXTI->IDR.Word |= (1UL << exti_pad);
        EXTI->ICR.Word |= (1UL << exti_pad);

        if (calc_step == CALC_INIT) // Clear First half-EXTI
        {
            calc_step = CALC_TRIG;
            
            xtal_tmarr = 0; // 211103 --whl

            EXTI->IER.Word |= (1UL << exti_pad); // EXTI_IRQ_EN
        }
        else if (calc_step == CALC_TRIG)
        {
            calc_step = CALC_BUSY;

            CTMR1->CR1.CEN = 0;
            xtal_tmarr = 0; //xtal_tmcnt = 0;
            CTMR1->CNT = 0;
            CTMR1->CR1.CEN = 1; // Enable tmr

            EXTI->IER.Word |= (1UL << exti_pad); // EXTI_IRQ_EN
        }
        else if (calc_step == CALC_BUSY)
        {
            calc_step = CALC_END;

            CTMR1->CR1.CEN = 0; // Diable tmr
            xtal_tmcnt = CTMR1->CNT;
        }
    }
}

void CTMR1_IRQHandler(void)
{
    uint32_t irq_sta = CTMR1->RIF.Word;

    if (irq_sta & 0x01/*TIM_UI_IRQ_BIT*/)
    {
        //CTMR3->IDR.UI = 1;        
        xtal_tmarr++;
        CTMR1->ICR.UI = 1;        
        //CTMR3->IER.UI = 1;
    }
}
/*}}*/

static void xtal_init(uint8_t pad)
{
    exti_pad  = pad%8;
    calc_step = first_xtal_init ? CALC_INIT : CALC_TRIG; // 211104 --whl
    xtal_tmarr = 0; //xtal_tmcnt = 0;

    // timInit
    CTMR1->CR1.CEN = 0;
    CTMR1->PSC = 0;
    CTMR1->ARR = 59999;//63999; // 4ms = 64K/16M
    CTMR1->CR1.ARPE = 1;
    CTMR1->EGR.UG = 1;
    CTMR1->CR1.DIR = 0; // Counter used as upcounter
    CTMR1->CR1.CMS = 0; // Edge-aligned mode
    CTMR1->CNT = 0x00;
    CTMR1->ICR.UI = 1;
    CTMR1->IER.UI = 1;
    NVIC_EnableIRQ(CTMR1_IRQn);

    // IO Mod: input pull up
    CSC->CSC_PIO[pad].Word = 0x180;
    // Rising&Falling edge trigger enable
//    extiTriggerEdgeSel(EXTI_Trigger_Falling, pad, 1); 
    EXTI->RTS.Word = 1 << exti_pad;
    EXTI->FTS.Word = 1 << exti_pad;
    // DAT_CLR&EN EXTI_IRQ
//    extiTrigIrq(pad, EXTI_IRQ_CLR | EXTI_IRQ_EN);
    extiITStaClear(pad);
    extiITEnable(pad);
    extiITCngfig(pad);
    
    NVIC_EnableIRQ(EXTI_IRQn);

    CTMR1->CR1.CEN = 1;
    first_xtal_init = false; 
}

static void xtal_deinit(void)
{
    CTMR1->CR1.CEN = 0;

    NVIC_DisableIRQ(CTMR1_IRQn);
    NVIC_DisableIRQ(EXTI_IRQn);
}

static uint32_t xtal_count(uint8_t pad)
{
    xtal_init(pad);

    while (calc_step != CALC_END)
    {
        if (xtal_tmarr > TIM_ARR_STD * 2)
        {
            // None EXTI Irq
            xtal_deinit();
            return 0;
        }
    };

    xtal_deinit();
    return (((uint32_t)xtal_tmarr << 16) | xtal_tmcnt);
}

uint32_t xtal_calc(uint8_t pad)
{
    uint8_t cap_val  = 0x20; // rang=0~63
    uint8_t cap_step = 0x10;
    uint32_t tmr_val;
    
    first_xtal_init = true;
    
    do
    {
        // Set cap_trim
        AON->XOSC16M_CTRL.XOSC16M_CAP_TR = cap_val;
        // Calc to judge
        tmr_val = xtal_count(pad);
        if (((tmr_val > TIM_VAL_MIN) && (tmr_val < TIM_VAL_MAX)) || (tmr_val == 0))
        {
            // Found or Error
            break;
        }

        // next half-step
        if (tmr_val > TIM_VAL_STD)
            cap_val += cap_step; // freq less std.
        else
            cap_val -= cap_step; // freq more std.

        cap_step >>= 1;
    }
    while (cap_step != 0);
    
    // if need
    #if (1)
    cap_val += 9;
    
    if (cap_val > 0x3F)
    {
        tmr_val = 0xFFFFFF;
    }
    #endif
    
    return ((uint32_t)cap_val << 24) | tmr_val;
}

#endif //(CFG_TEST)
