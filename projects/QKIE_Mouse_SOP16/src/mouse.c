/**
 ****************************************************************************************
 *
 * @file keys.c
 *
 * @brief keys operation.
 *
 ****************************************************************************************
 */

#include "regs.h"
#include "drvs.h"
#include "app.h"
#include "hid_desc.h"
#include "mouse.h"
#include "sftmr.h"
#include "gapc_api.h"
#include "leds.h"
#include "bledef.h"
#include "user_api.h"

#if (DBG_MOUSE)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define MOUSE_SCAN_INTEV 1

#define GPIO_DIRCLR(bits)     GPIO->DIR_CLR = bits
#define GPIO_DIRSET(bits)     GPIO->DIR_SET = bits
#define GPIO_DATSET(bits)     GPIO->DAT_SET = bits
#define GPIO_DATCLR(bits)     GPIO->DAT_CLR = bits
#define GPIO_GET_DATA(io_idx) (((GPIO->PIN) >> (io_idx)) & 0x01)
#define GPIO_ALL_DATA()       (GPIO->PIN)

#define IOM_CTRL(pad, ctrl)   CSC->CSC_PIO[pad].Word = (ctrl)
// #define CSC_EN(fsel)            ((fsel) | (1UL << 8/*CSCFEN*/))
#define CSC_INPUT(pad, fsel)  CSC->CSC_INPUT[fsel].Word = CSC_EN(pad)
#define CSC_OUTPUT(pad, fsel) CSC->CSC_OUTPUT[pad].Word = CSC_EN(fsel)

// LED
#define PAD_LED_OUT (1)

#define SCLK_L() GPIO_DATCLR(PA(PAD_SENSOR_CLK))
#define SCLK_H() GPIO_DATSET(PA(PAD_SENSOR_CLK))
#define SDIO_L() GPIO_DATCLR(PA(PAD_SENSOR_DIO))
#define SDIO_H() GPIO_DATSET(PA(PAD_SENSOR_DIO))

#define PAD_SENSOR_MSK (PA(PAD_SENSOR_CLK) | PA(PAD_SENSOR_DIO))

#define PAD_INPUT_MSK (PAD_WHEEL_MSK | PAD_KSCAN_MSK)

// Product_ID1
#define PAW3212_ID      (0x02)
#define PAW3205_ID      (0xD0)
#define FCT3065_ID      (0x31)
#define KA5857_ID       (0x58)
#define PIXART_ID       (0x30) // PAW3205		MX8650A
// #define PAW3205_ID						0xD0
#define MX8650A_ID      (0x50) // SPCP6050
#define MOUSE_SENSOR_ID PAW3205_ID

#define KA5857_DPI0 (21)
#define KA5857_DPI1 (32)
#define KA5857_DPI2 (42)
#define KA5857_DPI3 (63)

#define SENSOR_X_ADDR (0x03)
#define SENSOR_Y_ADDR (0x04)
#define SENSOR_XY_H   (0x12)
#define SENSOR_POWER  (0x06)
#define Sensor_Move() (mouse_sensor_read_reg(2) & 0x80)

//const uint8_t  def_btaddr[6] = BLE_ADDR;

uint8_t Sensor_type;

uint8_t CPI_Count;
uint8_t sensor_id;
uint8_t update_connect_par_flag;
uint8_t key_data;
uint8_t config_sleep;
uint8_t updata_conn_status;

uint8_t conn_intv;
uint8_t conn_late;

uint8_t wheel_val;
uint8_t wheel_cnt;
uint8_t channle_slect;
uint8_t wheel_cnt = 0;
uint32_t work_mode;
uint32_t cpi_data ;
uint32_t bt_addr[2];
uint8_t g_poweron_key_sta;
__DATA_ALIGNED(4) uint8_t ltk_data[56 /*sizeof(struct gapc_ltk )*2*/];
__DATA_ALIGNED(4) uint8_t dongle_addr[8];
__DATA_ALIGNED(4) uint8_t poweroff_store_dongle_addr[8];
// uint32_t ltk_32bit[14];
uint8_t poweron_work_status;
uint8_t sys_timeout_cnt;

static uint8_t  updata_parm_data;
static uint16_t update_cnt1;
static uint16_t update_cnt2;
static uint8_t  update_count;
static uint16_t key_status_cont;
static uint8_t  DPI_DATA_BACK;

static void mouse_sensor_resync(void);

const struct gapc_conn_param mouse_le_conn_pref1 = {
    /// Connection interval minimum unit in 1.25ms
    .intv_min = 6,
    /// Connection interval maximum unit in 1.25ms
    .intv_max = 6,
    /// Slave latency
    .latency = 0,
    /// Connection supervision timeout multiplier unit in 10ms
    .time_out = 300,
};

const struct gapc_conn_param mouse_le_conn_pref2 = {
    /// Connection interval minimum unit in 1.25ms
    .intv_min = 6,
    /// Connection interval maximum unit in 1.25ms
    .intv_max = 6,
    /// Slave latency
    .latency = CFG_CONN_LATENCY, // 24,
    /// Connection supervision timeout multiplier unit in 10ms
    .time_out = 300,
};

void mouse_data_clear(void)
{
    Sensor_type             = 0;
    wheel_val               = 0;
    wheel_cnt               = 0;
    CPI_Count               = 1;
    update_connect_par_flag = 0;
//    mouse_scan_time_id      = 0;
    key_data                = 0;
    updata_parm_data        = 0;
    update_cnt1             = 0;
    update_cnt2             = 0;
    key_status_cont         = 0;
    DPI_DATA_BACK           = 0;
    config_sleep            = 0x02;
    updata_conn_status      = 0;
    work_mode               = BT_MODE;
    memset(dongle_addr,0xff,8);
    bt_addr[BT_ADDR_0]      = 0;
    bt_addr[BT_ADDR_1]      = 0;

#if (CFG_TEST_CIRCLE_DATA)
    g_xy_idx = 0;
    g_key_start = 0;
#endif

    channle_slect = 0;
//    memset(ltk_data, 0, sizeof(ltk_data));
    poweron_work_status = SYS_IDLE;
    sys_timeout_cnt     = 0;
//    memcpy((uint8_t *)&ble_dev_addr, def_btaddr, 6);
}

enum mouse_cpi
{
    CPI_600 = 0,
    CPI_800,
    CPI_1000,
    CPI_1200,
    CPI_1600,

    CPI_MAX = CPI_1600,
};

struct mouse_report st_mouse_env;

/*
 * FUNCTIONS
 ****************************************************************************************
 */
#if (CFG_TEST_CIRCLE_DATA)
uint8_t g_key_start, g_xy_idx;
int8_t g_x_offset[] = {-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1, 1, 2, 4, 5, 6, 7, 8, 9, 8,8,9,8,7,6,5,4,2,1};
int8_t g_y_offset[] = { 8, 9, 8, 7, 6, 5, 4, 2, 1,-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1,1,2,4,5,6,7,8,9,8};
#endif

