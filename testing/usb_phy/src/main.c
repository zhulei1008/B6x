/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "regs.h"


/*
 * DEFINES
 ****************************************************************************************
 */

// USB ITF pad
#define PA_USB_DP          4
#define PA_USB_DM          5
#define PA_USB_SOF         15

// USB PHY pad
#define PA_USB_DIP_PHY     2
#define PA_USB_DIM_PHY     3
#define PA_USB_DIDIF_PHY   8
#define PA_USB_DOP_PHY     9
#define PA_USB_DOM_PHY     10
#define PA_USB_NDOE_PHY    11
#define PA_USB_PU_PHY      14

// GPIOs of USB, BIT(PA_USB...)
#define GPIO_USB_ALL       0x4F3C


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void usbInit(void)
{
    #if (DBG_MODE)
    // IO Test USB+PHY
    GPIO_DAT_CLR(GPIO_USB_ALL);
    GPIO_DIR_SET(GPIO_USB_ALL); // output enable
    
	GPIO_DAT_SET(GPIO_USB_ALL);
    GPIO_DAT_CLR(GPIO_USB_ALL); // one pulse
    
    GPIO_DIR_CLR(GPIO_USB_ALL); // output disable
    #endif
     
    // HiZ(DP DM) as USB Interface
    gpio_set_hiz(PA_USB_DP);
    gpio_set_hiz(PA_USB_DM);
	// Enable usb pull-up 1.5K for DP(1:1.7mA 2:2.4mA 3:2.5mA)
    SYSCFG->USB_CTRL.DIG_USB_PU = 1;

    // USB SOF signal observe if need
    iom_ctrl(PA_USB_SOF, IOM_SEL_SPECL);
    
    // USB PHY function
    iom_ctrl(PA_USB_DIP_PHY,   IOM_SEL_USB);
    iom_ctrl(PA_USB_DIM_PHY,   IOM_SEL_USB);
    iom_ctrl(PA_USB_DIDIF_PHY, IOM_SEL_USB);
    iom_ctrl(PA_USB_DOP_PHY,   IOM_SEL_USB | IOM_INPUT);
    iom_ctrl(PA_USB_DOM_PHY,   IOM_SEL_USB | IOM_INPUT);    
    iom_ctrl(PA_USB_NDOE_PHY,  IOM_SEL_USB | IOM_INPUT);
    iom_ctrl(PA_USB_PU_PHY,    IOM_SEL_USB | IOM_INPUT);
    // Enable usb phy mode
    SYSCFG->USB_CTRL.USB_PHY_MOD = 1;
}

static void sysInit(void)
{
    // clk enable
    RCC_APBCLK_EN(APB_RF_BIT  | APB_UART1_BIT);
    RCC_AHBCLK_EN(AHB_USB_BIT | AHB_SYSCFG_BIT);
    
    // flash clk switch to xo16m
    rcc_fshclk_set(FSH_CLK_HSE16);
    
    // USB work in 48MHz
    rcc_sysclk_set(SYS_CLK_48M);
}

int main(void)
{
    sysInit();
    
    usbInit();

    while(1);
}
