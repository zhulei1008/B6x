#include "b6x.h"
#include "regs.h"
#include "drvs.h"
#include "cfg.h"
#include "ft_gpio.h"
#include "ft_test.h"

#define GPIOS_DEFAULT_MOD   0x00 //PA.
#define GPIOS_IE_MOD        0x100 //PA.
#define GPIOS_HIGHZ_MOD     0x00 //PA.

#ifdef CHIP_QFN32
#define ALL_GPIOS         	0xFFF3F //0xFFF3F //(except 6,7)
#define ALL_ODD_GPIOS	  	0xAAA2A //PA1,3, ,....19(except 6,7)
#define ALL_EVEN_GPIOS	  	0x55515 //PA0,2,4,....18.
	
//#define ALL_GPIOS_NUM     	30
//#define ALL_ODD_GPIOS_NUM 	15
//#define ALL_EVEN_GPIOS_NUM	15

//uint8_t ALL_ODD_GPIOS_ARRAY[ALL_ODD_GPIOS_NUM]   = {1,3,7,9,11,13,15,17,19,21,23,25,27,29,31};

//uint8_t ALL_EVEN_GPIOS_ARRAY[ALL_EVEN_GPIOS_NUM] = {0,2,4,8,10,12,14,16,18,20,22,24,26,28,30};

uint8_t ALL_ODD_GPIOS_ARRAY[]   = {1,3,5,9,11,13,15,17,19};

uint8_t ALL_EVEN_GPIOS_ARRAY[]  = {0,2,4,8,10,12,14,16,18};

#endif


#ifdef CHIP_QFN20
#define ALL_GPIOS            0xFAEF149D //(except PA6,7)
#define ALL_ODD_GPIOS	     0xAAAA0088 //PA.
#define ALL_EVEN_GPIOS	     0x50451415 //PA.

//#define ALL_GPIOS_NUM         18
//#define ALL_ODD_GPIOS_NUM     10
//#define ALL_EVEN_GPIOS_NUM    10
//const uint8_t ALL_ODD_GPIOS_ARRAY[ALL_ODD_GPIOS_NUM]      = {3,7,17,19,21,23,25,27,29,31};
//const uint8_t ALL_EVEN_GPIOS_ARRAY[ALL_EVEN_GPIOS_NUM]    = {0,2,4,10,12,16,18,22,28,30};
const uint8_t ALL_ODD_GPIOS_ARRAY[]      = {3,7,17,19,21,23,25,27,29,31};
const uint8_t ALL_EVEN_GPIOS_ARRAY[]     = {0,2,4,10,12,16,18,22,28,30};

#endif

#ifdef CHIP_SOP16
#define ALL_GPIOS            0xFAEF149D //(except PA6,7)
#define ALL_ODD_GPIOS	     0xAAAA0088 //PA.
#define ALL_EVEN_GPIOS	     0x50451415 //PA.

//#define ALL_GPIOS_NUM         18
//#define ALL_ODD_GPIOS_NUM     10
//#define ALL_EVEN_GPIOS_NUM    10
//const uint8_t ALL_ODD_GPIOS_ARRAY[ALL_ODD_GPIOS_NUM]      = {3,7,17,19,21,23,25,27,29,31};
//const uint8_t ALL_EVEN_GPIOS_ARRAY[ALL_EVEN_GPIOS_NUM]    = {0,2,4,10,12,16,18,22,28,30};
const uint8_t ALL_ODD_GPIOS_ARRAY[]      = {3,7,17,19,21,23,25,27,29,31};
const uint8_t ALL_EVEN_GPIOS_ARRAY[]     = {0,2,4,10,12,16,18,22,28,30};

#endif

void ft_gpio_inint_test(uint32_t mod)
{
	GPIO_DAT_CLR(ALL_GPIOS);
	GPIO_DIR_CLR(ALL_GPIOS);

    for(uint8_t i = 0; i < sizeof(ALL_ODD_GPIOS_ARRAY); i++)
    {
        CSC->CSC_PIO[ALL_ODD_GPIOS_ARRAY[i]].Word = mod;  
    }

    for(uint8_t i = 0; i < sizeof(ALL_EVEN_GPIOS_ARRAY); i++)
    {
        CSC->CSC_PIO[ALL_EVEN_GPIOS_ARRAY[i]].Word = mod; 
    }
}


/*INPUT TESET*/
void ft_gpio_in_oddhigh_evenlow_test(void)
{	
	uint32_t gpiostate;
	
	ft_gpio_inint_test(GPIOS_IE_MOD);
	GPIO_DIR_CLR(ALL_GPIOS);
	
	gpiostate = (GPIO_PIN_GET() & ALL_GPIOS); //(except PA6,7)
	
	if (gpiostate == ALL_ODD_GPIOS)
	{
		FT_RET(T_OK, CMD_GPIO_IN_OHEL, 0);
	}
	else 
	{
		FT_RET(T_FAIL, CMD_GPIO_IN_OHEL, 0);
	}
}

void ft_gpio_in_oddlow_evenhigh_test(void)
{
	uint32_t gpiostate;

	ft_gpio_inint_test(GPIOS_IE_MOD);
	GPIO_DIR_CLR(ALL_GPIOS);
	
	gpiostate = (GPIO_PIN_GET() & ALL_GPIOS); //(except PA6,7)
	
	if (gpiostate == ALL_EVEN_GPIOS)
	{
		FT_RET(T_OK, CMD_GPIO_IN_OLEH, 0);
	}
	else 
	{
		FT_RET(T_FAIL, CMD_GPIO_IN_OLEH, 0);
	}
}


/*OUTPUT TESET*/
void ft_gpio_out_oddhigh_evenlow_test(void)
{
	ft_gpio_inint_test(GPIOS_DEFAULT_MOD);
	GPIO_DIR_SET(ALL_GPIOS);
	GPIO_DAT_SET(ALL_ODD_GPIOS);
	GPIO_DAT_CLR(ALL_EVEN_GPIOS);
}

void ft_gpio_out_oddlow_evenhigh_test(void)
{ 
	ft_gpio_inint_test(GPIOS_DEFAULT_MOD);
	GPIO_DIR_SET(ALL_GPIOS);
	GPIO_DAT_CLR(ALL_ODD_GPIOS);
	GPIO_DAT_SET(ALL_EVEN_GPIOS);
}

/*Pullup/Pulldown TESET*/
void ft_gpio_oddpu_evenpd_test(void)
{	   
    ft_gpio_inint_test(GPIOS_IE_MOD);
	
	for (uint8_t i = 0; i < sizeof(ALL_ODD_GPIOS_ARRAY); i++)
	{
		iom_ctrl(ALL_ODD_GPIOS_ARRAY[i],IOM_PULLUP);   
	}
	
	for (uint8_t i = 0; i < sizeof(ALL_EVEN_GPIOS_ARRAY); i++)
	{
		iom_ctrl(ALL_EVEN_GPIOS_ARRAY[i],IOM_PULLDOWN);  
	}
}

void ft_gpio_oddpd_evenpu_test(void)
{	    
    ft_gpio_inint_test(GPIOS_IE_MOD);
	
	for (uint8_t i = 0; i < sizeof(ALL_ODD_GPIOS_ARRAY); i++)
	{
		iom_ctrl(ALL_ODD_GPIOS_ARRAY[i],IOM_PULLDOWN);   
	}
	
	for (uint8_t i = 0; i < sizeof(ALL_EVEN_GPIOS_ARRAY); i++)
	{
		iom_ctrl(ALL_EVEN_GPIOS_ARRAY[i],IOM_PULLUP);  
	}
}