uint8_t mouse_key_scan(void)
{
    GPIO_DATSET(PA(PAD_KSCAN_C0));
    GPIO_DIRSET(PA(PAD_KSCAN_C0));
    IOM_CTRL(PAD_KSCAN_C0, IOM_DRV_LVL1 | IOM_SEL_GPIO);

    uint8_t  key_press1, key_press2, real_press;
    uint32_t pin_val;

    pin_val = GPIO_ALL_DATA();

    GPIO_DATCLR(PA(PAD_KSCAN_C0));
    #if(CFG_QFN32)
    key_press1  = (((pin_val >> PAD_KSCAN_R0) & 0x01) << 0);
    key_press1 |= (((pin_val >> PAD_KSCAN_R2) & 0x01) << 1);
    key_press1 |= (((pin_val >> PAD_KSCAN_R1) & 0x01) << 2);

    key_press1 ^= ROW_PAD_MSK;

    pin_val     = GPIO_ALL_DATA();
    key_press2  = (((pin_val >> PAD_KSCAN_R0) & 0x01) << 0);
    key_press2 |= (((pin_val >> PAD_KSCAN_R2) & 0x01) << 1);
    key_press2 |= (((pin_val >> PAD_KSCAN_R1) & 0x01) << 2);
    #else
    key_press1  = (((pin_val >> PAD_KSCAN_R2) & 0x01) << 0);
    key_press1 |= (((pin_val >> PAD_KSCAN_R0) & 0x01) << 1);
    key_press1 |= (((pin_val >> PAD_KSCAN_R1) & 0x01) << 2);

    key_press1 ^= ROW_PAD_MSK;

    pin_val     = GPIO_ALL_DATA();
    key_press2  = (((pin_val >> PAD_KSCAN_R2) & 0x01) << 0);
    key_press2 |= (((pin_val >> PAD_KSCAN_R0) & 0x01) << 1);
    key_press2 |= (((pin_val >> PAD_KSCAN_R1) & 0x01) << 2);
    #endif
    
    key_press2 ^= ROW_PAD_MSK;
    real_press = key_press1 ^ key_press2;

    
    for (uint8_t i = 0; i < 3; ++i)
    {
        if (((real_press >> i) & 0x01) == 0)
        {
            key_press2 &= ~(0x01 << i);
        }
    }

    real_press = (key_press2 << 3 | key_press1);
    
    GPIO_DATSET(PA(PAD_KSCAN_C0));
    
    if (real_press)
    {
        if ((real_press & (0x03 << 4)) >= 0x10)
        {
            uint8_t real_press_bit4  = ((real_press >> 4) & 0x01);
            uint8_t real_press_bit5  = ((real_press >> 5) & 0x01);
            real_press              &= ~(0x0F << 4);
            real_press              |= real_press_bit4 << 5;
            real_press              |= real_press_bit5 << 4;
        }

        //DEBUG("pres1:%x, pres2:%x, real:%x, io_sta:%x", key_press1, key_press2, real_press, GPIO_ALL_DATA());
        //DEBUG("pres1:%x, pres2:%x, real:%x, io_sta:%x", key_press1, key_press2, real_press, GPIO_ALL_DATA());
        //g_poweron_key_sta = 0;
        st_mouse_env.sta                         |= MOUSE_STA_BUTTON;
        st_mouse_env.mouseTyp                     = MOUSE_BUTTON;
        st_mouse_env.report[MOUSE_BUTTON_OFFSET]  = real_press;
        //DEBUG("real:%x",real_press);
    }
    
#if (CFG_TEST_CIRCLE_DATA)

    if (real_press == (BTN_L | BTN_R))
    {
        g_key_start = 1; 
    }
    else if (real_press == BTN_R)
    {
        g_key_start = 0; 
        g_xy_idx = 0;
    }
    
    if ((g_key_start) && (hids_report_buff_empty()))
    {
        #if (CFG_IO_OBSERVE)
        GPIO_DAT_SET(GPIO06);
        #endif
        st_mouse_env.report[MOUSE_X_OFFSET] = (g_x_offset[g_xy_idx] << 1) & 0xFF;
        st_mouse_env.report[MOUSE_Y_OFFSET] = (g_y_offset[g_xy_idx] << 1) & 0xFF;
        g_xy_idx = (g_xy_idx + 1) % sizeof(g_y_offset);

        st_mouse_env.sta      |= MOUSE_STA_XY;
        st_mouse_env.mouseTyp  = MOUSE_XY;
        #if (CFG_IO_OBSERVE)
        GPIO_DAT_CLR(GPIO06);
        #endif
    }
#endif
    
    return real_press;
}

// Wheel Scroll
uint8_t mouse_scroll(void)
{
    // 00 -> 02 -> 03 -> 01 -> 02 -> 03 -> 01 -> 02
    static uint8_t sta_last = 0, sta_before = 0, sta_now = 0;
    uint8_t        sta_tmp = 0, result = 0;
//    if(++wheel_cnt>1)wheel_cnt=0;
//    else return (0);
    IOM_CTRL(PAD_WHEEL_ZA, IOM_PULLUP | IOM_INPUT);
    IOM_CTRL(PAD_WHEEL_ZB, IOM_PULLUP | IOM_INPUT);
    if (GPIO_GET_DATA(PAD_WHEEL_ZA) == 0)
    {
        sta_tmp |= 2;
    }

    if (GPIO_GET_DATA(PAD_WHEEL_ZB) == 0)
    {
        sta_tmp |= 1;
    }

    if (sta_now == sta_tmp)
    {
        wheel_val = 0; 
        return 0;
    }
    IOM_CTRL(PAD_WHEEL_ZA,   IOM_HIZ);
    IOM_CTRL(PAD_WHEEL_ZB,   IOM_HIZ);
    sta_last   = sta_before;
    sta_before = sta_now;
    sta_now    = sta_tmp;

    result = sta_now << 4 | sta_before << 2 | sta_last;

    switch (result)
    {
        case 0x0B:
//        case 0x12:
//        case 0x2D:
        case 0x34:
        {
            wheel_val++;
        }
        break;

        case 0x07:
//        case 0x1E:
//        case 0x21:
        case 0x38:
        {
            wheel_val--;
        }
        break;

        default:
            break;
    }

    if (wheel_val != 0)
    {
        st_mouse_env.sta                        |= MOUSE_STA_WHEEL;
        st_mouse_env.mouseTyp                    = MOUSE_WHEEL;
        st_mouse_env.report[MOUSE_WHEEL_OFFSET]  = wheel_val;
       
        wheel_cnt = 100;
    }
    else
    {
        st_mouse_env.sta &=(~MOUSE_STA_WHEEL);
        st_mouse_env.mouseTyp &= (~MOUSE_WHEEL);
        st_mouse_env.report[MOUSE_WHEEL_OFFSET]  = 0;
    }

    return 1;
}

#if (CFG_HW_SPI)

/*********************************************/
/******************* HW SPI ******************/
/*********************************************/
__INLINE__ void iocsc_hw_spi_clk(uint8_t pad_clk)
{
    // csc connect
    CSC_OUTPUT(pad_clk,  CSC_SPIM_CLK);
    
    // iomode control
    IOM_CTRL(pad_clk,  IOM_SEL_CSC | IOM_DRV_LVL1/* | IOM_PULLUP*/);
}

__INLINE__ void iocsc_hw_spi_mosi(uint8_t pad_mosi)
{
    CSC->CSC_INPUT[CSC_SPIM_MISO].Word = 0;
    CSC_OUTPUT(pad_mosi, CSC_SPIM_MOSI);
    
//    CSC_INPUT(pad_mosi,  CSC_SPIM_MISO);
    IOM_CTRL(pad_mosi, IOM_SEL_CSC | IOM_DRV_LVL1/* | IOM_PULLUP*/);
}

__INLINE__ void iocsc_hw_spi_miso(uint8_t pad_miso)
{
    CSC->CSC_OUTPUT[pad_miso].Word = 0;
    CSC_INPUT(pad_miso,  CSC_SPIM_MISO);
    IOM_CTRL(pad_miso, IOM_SEL_CSC | IOM_PULLUP | IOM_INPUT);
}

