/**
 ****************************************************************************************
 *
 * @file batt.c
 *
 * @brief Battery voltage detection
 *
 ****************************************************************************************
 */
#include "app.h"
#include "batt.h"
#include "sadc.h"
#include "drvs.h"
#include "sftmr.h"
#if (DBG_BATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<BATT>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

#define ADC_CHAN_VDD12        (14)    // 1200mV
#define ADC_READ_CNT          (0x07)
#define BATT_SCAN_INTEV 1000
uint8_t batt_lvl = 0;
uint8_t lvl_back ;
uint8_t batt_blink_max;
uint16_t batt_vol(void);
uint8_t batt_level(void);
uint8_t batt_scan_time_id = 0;
uint32_t g_vdd12_vol;
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#if(CFG_QFN32)
void batt_init(void)
{
   // uint16_t adc_val = 0; batt_lvl=0;
    // PA08 ADC
    iom_ctrl(PA04, IOM_ANALOG);
    
    // 选择vref 2.4V
    sadc_init(SADC_ANA_VREF_2V4);

    sadc_conf(SADC_CR_DFLT);
    
    // PA08对应ADC通道7(SADC_CH_AIN7)
    // 读8次求平均
    //adc_val = sadc_read(SADC_CH_AIN3, 8)/*&0xfffc*/;
    
    // 根据ADC读数计算io上输入电压
   // bat_vol = (adc_val * 2400) >> 10; // adc_val * 2400mV / 1024
    lvl_back = 0xff;
    batt_blink_max = 0;
    batt_scan_time_id=0;
    batt_lvl = batt_level();
    //DEBUG("adc:%x, vol:%d\r\n", adc_val, batt_lvl);
}
uint16_t batt_vol(void)
{
	uint16_t adc_val1 = 0, bat_vol1 = 0;
	adc_val1 = (sadc_read(SADC_CH_AIN3, 8)/*&0xfffc*/);
	bat_vol1 = (adc_val1 * 2400) >> 10; // adc_val * 2400mV / 1024
	DEBUG("adc:%x, vol:%d\r\n", adc_val1, bat_vol1);
	return(bat_vol1);
}
#else
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void batt_init(void)
{
    //iom_ctrl(PA04, IOM_ANALOG);


    sadc_init(SADC_ANA_VREF_VDD);    // 选择vref 1.2V

    sadc_conf(SADC_CR_DFLT);
	lvl_back = 0xff;
    batt_blink_max = 0;
    batt_scan_time_id=0;
    batt_lvl = batt_level();
    /********************************************/
    // 获取FT烧录的VDD12电压. wq --- 2023.11.06
    g_vdd12_vol = get_trim_vdd12_voltage();
    
    // FT没烧录电压时,设为默认1200.
    if (g_vdd12_vol == 0)
        g_vdd12_vol = 1200;
    /********************************************/
    DEBUG("g_vdd12_vol:%x\r\n", g_vdd12_vol);
    batt_lvl=batt_vol();
	
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
#endif
uint8_t batt_level(void)
{
    uint8_t lvl = 100;
    //3.25-4.2
    #if(CFG_QFN32)
	uint16_t vdd = batt_vol()*2;
    #else
    uint16_t vdd = batt_vol()+30;//add 50mv
    #endif
    DEBUG("vdd:%d\r\n", vdd);
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
    else if (vdd>3000) lvl = 1;
		else lvl = 0;

    return lvl;
}

static tmr_tk_t batt_scan_handle(tmr_id_t id)
{
    
    if (id == batt_scan_time_id)
    {
        DEBUG("batttime");
        batt_lvl=batt_level();
        //if(lvl_back>batt_lvl)
        lvl_back= batt_lvl;
        if(lvl_back==0)
        {
            ke_timer_set(APP_BATT_LOW_LINK, TASK_APP, 1000);
           
        }
        else
        {
            batt_blink_max = 0;
        }
    }

    return (BATT_SCAN_INTEV);
}

void batt_scan_init(void)
{
    batt_init();


    if (batt_scan_time_id != TMR_ID_NONE)
    {
        sftmr_clear(batt_scan_time_id);
        batt_scan_time_id = TMR_ID_NONE;
    }

    batt_scan_time_id = sftmr_start(BATT_SCAN_INTEV, batt_scan_handle);
    
}
#if(0)
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
uint8_t batt_lvl;

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
#endif

