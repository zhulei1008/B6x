#include "regs.h"
#include "fpga_spi_rf.h"

void rf_spi_tf(void);
uint32_t rf_reg_rd (uint32_t addr);
void rf_reg_wr (uint32_t addr, uint32_t value);

#define RF_SPI_WR(addr, val)    rf_reg_wr(((addr)), (val))
#define RF_SPI_RD(addr)         rf_reg_rd((addr))

#define PLL_DAC_ADJ0_POS 0
#define PLL_DAC_ADJ1_POS 5
#define PLL_DAC_ADJ2_POS 10
#define PLL_DAC_ADJ3_POS 15
#define PLL_DAC_ADJ4_POS 20
#define PLL_DAC_ADJ5_POS 25

void rf_mdm_spi_init(void)
{
    MDM->REG0.TX_SCALE_EN      = 1;
    
    #if (MDM_EXT_CTRL_ADDR_OFFSET)
    MDM->REG0.TX_SCALE_COEF_1M = 6;
    MDM->REG0.TX_SCALE_COEF_2M = 6;
    #else
    MDM->REG0.TX_SCALE_COEF    = 6;
    #endif
}

void rf_reg_spi_init(void)
{
    rf_mdm_spi_init();
    
    uint32_t reg_val = RF_SPI_RD(RF_PLL_GAIN_CTRL_ADDR_OFFSET);
    reg_val &= ~0x1FUL;
    
    // 1Mbps, chan37,38,39
//    reg_val |= 0x0A; //RF->PLL_GAIN_CTRL.PLL_VTXD_EXT

    RF_SPI_WR(RF_PLL_GAIN_CTRL_ADDR_OFFSET, reg_val);
    
    reg_val = RF_SPI_RD(RF_ANAMISC_CTRL1_ADDR_OFFSET);
    reg_val &= ~((0x3UL << 11/*DAC_REFH_ADDJ, DAC_REFH_ADJ*/) | (0x07 << 5/*DAC_REFL_ADJ*/));
    reg_val |= (/*(0x03UL << 11) | */(0x04UL << 5));
    RF_SPI_WR(RF_ANAMISC_CTRL1_ADDR_OFFSET, reg_val);
    
    reg_val = RF_SPI_RD(RF_PLL_DAC_TAB_ADDR_OFFSET);
    reg_val = (14 << PLL_DAC_ADJ0_POS) | // chan 0 --- 6
              (15 << PLL_DAC_ADJ1_POS) | // chan 7 --- 14
              (10 << PLL_DAC_ADJ2_POS) | // chan 15 --- 22
              (14 << PLL_DAC_ADJ3_POS) | // chan 23 --- 30
              (10 << PLL_DAC_ADJ4_POS) | // chan 31 --- 38
              (11 << PLL_DAC_ADJ5_POS);   // chan 39
    
    RF_SPI_WR(RF_PLL_DAC_TAB_ADDR_OFFSET, reg_val);
}