void hw_spi_init(void)
{
    RCC->AHBCLK_EN_RUN.SPIM_CLKEN_RUN = 0;
    RCC->AHBRST_CTRL.SPIM_RSTREQ      = 1;
    RCC->AHBRST_CTRL.SPIM_RSTREQ      = 0;
    RCC->AHBCLK_EN_RUN.SPIM_CLKEN_RUN = 1;
    
    iocsc_hw_spi_clk(PAD_SENSOR_CLK);
//    iocsc_hw_spi_mosi(PAD_SENSOR_DIO);
    
//    SPIM->CTRL.SPIM_CRAT      = 3;//spi_clk = hclk/2^(crat+1)
//    SPIM->CTRL.SPIM_CPHA      = 1;
//    SPIM->CTRL.SPIM_CPOL      = 1;
//    SPIM->CTRL.SPIM_TX_EN     = 1;
//    SPIM->CTRL.SPIM_RX_EN     = 1;
    SPIM->CTRL.Word           = 0x0E32;//bit0-4  
    SPIM->DAT_LEN             = 2;
}

void hw_spi_write(uint8_t addr, uint8_t data)
{
    addr |= 0x80;
    
    //DEBUG("addr:%02x, data:%02x", addr, data);
    iocsc_hw_spi_mosi(PAD_SENSOR_DIO);
    
    SPIM->STATUS_CLR.Word     = SPIM_STATUS_CLR_ALL;
    
    // send data
    SPIM->TX_DATA             = addr;
    SPIM->TX_DATA             = data;
    SPIM->TXRX_BGN            = 1;

    // wait data send complete
//    while (!(SPIM->STATUS.SPIM_INTF));
    while (SPIM->STATUS.SPIM_BUSY);
    
//    DEBUG("addr:%02x, data:%02x", addr, data);
    SPIM->STATUS_CLR.Word     = SPIM_STATUS_CLR_ALL;
}

uint8_t hw_spi_read(uint8_t addr)
{
    uint8_t recv_data = 0;
    
    addr &= ~0x80U;

    // 配置IO为MOSI
    iocsc_hw_spi_mosi(PAD_SENSOR_DIO);
    
    SPIM->STATUS_CLR.Word     = SPIM_STATUS_CLR_ALL;
    
    // send 1 byte data
    SPIM->TX_DATA             = addr;
    SPIM->TXRX_BGN            = 1;
    
    // wait data send complete
//    while (!(SPIM->STATUS.SPIM_RX_FEMPTY));
    while (SPIM->STATUS.SPIM_BUSY);
    recv_data = SPIM->RX_DATA;
    
//    SPIM->STATUS_CLR.Word     = SPIM_STATUS_CLR_ALL;
    
    // 配置IO为MISO
    iocsc_hw_spi_miso(PAD_SENSOR_DIO);

    SPIM->TX_DATA             = 0xFF;
    // recv data
//    SPIM->TXRX_BGN            = 1;
    
    // wait receive 
//    while (!(SPIM->STATUS.SPIM_RX_FEMPTY));
    while (SPIM->STATUS.SPIM_BUSY);

    recv_data = SPIM->RX_DATA;
    
    SPIM->STATUS_CLR.Word     = SPIM_STATUS_CLR_ALL;

//    DEBUG("addr:%02x, data:%02x", addr, recv_data);
//    IOM_CTRL(PAD_SENSOR_DIO, IOM_HIZ);

    return recv_data;
}
#else
/*********************************************/
/******************* SFT SPI *****************/
/*********************************************/
static uint8_t sft_io_spi_read(void)
{
    uint8_t byte_val = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        SCLK_L();
//        delay_us(1);
        byte_val <<= 1;
        SCLK_H();
//        delay_us(1);
        byte_val |= GPIO_GET_DATA(PAD_SENSOR_DIO);
    }

    return (byte_val);
}

static void sft_io_spi_write(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        SCLK_L();

//        delay_us(1);
        if (data & 0x80)
        {
            SDIO_H();
        }
        else
        {
            SDIO_L();
        }

        SCLK_H();
        data <<= 1;
//        delay_us(1);
    }
}

uint8_t sft_spi_read(uint8_t addr)
{
    uint8_t recv_data = 0;

    addr &= ~0x80U;
    
    GPIO_DATSET(PAD_SENSOR_MSK);
    GPIO_DIRSET(PAD_SENSOR_MSK);

    sft_io_spi_write(addr);

    GPIO_DIRCLR(PA(PAD_SENSOR_DIO));
    IOM_CTRL(PAD_SENSOR_DIO, IOM_PULLUP | IOM_INPUT);

    recv_data = sft_io_spi_read();

    return recv_data;
}

void sft_spi_write(uint8_t addr, uint8_t data)
{
    GPIO_DATSET(PAD_SENSOR_MSK);
    GPIO_DIRSET(PAD_SENSOR_MSK);

    sft_io_spi_write(addr | 0x80);
    sft_io_spi_write(data);
}

/*********************************************/
#endif

void mouse_io_init(void)
{
    GPIO_DIRCLR(PAD_INPUT_MSK);

    GPIO_DATSET(PA(PAD_KSCAN_C0));
    GPIO_DIRSET(PA(PAD_KSCAN_C0));
    IOM_CTRL(PAD_KSCAN_C0, IOM_DRV_LVL1 | IOM_SEL_GPIO);

#if (CFG_HW_SPI)
    hw_spi_init();
#else
    GPIO_DATSET(PAD_SENSOR_MSK);
    GPIO_DIRSET(PAD_SENSOR_MSK);
#endif

    IOM_CTRL(PAD_WHEEL_ZA, IOM_PULLUP | IOM_INPUT);
    IOM_CTRL(PAD_WHEEL_ZB, IOM_PULLUP | IOM_INPUT);

    IOM_CTRL(PAD_KSCAN_R0, IOM_PULLUP | IOM_INPUT);
    IOM_CTRL(PAD_KSCAN_R1, IOM_PULLUP | IOM_INPUT);
    IOM_CTRL(PAD_KSCAN_R2, IOM_PULLUP | IOM_INPUT);

//    IOM_CTRL(PAD_LED_OUT,  IOM_HIZ);
//    IOM_CTRL(PAD_WHEEL_ZA, IOM_HIZ);
//    IOM_CTRL(PAD_WHEEL_ZB, IOM_HIZ);
//    mouse_sensor_resync();
}

void mouse_io_hiz(void)
{
//    IOM_CTRL(PAD_KSCAN_C0,   IOM_HIZ);
//    IOM_CTRL(PAD_KSCAN_R1,   IOM_HIZ);
//    IOM_CTRL(PAD_KSCAN_R2,   IOM_HIZ);

//    //IOM_CTRL(PAD_LED_OUT,    IOM_HIZ);
//    IOM_CTRL(PAD_WHEEL_ZA,   IOM_HIZ);
//    IOM_CTRL(PAD_WHEEL_ZB,   IOM_HIZ);
#if (CFG_HW_SPI)

    CSC->CSC_INPUT[CSC_SPIM_MISO].Word   = 0;
    CSC->CSC_OUTPUT[PAD_SENSOR_DIO].Word = 0;

//    IOM_CTRL(PAD_SENSOR_CLK, IOM_HIZ);
    IOM_CTRL(PAD_SENSOR_DIO, IOM_HIZ);
#else

    IOM_CTRL(PAD_SENSOR_CLK, IOM_HIZ);
    IOM_CTRL(PAD_SENSOR_DIO, IOM_HIZ);
#endif
}

uint8_t mouse_sensor_read_reg(uint8_t addr)
{
    uint8_t reg_val = 0;

#if (CFG_HW_SPI)
    reg_val = hw_spi_read(addr);
#else
    reg_val = sft_spi_read(addr);
#endif
    
    return reg_val;
}

