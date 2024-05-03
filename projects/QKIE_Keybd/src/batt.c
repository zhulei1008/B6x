/**
 ****************************************************************************************
 *
 * @file batt.c
 *
 * @brief Battery voltage detection
 *
 ****************************************************************************************
 */
#include "batt.h"
#include "sadc.h"
#include "drvs.h"
#include "leds.h"
#include "user_api.h"
#include "ke_api.h"
#include "app.h"
/*
 * DEFINES
 ****************************************************************************************
 */
 #if (DBG_BATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

uint8_t batt_lvl;
bool powerOn_PowerLed_ActionDone_Flag;
uint32_t g_vdd12_vol;
static uint16_t battDetectPeriod = 100;
uint8_t lowLedBlinkCnt = 0;

#define SADC_ANA_1V2         (SADC_VREF_VDD | SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                                | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) /*| SADC_VREF_TRIM_2V4*/ | SADC_EN_BIT) // 136DC
                                
                                
                                
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void Batt_Init(void)
{
    //iom_ctrl(PA04, IOM_ANALOG);
	battDetectPeriod = 100;

    sadc_init(SADC_ANA_1V2);    // ѡ��vref 1.2V

    sadc_conf(SADC_CR_DFLT);
	
    /********************************************/
    // ��ȡFT��¼��VDD12��ѹ. wq --- 2023.11.06
    g_vdd12_vol = get_trim_vdd12_voltage();
    
    // FTû��¼��ѹʱ,��ΪĬ��1200.
    if (g_vdd12_vol == 0)
        g_vdd12_vol = 1200;
    /********************************************/
    DEBUG("g_vdd12_vol:%x\r\n", g_vdd12_vol);
     batt_vol();
	
    //DEBUG("vol:%d\r\n", batt_lvl);
}

uint16_t batt_vol(void)
{
	uint16_t adc_val1 = 0, bat_vol1 = 0;
	
	adc_val1 = (sadc_read(SADC_CH_VDD12, 8)/*&0xfffc*/);
	
	bat_vol1 = (g_vdd12_vol << 10)/ adc_val1;// adc_val * 1200mV / 1024
	
	DEBUG("adc:%x, vol:%d\r\n", adc_val1, bat_vol1);
	return(bat_vol1);
}

uint8_t batt_level(void)
{
    uint8_t lvl = 100;
	
	uint16_t vdd = batt_vol();
	
    if (vdd >= 4100)
        lvl = 100;
    else if ((vdd >= 4000) && (vdd < 4100))	//0x5A-0X63  90-99
        lvl = 90 + (vdd - 4000)/10; 
    else if ((vdd >= 3900) && (vdd < 4000)) //0X50-0X59  80-89
        lvl = 80 + (vdd - 3900)/10;  
    else if ((vdd >= 3800) && (vdd < 3900)) //0X46-0X4F  70-79
        lvl = 70 + (vdd - 3800)/10; 
    else if ((vdd >= 3700) && (vdd < 3800)) //0X3C-0X45  60-69
        lvl = 60 + (vdd - 3700)/10; 
    else if ((vdd >= 3600) && (vdd < 3700)) //0X32-0X3B  50-59
        lvl = 50 + (vdd - 3600)/10;    
    else if ((vdd >= 3500) && (vdd < 3600)) //0X28-0X31  40-49
        lvl = 40 + (vdd - 3500)/10;  
    else if ((vdd >= 3400) && (vdd < 3500)) //0X1E-0X27  30-39
        lvl = 30 + (vdd - 3400)/10;      
    else if ((vdd >= 3350) && (vdd < 3400)) //0X14-0X1D  20-29
        lvl = 20 + (vdd - 3350)/5;  
    else if ((vdd >= 3300) && (vdd < 3350)) //0X0A-0X13  10-19
        lvl = 10 + (vdd - 3300)/5;       
    else if ((vdd >= 3250) && (vdd < 3300)) //0X00-0X09  0-9
        lvl = (vdd - 3250)/5; 
	else 
		lvl = 0;

    return lvl;
}

void LowBatt_ALarm(void)
{
    uint32_t bat_Val = 4200;
	
	if (battDetectPeriod > 0) 
	{
		battDetectPeriod--;		
	}
	else	
	{
		battDetectPeriod = 100;
		bat_Val = batt_vol();
		
		if (bat_Val <= (LOWBATT_THRESHOLD - ONCHIP_DIODE_VOLTAGEDROP))
		{
			ke_timer_set(APP_TIMER_LOWBAT_ALARM,TASK_APP,1000);	
		}
		else
		{
			if (powerOn_PowerLed_ActionDone_Flag)
			{
				GPIO_DAT_CLR(BIT(POWER_LED));			
			}			
		}
	}
}
