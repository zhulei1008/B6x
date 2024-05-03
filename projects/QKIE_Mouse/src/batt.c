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
#if (DBG_BATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

#define ADC_CHAN_VDD12        (14)    // 1200mV
#define ADC_READ_CNT          (0x07)
uint8_t batt_lvl = 0;
uint16_t batt_vol(void);
uint8_t batt_level(void);
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void batt_init(void)
{
    //uint16_t adc_val = 0;
   // batt_lvl=0;
    // PA08 ADC
    iom_ctrl(PA04, IOM_ANALOG);
    
    // 选择vref 2.4V
    sadc_init(SADC_ANA_DFLT);

    sadc_conf(SADC_CR_DFLT);
    
    // PA08对应ADC通道7(SADC_CH_AIN7)
    // 读8次求平均
    //adc_val = sadc_read(SADC_CH_AIN3, 8)/*&0xfffc*/;
    
    // 根据ADC读数计算io上输入电压
   // bat_vol = (adc_val * 2400) >> 10; // adc_val * 2400mV / 1024
    batt_lvl = batt_level();
    //DEBUG("adc:%x, vol:%d\r\n", adc_val, bat_vol);
}
uint16_t batt_vol(void)
{
	uint16_t adc_val1 = 0, bat_vol1 = 0;
	adc_val1 = (sadc_read(SADC_CH_AIN3, 8)/*&0xfffc*/);
	bat_vol1 = (adc_val1 * 2400) >> 10; // adc_val * 2400mV / 1024
	DEBUG("adc:%x, vol:%d\r\n", adc_val1, bat_vol1);
	return(bat_vol1);
}
uint8_t batt_level(void)
{
    uint8_t lvl = 100;
    //3.25-4.2
		uint16_t vdd = batt_vol()*2;
    if (vdd >= 4100)
        lvl = 100;
    else if ((vdd >= 4000) && (vdd < 4100))                 //0x5A-0X63  90-99
        lvl = 90 + (vdd - 4000)/10; 
    else if ((vdd >= 3900) && (vdd < 4000))                 //0X50-0X59  80-89
        lvl = 80 + (vdd - 3900)/10;  
    else if ((vdd >= 3800) && (vdd < 3900))                 //0X46-0X4F  70-79
        lvl = 70 + (vdd - 3800)/10; 
    else if ((vdd >= 3700) && (vdd < 3800))                 //0X3C-0X45  60-69
        lvl = 60 + (vdd - 3700)/10; 
    else if ((vdd >= 3600) && (vdd < 3700))                 //0X32-0X3B  50-59
        lvl = 50 + (vdd - 3600)/10;    
    else if ((vdd >= 3500) && (vdd < 3600))                 //0X28-0X31  40-49
       lvl = 40 + (vdd - 3500)/10;  
    else if ((vdd >= 3400) && (vdd < 3500))                 //0X1E-0X27  30-39
        lvl = 30 + (vdd - 3400)/10;      
    else if ((vdd >= 3350) && (vdd < 3400))                 //0X14-0X1D  20-29
        lvl = 20 + (vdd - 3350)/5;  
    else if ((vdd >= 3300) && (vdd < 3350))                 //0X0A-0X13  10-19
        lvl = 10 + (vdd - 3300)/5;       
    else if ((vdd >= 3250) && (vdd < 3300))                 //0X00-0X09  0-9
        lvl = 0x00 + (vdd - 3250)/5; 
		else lvl = 0;

    return lvl;
}