void mouse_sensor_write_reg(uint8_t addr, uint8_t data)
{
#if (CFG_HW_SPI)
    hw_spi_write(addr, data);
#else
    sft_spi_write(addr, data);
#endif
}

static void mouse_sensor_resync(void)
{
#if (CFG_HW_SPI)
    CSC->CSC_OUTPUT[PAD_SENSOR_CLK].Word = 0;
    CSC->CSC_OUTPUT[PAD_SENSOR_DIO].Word = 0;
    CSC->CSC_INPUT[CSC_SPIM_MISO].Word   = 0;
#else
    IOM_CTRL(PAD_SENSOR_DIO, IOM_SEL_GPIO | IOM_INPUT);
#endif
    SCLK_H();
    GPIO_DIRSET(PA(PAD_SENSOR_CLK));
    IOM_CTRL(PAD_SENSOR_CLK, IOM_SEL_GPIO);

    //SCLK_H();
    bootDelayUs(10);

    SCLK_L();

    // 100us
    bootDelayUs(100);

    SCLK_H();

    // 10ms
    bootDelayUs(10000);

#if (CFG_HW_SPI)
    iocsc_hw_spi_clk(PAD_SENSOR_CLK);
    iocsc_hw_spi_mosi(PAD_SENSOR_DIO);
#endif
}

uint8_t mouse_sensor_id(void)
{
    uint8_t err_cnt = 6;

    do
    {
        sensor_id = mouse_sensor_read_reg(0x00);
        DEBUG("sensor_id:%02x, resync_cnt:%d", sensor_id, 6 - err_cnt);

        if ((sensor_id == PIXART_ID) || (sensor_id == FCT3065_ID) || (sensor_id == KA5857_ID))
        {
            return sensor_id;
        }

        mouse_sensor_resync();

    } while (--err_cnt);

    return 0;
}

// performance optimization
void mouse_sensor_paw3205_init(void)
{
//    mouse_sensor_write_reg(0x09, 0x5A);
//    mouse_sensor_write_reg(0x0B, 0xf3);
//    mouse_sensor_write_reg(0x09, 0x00);
    
    mouse_sensor_write_reg(0x09, 0x5A);
    mouse_sensor_write_reg(0x28, 0xB4);
    mouse_sensor_write_reg(0x29, 0x46);
    mouse_sensor_write_reg(0x2A, 0x96);
    mouse_sensor_write_reg(0x2B, 0x8C);
    mouse_sensor_write_reg(0x2C, 0x6E);
    mouse_sensor_write_reg(0x2D, 0x64);
    mouse_sensor_write_reg(0x38, 0x5F);
    mouse_sensor_write_reg(0x39, 0x0F);
    mouse_sensor_write_reg(0x3A, 0x32);
    mouse_sensor_write_reg(0x3B, 0x47);
    mouse_sensor_write_reg(0x42, 0x10);
    //mouse_sensor_write_reg(0x43, 0x09);//jia
    mouse_sensor_write_reg(0x54, 0x2E);
    mouse_sensor_write_reg(0x55, 0xF2);
    mouse_sensor_write_reg(0x61, 0xF4);
    mouse_sensor_write_reg(0x63, 0x70);
    mouse_sensor_write_reg(0x75, 0x52);
    mouse_sensor_write_reg(0x76, 0x41);
    mouse_sensor_write_reg(0x77, 0xED);
    mouse_sensor_write_reg(0x78, 0x23);
    mouse_sensor_write_reg(0x79, 0x46);
    mouse_sensor_write_reg(0x7A, 0xE5);
    mouse_sensor_write_reg(0x7C, 0x48);
    mouse_sensor_write_reg(0x7D, 0x80);//
    mouse_sensor_write_reg(0x7E, 0x77);
    mouse_sensor_write_reg(0x7F, 0x01);
    mouse_sensor_write_reg(0x0B, 0x00);
    mouse_sensor_write_reg(0x7F, 0x00);
    mouse_sensor_write_reg(0x09, 0x00);
}

void MX8650A_ID_Init(void)
{
    mouse_sensor_write_reg(0xFF, 0x27);
    mouse_sensor_write_reg(0xAB, 0x40);
    mouse_sensor_write_reg(0xBE, 0xD1);
    mouse_sensor_write_reg(0xA0, 0x41);
    mouse_sensor_write_reg(0xA1, 0x41);
    mouse_sensor_write_reg(0xA5, 0x84);
    mouse_sensor_write_reg(0xA7, 0xFF);
    mouse_sensor_write_reg(0xB5, 0xD0);
    mouse_sensor_write_reg(0xB6, 0x00);
    mouse_sensor_write_reg(0xFF, 0x00);
}

void KA5857_ID_Init(void)
{
    do
    {
        mouse_sensor_write_reg(0x09, 0xA5);
        mouse_sensor_write_reg(0x19, 0x40);
        mouse_sensor_write_reg(0x6A, 0xE0);
        mouse_sensor_write_reg(0x69, 0x23);
    } while (mouse_sensor_read_reg(0x09) != 0xA5);

    mouse_sensor_write_reg(0x09, 0x00);
}

void mouse_move(void)
{
    uint8_t motion_sta = 0;
    int8_t  delta_x = 0, delta_y = 0;

//    #if (CFG_HW_SPI)
//    iocsc_hw_spi_clk(PAD_SENSOR_CLK);
//    #endif
    
    if (hids_report_buff_empty())
    {

            motion_sta = mouse_sensor_read_reg(0x02);
            if ((motion_sta & (1U << 7)) && (motion_sta != 0xFF))
            {
                #if(CFG_QFN32)
                delta_x                             = mouse_sensor_read_reg(0x03);
                st_mouse_env.report[MOUSE_X_OFFSET] = 0-delta_x;
                //st_mouse_env.report[MOUSE_Y_OFFSET] =delta_y;// 0;//delta_x;

                delta_y                             = mouse_sensor_read_reg(0x04);
                st_mouse_env.report[MOUSE_Y_OFFSET] = delta_y;
                
                #else
                delta_x                             = mouse_sensor_read_reg(0x03);
                st_mouse_env.report[MOUSE_X_OFFSET] = delta_x;

                delta_y                             = mouse_sensor_read_reg(0x04);
                st_mouse_env.report[MOUSE_Y_OFFSET] = (0 - delta_y);
                #endif
                st_mouse_env.sta      |= MOUSE_STA_XY;
                st_mouse_env.mouseTyp  = MOUSE_XY;
        }
    }
}

void CPI_Write(void)
{
    uint8_t CPI_TMP;

    switch (CPI_Count)
    {
        case 0:
            CPI_TMP = 0;
            break;

        case 1:
            CPI_TMP = 2;
            break;

        case 2:
            CPI_TMP = 3;
            break;
    }

    mouse_sensor_id();

    switch (Sensor_type)
    {
        case PAW3205_ID:
            mouse_sensor_write_reg(SENSOR_POWER, (CPI_TMP + 1));
            CPI_TMP = 0;
            CPI_TMP = mouse_sensor_read_reg(SENSOR_POWER);
            DEBUG("CPI:0x%x\r\n", CPI_TMP);
            // 00	600  800	1000	1200	1600
            break;

        case FCT3065_ID:
            mouse_sensor_write_reg(SENSOR_POWER, CPI_TMP);
            // 00	800	1000	1300	1600
            break;

        case MX8650A_ID:
            mouse_sensor_write_reg(SENSOR_POWER, CPI_TMP);
            // 00	800	1000	1200	1600
            break;

        case KA5857_ID:
            mouse_sensor_write_reg(0x5A, 0x09);

            switch (CPI_TMP)
            {
                case 0:
                    mouse_sensor_write_reg(0x0D, KA5857_DPI0);
                    mouse_sensor_write_reg(0x0E, KA5857_DPI0);
                    break;

                case 1:
                    mouse_sensor_write_reg(0x0D, KA5857_DPI1);
                    mouse_sensor_write_reg(0x0E, KA5857_DPI1);
                    break;

                case 2:
                    mouse_sensor_write_reg(0x0D, KA5857_DPI2);
                    mouse_sensor_write_reg(0x0E, KA5857_DPI2);
                    break;

                case 3:
                    mouse_sensor_write_reg(0x0D, KA5857_DPI3);
                    mouse_sensor_write_reg(0x0E, KA5857_DPI3);
                    break;
            }

            mouse_sensor_write_reg(0x00, 0x09);
            break;
    }
}

