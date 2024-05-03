#include "string.h"
#include "b6x.h"
#include "reg_rf_c.h"
#include "dbg.h"


/*
 * Utils Function
 ****************************************************************************************
 */

static inline void nopDelay(uint16_t x)
{
    while (x--)
    {
        __NOP();
        __NOP();
    }
}

static inline uint16_t read16p(void const *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}

static inline uint32_t read32p(void const *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = read16p(ptr32);
    addr_h = read16p((uint8_t *)ptr32 + 2);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

static inline void write32p(void const *ptr32, uint32_t value)
{
    uint8_t *ptr = (uint8_t *)ptr32;

    *ptr++ = (uint8_t)(value & 0xff);
    *ptr++ = (uint8_t)((value & 0xff00) >> 8);
    *ptr++ = (uint8_t)((value & 0xff0000) >> 16);
    *ptr = (uint8_t)((value & 0xff000000) >> 24);
}


/*
 * RF REG(rd wr)
 ****************************************************************************************
 */

// EM RF SPI address
#define EM_BASE_ADDR               (0x20008000)
#define EM_RF_SW_SPI_OFFSET        (0x128)
#define RF_EM_SPI_ADRESS           (EM_BASE_ADDR + EM_RF_SW_SPI_OFFSET)

#define RPL_SPIRD                   0x00
#define RPL_SPIWR                   0x80
#define EM_RF_SW_SPI_SIZE_MAX       8
#define BLE_RADIOCNTL0_ADDR         0x50000070
#define BLE_RADIOCNTL1_ADDR         0x50000074

/// Macro to read a BLE register
#define REG_BLE_RD(addr)             (*(volatile uint32_t *)(addr))

/// Macro to write a BLE register
#define REG_BLE_WR(addr, value)      (*(volatile uint32_t *)(addr)) = (value)

static inline void ble_radiocntl0_spigo_setf(uint8_t spigo)
{
    REG_BLE_WR(BLE_RADIOCNTL0_ADDR, (REG_BLE_RD(BLE_RADIOCNTL0_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)spigo << 0));
}

static inline uint8_t ble_radiocntl0_spicomp_getf(void)
{
    uint32_t localVal = REG_BLE_RD(BLE_RADIOCNTL0_ADDR);
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}

static inline uint16_t ble_radiocntl0_spiptr_getf(void)
{
    uint32_t localVal = REG_BLE_RD(BLE_RADIOCNTL0_ADDR);
    return ((localVal & ((uint32_t)0x3FFF0000)) >> 16);
}

static inline void ble_radiocntl0_pack(uint16_t spiptr, uint8_t spicfg, uint8_t spifreq, uint8_t spigo)
{
    REG_BLE_WR(BLE_RADIOCNTL0_ADDR,  ((uint32_t)spiptr << 16) | ((uint32_t)spicfg << 7) | ((uint32_t)spifreq << 4) | ((uint32_t)spigo << 0));
}

static inline void ble_radiocntl1_pack(uint8_t forceagcen, uint8_t forceiq, uint8_t rxdnsl, uint8_t txdnsl, uint16_t forceagclength, uint8_t syncpulsemode, uint8_t syncpulsesrc, uint8_t dpcorren, uint8_t jefselect, uint8_t xrfsel, uint8_t subversion)
{
    REG_BLE_WR(BLE_RADIOCNTL1_ADDR,  ((uint32_t)forceagcen << 31) | ((uint32_t)forceiq << 30) | ((uint32_t)rxdnsl << 29) | ((uint32_t)txdnsl << 28) | ((uint32_t)forceagclength << 16) | ((uint32_t)syncpulsemode << 15) | ((uint32_t)syncpulsesrc << 14) | ((uint32_t)dpcorren << 13) | ((uint32_t)jefselect << 12) | ((uint32_t)xrfsel << 4) | ((uint32_t)subversion << 0));
}

void rf_core_init(void)
{
    /* BLE RADIOCNTL0 */
    ble_radiocntl0_pack(/*uint16_t spiptr*/   EM_RF_SW_SPI_OFFSET >> 2,
            /*uint8_t  spicfg*/   0,
            /*uint8_t  spifreq*/  0,
            /*uint8_t  spigo*/    0);

    /* BLE RADIOCNTL1 */
    ble_radiocntl1_pack(/*uint8_t  forceagcen*/      0,
            /*uint8_t  forceiq*/         0,
            /*uint8_t  rxdnsl*/          0,
            /*uint8_t  txdnsl*/          0,
            /*uint16_t forceagclength*/  0,
            /*uint8_t  syncpulsemode*/   0,
            /*uint8_t  syncpulsesrc*/    1,
            /*uint8_t  dpcorren*/        0,
            /*uint8_t  jefselect*/       1,
    #if (FPGA_TEST)
            /*uint8_t  xrfsel*/          1,
    #else
            /*uint8_t  xrfsel*/          2,
    #endif
            /*uint8_t  subversion*/      0); //0x00005020
}

void rf_spi_tf(void)
{
    //launch SPI transfer
    ble_radiocntl0_spigo_setf(1);

    //wait for transfer to be completed
    while (!ble_radiocntl0_spicomp_getf());
}

uint32_t em_spi_addr = 0;

uint32_t rf_reg_rd (uint32_t addr)
{
    uint8_t buf[EM_RF_SW_SPI_SIZE_MAX];
    uint32_t ret;
    
    em_spi_addr = EM_RF_SW_SPI_OFFSET;
    
    //copy control and number of u32 to send
    buf[0] = (uint8_t)RPL_SPIRD;

    //copy address
    buf[1] = (uint8_t)(addr);

    memcpy((void *)RF_EM_SPI_ADRESS, buf, 2);
    
    //ble_radiocntl0_spiptr_setf((uint16_t)(EM_RF_SW_SPI_OFFSET>>2));
    
    //do the transfer
    rf_spi_tf();

    //read back the buffer - 4 bytes register value MSB in buf[0]
    memcpy(buf, (void *)(RF_EM_SPI_ADRESS + 4), 4);
    ret = read32p(&buf[0]);

    return ret;
}

void rf_reg_wr (uint32_t addr, uint32_t value)
{
    uint8_t buf[EM_RF_SW_SPI_SIZE_MAX];

    //inversion for EM reading by U8 on BJ SPI side
    //copy control and number of u32 to send
    buf[0] = (uint8_t)RPL_SPIWR;
    //copy address
    buf[1] = (uint8_t)(addr);
    buf[2] = 0;
    buf[3] = 0;
    //on old implementations (BT core 3.0, BLE core 1.0) swap the data
    write32p(&buf[4], value);

    memcpy((void *)RF_EM_SPI_ADRESS, buf, 8);

    //do the transfer
    rf_spi_tf();

    ble_radiocntl0_spiptr_getf();
}


/*
 * RF PLL(Tx Rx) 
 ****************************************************************************************
 */

#define RF_SPI_WR(addr, val)    rf_reg_wr(((addr)), (val))
#define RF_SPI_RD(addr)         rf_reg_rd((addr))

const uint32_t g_rx_pll_frach[18] = 
{
    0x000000, 0x0E38E3, 0x1C71C7, 0x2AAAAA, 0x38E38E, 0x471C71,
    0x555555, 0x638E38, 0x71C71C, 0x800000, 0x8E38E3, 0x9C71C7,
    0xAAAAAA, 0xB8E38E, 0xC71C71, 0xD55555, 0xE38E38, 0xF1C71C
};

void tx_pll_1m_test(uint8_t pll_dis, uint8_t pll_frach)
{
    uint32_t reg_val = ((pll_dis << 24) | (pll_frach << 20));
    
    RF_SPI_WR(RF_DIG_CTRL_ADDR_OFFSET,      0x00105299);
    RF_SPI_WR(RF_PLL_DYM_CTRL_ADDR_OFFSET,  0x3C00);
    RF_SPI_WR(RF_PLL_ANA_CTRL_ADDR_OFFSET,  0x7180FE00);
    RF_SPI_WR(RF_PLL_FREQ_CTRL_ADDR_OFFSET, reg_val);
    RF_SPI_WR(RF_PLL_DYM_CTRL_ADDR_OFFSET,  0x3D00);
}

void rx_pll_1m_test(uint8_t pll_dis, uint8_t pll_frach)
{
    uint32_t reg_val = ((pll_dis << 24) | g_rx_pll_frach[pll_frach]);
    
    RF_SPI_WR(RF_DIG_CTRL_ADDR_OFFSET,      0x00105299);
    RF_SPI_WR(RF_PLL_DYM_CTRL_ADDR_OFFSET,  0x3C00);
    RF_SPI_WR(RF_PLL_ANA_CTRL_ADDR_OFFSET,  0x3180FE00);
    RF_SPI_WR(RF_PLL_FREQ_CTRL_ADDR_OFFSET, reg_val);
    RF_SPI_WR(RF_PLL_DYM_CTRL_ADDR_OFFSET,  0x3E00);
}


/*
 * RF Test 
 ****************************************************************************************
 */

void rfRegPrint(void)
{
    for (uint8_t i = 0; i < 0x15; i++)
    {
        debug("reg%02x=0x%08X\r\n", i<<2, rf_reg_rd(i<<2));
    }
}

void rfpllTest(void)
{
    rf_core_init();
    
    rfRegPrint();
    
    tx_pll_1m_test(0x15, 0xE); // tx 2414M
    
    while (1);
}