void sensor_powerdown(void)
{
    uint8_t config  = 0;
    uint8_t id= mouse_sensor_id();
    DEBUG("id:%x", id);
    config          = mouse_sensor_read_reg(0x06);
    config_sleep    = config;
    
    config         |= 0x08;
    DEBUG("config:%x", config);
    mouse_sensor_write_reg(0x06, config); // sensor Power down
    
//    config = 0;
    
    config          = mouse_sensor_read_reg(0x06);
    DEBUG("config:%x", config);
    if((config & 0x08) != 0x08)
    {
        config         |= 0x08;
        mouse_sensor_write_reg(0x06, config); // sensor Power down
    }
    
//    config = 0;
    config          = mouse_sensor_read_reg(0x06);
    DEBUG("config:%x", config);
    if((config & 0x08)  != 0x08)
    {
        config         |= 0x08;
        mouse_sensor_write_reg(0x06, config); // sensor Power down
    }
    
    bootDelayUs(5);
}

void Sensor_Power_ON(void)
{
    uint8_t config = 0;
    config         = mouse_sensor_read_reg(0x06) & 0xF7;
    mouse_sensor_write_reg(0x06, config); // sensor Power on
}

void mouse_sensor_init(void)
{
    uint8_t id_valid      = 0;
    uint8_t sensor_config = 0;
    uint8_t sensor_init_ok_flag;
    
    bootDelayUs(10);

//    mouse_sensor_write_reg(0x06, config_sleep);
//    bootDelayUs(1000);
SENSOR_SYNC:
    id_valid = mouse_sensor_id();
    DEBUG("id_valid:0x%x", id_valid);
    sensor_init_ok_flag = 1;

    if (id_valid == 0)
    {
        sensor_init_ok_flag = 0;
        DEBUG("Not_read_sensor\r\n");
       // NVIC_SystemReset();
        goto SENSOR_SYNC;
    }

    mouse_sensor_write_reg(0x06, config_sleep);
    bootDelayUs(1000);

//    if (id_valid != 0)
    {
        switch (sensor_id)
        {
            case FCT3065_ID:
                Sensor_type = FCT3065_ID;
                sensor_id  = mouse_sensor_read_reg(1);
                DEBUG("sensor_3065_id1:0x%x", sensor_id);
                break;

            // goto Init;
            case KA5857_ID:
                Sensor_type = KA5857_ID;
                // goto Init;
                break;

            case PIXART_ID: // FCT3065_ID PAW3205DB_ID
            {
                sensor_id  = mouse_sensor_read_reg(1);
                DEBUG("sensor_id1:0x%x", sensor_id);
                sensor_id &= 0xF0;
                DEBUG("sensor_id1:0x%x", sensor_id);

                switch (sensor_id)
                {
                    case PAW3205_ID:
                    {
                        Sensor_type = PAW3205_ID;
                    } break;

                    case MX8650A_ID:
                    {
                        Sensor_type = MX8650A_ID;
                    } break;
                }
            } break;

            default:
                sensor_init_ok_flag = 0;
                break;
        }
        bootDelayUs(1000);
    }

    if (sensor_init_ok_flag == 0)
    {
        DEBUG("Not_read_sensor\r\n");
        NVIC_SystemReset();
    }

    // Init:
    sensor_config = mouse_sensor_read_reg(SENSOR_POWER) | 0x80;

    if (sensor_config & 0x08)
    {
        sensor_config &= (~0x08);
        mouse_sensor_write_reg(SENSOR_POWER, sensor_config);
    }

    sensor_config = mouse_sensor_read_reg(SENSOR_POWER) | 0x80;

    if (sensor_config & 0x08)
    {
        sensor_config &= (~0x08);
        mouse_sensor_write_reg(SENSOR_POWER, sensor_config);
    }

    switch (Sensor_type)
    {
        case FCT3065_ID:
            mouse_sensor_paw3205_init();
            DEBUG("Sensor_FCT3065_ID\r\n");
            break;

        case KA5857_ID:
            DEBUG("Sensor_KA5857_ID\r\n");
            KA5857_ID_Init();
            break;

        case PAW3205_ID:
            DEBUG("Sensor_PAW3205_ID\r\n");
            mouse_sensor_paw3205_init();
            break;

        case MX8650A_ID:
            DEBUG("Sensor_MX8650A_ID\r\n");
            MX8650A_ID_Init();
            break;
        
        default:
            DEBUG("NULL\r\n");
            break;
    }

    CPI_Write();
}

void sensor_rset_on(void)
{
    uint8_t sensor_config = 0;
    mouse_sensor_write_reg(0x06, config_sleep);
    sensor_config = mouse_sensor_read_reg(SENSOR_POWER) | 0x80;
    DEBUG("sensor_config:%d", sensor_config);
    if (sensor_config & 0x08)
    {
        sensor_config &= (~0x08);
        mouse_sensor_write_reg(SENSOR_POWER, sensor_config);
    }

    sensor_config = mouse_sensor_read_reg(SENSOR_POWER) | 0x80;
    DEBUG("sensor_config:%d", sensor_config);
    if (sensor_config & 0x08)
    {
        sensor_config &= (~0x08);
        mouse_sensor_write_reg(SENSOR_POWER, sensor_config);
    }
    DEBUG("sensor_config:%d", sensor_config);
    CPI_Write();
}

void mouse_send_report(const uint8_t *report)
{
    if (app_state_get() < (APP_ENCRYPTED - work_mode))
    {
        return;
    }

#if (CFG_BLE_SFT_WKUP)
    ble_wakeup();
#endif
    
#if (CFG_IO_OBSERVE)
    uint8_t le_sta;
//    GPIO_DAT_SET(GPIO07);
    le_sta = mouse_report_send(app_env.curidx, report);
//    GPIO_DAT_CLR(GPIO07);
    
//    if (le_sta)
//    {
//    }
#else
    //DEBUG("re\r\n");
    mouse_report_send(app_env.curidx, report);
#endif
}

void mouse_enter_powerdown(void)
{
   if(work_mode == B24G_MODE)
   {
        uint8_t all_ff[8]={0xff,0xff,0xff,0xff,0xff,0xff};
        
        if(memcmp(all_ff,dongle_addr,6))
        {
            
           DEBUG("DONGLE_ADDR");
           memcpy(poweroff_store_dongle_addr,dongle_addr,6);
           flash_erase_write(POWEROFF_STORE_DONGLE_MAC_OFFSET,(uint32_t*)&poweroff_store_dongle_addr);     
        }
    }       
    sensor_powerdown();
    
//    GPIO->DIR_CLR = PA(PAD_KSCAN_C0) | PA(PAD_SENSOR_CLK) | PA(PAD_SENSOR_DIO) | 
//                    PA(CHANNEL_SLECT_PIN) | PAD_WHEEL_MSK | PA(4) | PA(6) | PA(7);

    GPIO->DIR_CLR = PA(PAD_KSCAN_C0);
    IOM_CTRL(PAD_KSCAN_C0, IOM_HIZ);

    GPIO->DIR_CLR = PA(PAD_SENSOR_CLK);
    GPIO->DIR_CLR = PA(PAD_SENSOR_DIO); // PAD_SENSOR_DIO;
		
    //if(Sensor_type==PAW3205_ID)
    {
        IOM_CTRL(PAD_SENSOR_CLK, IOM_SEL_GPIO|IOM_PULLUP|IOM_INPUT);
        IOM_CTRL(PAD_SENSOR_DIO, IOM_SEL_GPIO|/*IOM_PULLUP|*/IOM_INPUT);
    }
//    else
//    {
//            IOM_CTRL(PAD_SENSOR_CLK, IOM_HIZ);
//            IOM_CTRL(PAD_SENSOR_DIO, IOM_HIZ);
//    }

	
    GPIO->DIR_CLR = PA(CHANNEL_SLECT_PIN);
    IOM_CTRL(CHANNEL_SLECT_PIN, IOM_HIZ);
    
    GPIO->DAT_CLR = PA(15);//_24G_LED;//led
    
    GPIO->DIR_SET = PA(12); // SA2
    GPIO->DAT_CLR = PA(12);
    // IOM_CTRL(12,IOM_HIZ);

    GPIO->DIR_CLR = PA(4); // ADC
    IOM_CTRL(4, IOM_SEL_GPIO | IOM_HIZ);

    /**ZA ZB*/
    GPIO->DIR_CLR = PA(PAD_WHEEL_ZA) | PA(PAD_WHEEL_ZB);
    IOM_CTRL(PAD_WHEEL_ZA, IOM_HIZ);
    IOM_CTRL(PAD_WHEEL_ZB, IOM_HIZ);
		
		 
    /*led*/
//    GPIO->DIR_CLR = PA(_BT_LED);
//    IOM_CTRL(_BT_LED,IOM_HIZ);

//    GPIO->DIR_CLR = PA(_24G_LED);
//    IOM_CTRL(_24G_LED,IOM_HIZ);

//    GPIO->DIR_CLR = PA(_BATT_LOW_LED);
//    IOM_CTRL(_BATT_LOW_LED,IOM_HIZ);

//    sensor_powerdown();
    
    DEBUG("---enterpoweroff(io:%X)\r\n", GPIO_PIN_GET());
    #if !(DBG_MODE)
    GPIO->DIR_CLR = PA(6) | PA(7); // tx | rx
    IOM_CTRL(6, IOM_HIZ/*|IOM_SEL_GPIO*/);
    IOM_CTRL(7, IOM_HIZ/*|IOM_SEL_GPIO*/);
    #endif
    
    bootDelayUs(10000);

    wakeup_io_sw(PAD_POWEROFF_WEKUP_MSK, PAD_POWEROFF_WEKUP_MSK);

    DEBUG("---enterpoweroff(io:%X)\r\n", GPIO_PIN_GET());
    core_pwroff(CFG_WKUP_IO_EN | WKUP_IO_LATCH_N_BIT);
}

void mouse_data_handle(void)
{
#if (1)
    if (st_mouse_env.mouseTyp != st_mouse_env.mouseTyp0)
    {
        if (st_mouse_env.mouseTyp0 >= MOUSE_BUTTON)
        {
            if ((st_mouse_env.sta & MOUSE_STA_BUTTON) != MOUSE_STA_BUTTON)
            {
                // Release last report
                mouse_send_report(NULL);
            }

            if ((st_mouse_env.mouseTyp == MOUSE_WHEEL)||(st_mouse_env.mouseTyp == MOUSE_XY))
            {
                // new report
                mouse_send_report(st_mouse_env.report);
            }
        }
        else
        {
            if (st_mouse_env.mouseTyp >= MOUSE_BUTTON)
            {
                // new report
                mouse_send_report(st_mouse_env.report);
                
            }
        }

        // backup report
        st_mouse_env.mouseTyp0 = st_mouse_env.mouseTyp;
        memcpy(st_mouse_env.report0, st_mouse_env.report, REP_LEN_MOUSE);
    }
    else if ((st_mouse_env.mouseTyp == MOUSE_XY) || (memcmp(st_mouse_env.report0, st_mouse_env.report, REP_LEN_MOUSE) != 0))
    {
        if (st_mouse_env.mouseTyp >= MOUSE_BUTTON)
        {
            // Press new report
            mouse_send_report(st_mouse_env.report);
        }

        memcpy(st_mouse_env.report0, st_mouse_env.report, REP_LEN_MOUSE);
    }

#else
		if(st_mouse_env.mouseTyp!=MOUSE_STA_NONE)
		{
			
			if(st_mouse_env.sta == MOUSE_STA_BUTTON)
			{
				if(st_mouse_env.mouseTyp0!=st_mouse_env.mouseTyp)
				{
					mouse_send_report(st_mouse_env.report);
					//st_mouse_env.mouseTyp0 = st_mouse_env.mouseTyp;
					//memcpy(st_mouse_env.report0, st_mouse_env.report, REP_LEN_MOUSE);
				}
				else if (memcmp(st_mouse_env.report0, st_mouse_env.report, REP_LEN_MOUSE) != 0)
				{

				// Press new report
						mouse_send_report(st_mouse_env.report);
						
				}
				memcpy(st_mouse_env.report0, st_mouse_env.report, REP_LEN_MOUSE);
			}
			else 
			{
				mouse_send_report(st_mouse_env.report);
			}
			st_mouse_env.mouseTyp0 = st_mouse_env.mouseTyp;
		}
		else
		{
			if(st_mouse_env.mouseTyp0==MOUSE_STA_BUTTON||st_mouse_env.mouseTyp0==MOUSE_STA_XY)
			{
				 mouse_send_report(NULL);
				 st_mouse_env.mouseTyp0 = MOUSE_STA_NONE;
				 wheel_cnt = 0;
			}
			else if(st_mouse_env.mouseTyp0==MOUSE_STA_WHEEL)
			{
				if(wheel_cnt>0)
				{
					wheel_cnt--;
				}
				else 
				{
					mouse_send_report(NULL);
					st_mouse_env.mouseTyp0 = MOUSE_STA_NONE;
				}
			}
			
		}
#endif
}

bool mouse_proc(void)
{
    if(wheel_val==0)
    {
        st_mouse_env.sta      = MOUSE_STA_NONE;
        st_mouse_env.mouseTyp = MOUSE_NONE;
        memset(st_mouse_env.report, 0, REP_LEN_MOUSE);
    }

    
//    if(wheel_val)
//    {
////        st_mouse_env.sta                        |= MOUSE_STA_WHEEL;
////        st_mouse_env.mouseTyp                    = MOUSE_WHEEL;
//        st_mouse_env.report[MOUSE_WHEEL_OFFSET]  = wheel_val;
////        DEBUG("W");
//    }
    key_data = mouse_key_scan();

   // mouse_scroll();
    #if (CFG_SENSOR)
    mouse_move();
    #endif
    //    mouse_io_hiz();
    if (app_state_get() < (APP_ENCRYPTED - work_mode))
    {
        return 1;
    }

    if (key_status_cont < 600)
    {
#if (TIMER_CORR)
        ADTMR1->ICR.UI  = 1;
        ADTMR1->CNT     = 0;
        ADTMR1->CR1.CEN = 1;
#endif
    }

    // update_cnt++;
    if (DPI_DATA_BACK != key_data)
    {
        DPI_DATA_BACK = key_data;

        if (DPI_DATA_BACK == 0x20)
        {
            CPI_Count++;

            if (CPI_Count > 2)
            {
                CPI_Count = 0;
            }
						
            cpi_data=CPI_Count;
          
            flash_erase_write(CPI_DATA_OFFSET, &cpi_data);
            DEBUG("store CPI:%X", cpi_data);
            CPI_Write();
        }
    }

    if (st_mouse_env.mouseTyp == MOUSE_NONE)
    {
        if (++key_status_cont > 5000)
        {
            key_status_cont = 5000;
        }
    }
    else
    {
        //DEBUG("mouseTyp:%d\r\n",st_mouse_env.mouseTyp);
        key_status_cont = 0;
        sys_timeout_cnt = 0;
        g_poweron_key_sta = 0;
    }
//    if(key_status_cont>30)
//    {
//        if( wheel_val)wheel_val=0;
//    }
    if (key_status_cont >= 660)
    {
#if (TIMER_CORR)
        ADTMR1->ICR.UI  = 1;
        ADTMR1->CNT     = 0;
        ADTMR1->CR1.CEN = 0;
#endif
        update_cnt2++;

        if ((updata_parm_data < 2) && (updata_conn_status == 0) &&
            ((conn_intv != 6) || (conn_late != CFG_CONN_LATENCY/*2*/)))
        {
            gapc_update_param(app_env.curidx, &mouse_le_conn_pref2);
            DEBUG("update_param4\r\n");
            update_cnt2 = 0;
            update_count=5;
        }
        else
        {
            if (((conn_intv != 6) || (conn_late != CFG_CONN_LATENCY/*2*/)) && (update_cnt2 > 100) && (updata_conn_status == 0)&&(update_count>0))
            {
                gapc_update_param(app_env.curidx, &mouse_le_conn_pref2);
                DEBUG("update_param5\r\n");
                update_cnt2 = 0;
                update_count--;
            }
        }
        
        update_cnt1      = 0;
        updata_parm_data = 2;
    }
    else if (key_status_cont < 1)
    {
        update_cnt1++;
        update_cnt2 = 0;

        if (update_connect_par_flag) // first connect delay some times go updata
        {
            if ((updata_conn_status == 0) && (update_cnt1 > 100) && ((conn_intv != 6) || (conn_late != 0)))
            {
//                gapc_update_param(app_env.curidx, &mouse_le_conn_pref1);
//                DEBUG("update_param1\r\n");
//                // updata_parm_data = 1;
//                update_cnt1             = 0;
//                update_connect_par_flag = 0;

            }
        }
        else
        {
            if ((updata_conn_status == 0) && ((updata_parm_data == 0) || (updata_parm_data == 2)) &&
                ((conn_intv != 6) || (conn_late != 0)))
            {
                gapc_update_param(app_env.curidx, &mouse_le_conn_pref1);
                DEBUG("update_param2\r\n");
                update_cnt1 = 0;
                update_count=5;
//                #if(TIMER_CORR)
//                ADTMR1->ARR      = 699;
//                #endif
            }
            else
            {
                if (((conn_intv != 6) || (conn_late != 0)) && (update_cnt1 > 100) && (updata_conn_status == 0)&&(update_count>0))
                {
                    gapc_update_param(app_env.curidx, &mouse_le_conn_pref1);
                    DEBUG("update_param3\r\n");
                    update_cnt1 = 0;
                    update_count--;
                }
            }
        }

        updata_parm_data = 1;
        return false;
    }

//    if (key_status_cont < 1)
//    {
//        return false;
//    }

#if (CFG_PWROFF)

    if (st_mouse_env.mouseTyp == MOUSE_NONE)
    {
        ++g_idle_cnt;
    }
    else
    {
        g_idle_cnt = 0;
    }

    if (g_idle_cnt > 800)
    {
        // Sensor_LED_OFF();

        // Sensor_Power_down();

        AON->BACKUP2 = 0;

        // 0: reset pin, 1: PA13
        AON->BKHOLD_CTRL.PIOA13_FUNC_SEL = 0;

        DELAY_US(64);
        core_pwroff(WKUP_IO_LATCH_N_BIT // 锁主支持唤醒IO状态
                    | CFG_WKUP_PWIO_FALL(PA07)
                    /*| CFG_WKUP_PWIO_FALL(PA13)*/
        );
    }
    else if (g_idle_cnt > 400)
    {
        if (!(AON_BKUP2_GET(AON_TIMEOUT_BIT)))
        {
            conn_param_set(CFG_IDLE_CONN_INTV, CFG_IDLE_CONN_LATENCY);
            AON_BKUP2_SET(AON_TIMEOUT_BIT);
#if (TIMER_CORR)
            CTMR1->ARR = 35999;
#endif
        }
    }
    else if (g_idle_cnt < 1)
    {
        if ((AON_BKUP2_GET(AON_TIMEOUT_BIT)))
        {
            AON_BKUP2_CLR(AON_TIMEOUT_BIT);
            AON_BKUP2_CLR(AON_ENTER_PWROFF_BIT);
            conn_param_set(CFG_CONN_INTV, 0);
#if (TIMER_CORR)
            CTMR1->ARR = 699;
#endif
            hyble_slpdur_set(CFG_OSCEN_DPSLP_WKUP, 3200); // 设置核提前起来时间
        }

        return false;
    }

#endif // CFG_PWROFF

    return true;
}

//static tmr_tk_t mouse_scan_handle(tmr_id_t id)
//{
//    if (id == mouse_scan_time_id)
//    {
//        mouse_proc();
//    }

//    return (MOUSE_SCAN_INTEV);
//}

//void mose_scan_init(void)
//{
//    mouse_io_init();
//    mouse_sensor_init();

//    if (mouse_scan_time_id != TMR_ID_NONE)
//    {
//        sftmr_clear(mouse_scan_time_id);
//        mouse_scan_time_id = TMR_ID_NONE;
//    }

//    mouse_scan_time_id = sftmr_start(MOUSE_SCAN_INTEV, mouse_scan_handle);
//}

__INLINE__ void BTaddr_Add_One(void)
{
//    bt_addr[channle_slect] += 1;
//    memcpy((uint8_t *)&ble_dev_addr, (uint8_t *)&bt_addr[channle_slect], 4);
    bt_addr[0] += 1;
    memcpy((uint8_t *)&ble_dev_addr, (uint8_t *)&bt_addr[0], 4);
}

void read_bt_mac_ltk_info(uint16_t rsn)
{
    uint8_t i = 0;
    
    g_poweron_key_sta = mouse_key_scan();
    DEBUG("g_poweron_key_sta:%d", g_poweron_key_sta);
    uint32_t store_work_mode = RD_32(FLASH_BASE + WORK_MODE_OFFSET);
    
    work_mode = BT_MODE;
    channle_slect = 0;
    g_sync_word = SYNC_WORD_BLE;
    
    gpio_dir_input(CHANNEL_SLECT_PIN, IE_UP);

    cpi_data = RD_32(FLASH_BASE + CPI_DATA_OFFSET);
    
    if (cpi_data == 0xFFFFFFFF)
    {
        CPI_Count = 1;
    }
    else
    {
        CPI_Count = (cpi_data & 0xFF);
    }

    DEBUG("CPI_Count:%d", CPI_Count);

//    fshc_read(BT_MAC_STORE_OFFSET, bt_addr, 2, FSH_CMD_RD);
    flash_read(BT_MAC_STORE_OFFSET, bt_addr, 2);
    
    if (bt_addr[BT_ADDR_0] == 0xFFFFFFFF)
    {
        memcpy((uint8_t *)&bt_addr[BT_ADDR_0], (uint8_t *)&ble_dev_addr, 4);
        
        bt_addr[BT_ADDR_1] = bt_addr[BT_ADDR_0];
        
        flash_erase_write(BT_MAC_STORE_OFFSET, bt_addr);
    }
    
    if (bt_addr[BT_ADDR_1] == 0xFFFFFFFF)
    {
        //memcpy((uint8_t *)&bt_addr[BT_ADDR_1], (uint8_t *)&ble_dev_addr, 4);
        bt_addr[BT_ADDR_1] = bt_addr[BT_ADDR_0];
        flash_erase_write(BT_MAC_STORE_OFFSET, bt_addr);
    }
    
    // 判断是否写成功.
//    fshc_read(BT_MAC_STORE_OFFSET, bt_addr, 2, FSH_CMD_RD);
    flash_read(BT_MAC_STORE_OFFSET, bt_addr, 2);
    
    DEBUG("addr0");
    debugHex((uint8_t *)&bt_addr[BT_ADDR_0], 4);
    DEBUG("addr1");
    debugHex((uint8_t *)&bt_addr[BT_ADDR_1], 4);
    memcpy((uint8_t *)&ble_dev_addr, (uint8_t *)&bt_addr[BT_ADDR_0], 4);
    DEBUG("store_wore_mode:%X", store_work_mode);

#if (CFG_SOP16_VDD12)
    if (B24G_MODE == store_work_mode)
    {
        work_mode = B24G_MODE;
        
        channle_slect = 1;
        
        memcpy((uint8_t *)&ble_dev_addr, (uint8_t *)&bt_addr[BT_ADDR_0], 4);
        
        poweron_work_status = SYS_CONNECT_BACK;
        
        g_sync_word = SYNC_WORD_2G4;
    }
    
    if (rsn == RSN_POR12_BK_BIT) 
    {
        DEBUG("key_sta:%x", key_sta);
        
        if ((BTN_L|BTN_R) == key_sta)
        {
            work_mode = B24G_MODE;
            
            channle_slect = 1;
            
            memcpy((uint8_t *)&ble_dev_addr, (uint8_t *)&bt_addr[BT_ADDR_0], 4);

            poweron_work_status = SYS_CONNECT_BACK;
            
            g_sync_word = SYNC_WORD_2G4;
            
            if (store_work_mode != work_mode)
            {
                flash_erase_write(WORK_MODE_OFFSET, &work_mode);
            }
            
            return;
        }
        else if (BTN_DPI == key_sta)
        {
            if (store_work_mode != 0xFFFFFFFF)
            {
                flash_erase_write(WORK_MODE_OFFSET, NULL);
            }

            work_mode = BT_MODE;
            channle_slect = 0;
            g_sync_word = SYNC_WORD_BLE;
        }
    }
    
    if (work_mode == B24G_MODE)
    {
        DEBUG("g_sync_word:%x, word_sta:%d, word_mode:%d", g_sync_word, poweron_work_status, work_mode);
        return;
    }
#else
    // Todo...
    #if(CHANNLE_SELECT_EN)
    if (!gpio_get(CHANNEL_SLECT_PIN))
    #endif    
    {
        work_mode = B24G_MODE;
        DEBUG("B24G_MODE");
        channle_slect = 1;
        
        memcpy((uint8_t *)&ble_dev_addr, (uint8_t *)&bt_addr[BT_ADDR_1], 4);

        poweron_work_status = SYS_CONNECT_BACK;
        
        g_sync_word = SYNC_WORD_2G4;

        #if(_24G_PARING_ENABLE)
        flash_read(DONGLE_MAC_OFFSET,(uint32_t*)&dongle_addr,2);
        DEBUG("dongle_adddr");
        debugHex(dongle_addr, 8);
        if (rsn == RSN_POR12_BK_BIT) 
        {
            if ((BTN_L|BTN_R) == g_poweron_key_sta)
            {
                memset(dongle_addr,0xff,8);
                flash_erase_write(DONGLE_MAC_OFFSET,NULL);
                
            }
        }
        #else
        if((rsn&RSN_POR12_CORE_BIT)==RSN_POR12_CORE_BIT)
        {
            flash_read(POWEROFF_STORE_DONGLE_MAC_OFFSET,(uint32_t*)&poweroff_store_dongle_addr,2);
            DEBUG("dongle_addr");
            debugHex(poweroff_store_dongle_addr, 6);
            memcpy(dongle_addr,poweroff_store_dongle_addr,6);
        }
        else if (rsn == RSN_POR12_BK_BIT) 
        {
            #if (LED_PLAY)
            leds_play(LED_PARING);
            #endif
            flash_read(POWEROFF_STORE_DONGLE_MAC_OFFSET,(uint32_t*)&poweroff_store_dongle_addr,2);
            if((poweroff_store_dongle_addr[0]!=0xff)||(poweroff_store_dongle_addr[1]!=0xff)
                ||(poweroff_store_dongle_addr[2]!=0xff)||(poweroff_store_dongle_addr[3]!=0xff)
                ||(poweroff_store_dongle_addr[4]!=0xff)||(poweroff_store_dongle_addr[5]!=0xff))           
                flash_erase_write(POWEROFF_STORE_DONGLE_MAC_OFFSET, NULL);
        }
        #endif
        return;
        
    }
#endif
    #if(ONLY_24G_MODE)
    return;
    #endif
    DEBUG("channle_slect:%d", channle_slect);
    //ble_dev_addr.addr[5] += channle_slect;
    DEBUG("ble_dev_addr");
    debugHex(ble_dev_addr.addr, 6);

//    fshc_read(LTK_STORE_OFFSET, (uint32_t *)&ltk_data, 14, FSH_CMD_RD);
    flash_read(LTK_STORE_OFFSET, (uint32_t *)&ltk_data, BLEN2WLEN(sizeof(ltk_data)));
//    memcpy((uint8_t *)&gLTK, &ltk_data[channle_slect * 28], 28);
    
    debugHex((uint8_t *)&gLTK, 28);
    debugHex((uint8_t *)&ltk_data, 56);

    for (i = 0; i < 28; i++)
    {
        if (ltk_data[(channle_slect * 28) + i] != 0xFF)
        {
            break;
        }
    }

    DEBUG("i____:%d", i);
    poweron_work_status = (i == 28) ? SYS_IDLE : SYS_CONNECT_BACK;

    // 上电模式
    if ((work_mode == BT_MODE) && (rsn == RSN_POR12_BK_BIT) && (BTN_DPI == g_poweron_key_sta))
    {
        poweron_work_status = SYS_PARING;
        
        BTaddr_Add_One();
        
#if (LED_PLAY)
        leds_play(LED_PARING);
#endif
        DEBUG("DPI_DOWEN");
    }

    DEBUG("g_sync_word:%x, word_sta:%d, word_mode:%d", g_sync_word, poweron_work_status, work_mode);
}

__SRAMFN void flash_erase_write(uint32_t offset, uint32_t *data)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
    fshc_erase(offset, FSH_CMD_ER_PAGE);
    
    if (data != NULL)
    {
        fshc_write(offset, data, 64, FSH_CMD_WR);
    }

    GLOBAL_INT_RESTORE();
}

//__SRAMFN void flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen)
//{
//    GLOBAL_INT_DISABLE();
//    
//    while (SYSCFG->ACC_CCR_BUSY);

//    fshc_read(offset, buff, wlen, FSH_CMD_RD);

//    GLOBAL_INT_RESTORE();
//}
