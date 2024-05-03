#include "drvs.h"
#include "reg_csc.h"
#include "gpio_test.h"

#if (CFG_TEST)

#define IOM_HIGHZ_MOD     0x00  //IO Mod: high impedance(default)
#define IOM_IE_MOD        0x100 //IO Mod: input
#define IOM_IE_DOWN_MOD   0x140 //IO Mod: input pull dwon
#define IOM_IE_UP_MOD     0x180 //IO Mod: input pull up

// *Note: All GPIO-TEST need open input mode
static void gpioInputMode(uint32_t io_mask, uint32_t mode)
{
    //    IOMCTL->SWDCLK.Word = IOM_HIGHZ_MOD;
    //    IOMCTL->SWDIO.Word  = IOM_HIGHZ_MOD;

    for (uint8_t io = 0; io < IO_MAX_CNT; io++)
    {
        if (io_mask & (1UL << io))
        {
            CSC->CSC_PIO[io].Word = mode;
        }
    }
}

// *Return io_sta(0: Pass, other: Error Bits)
static uint32_t gpioShortTest(uint32_t io_mask, uint8_t short_type)
{
    uint32_t io_sta;

    gpioInputMode(io_mask, IOM_IE_MOD); // Input mode
    GPIO_DIR_SET(io_mask); // Output mode
    
//    gpioDataSel(io_mask, short_type); // Set output
    if (short_type)
        GPIO_DAT_SET(io_mask);
    else
        GPIO_DAT_CLR(io_mask);

    io_sta = GPIO_PIN_STA_GET() & io_mask; // Read Input
    if (short_type == SHORT_GND)
    {
        io_sta ^= io_mask; // Output 1, XOR
    }

    return io_sta;
}

static uint32_t gpioPullTest(uint32_t io_mask, uint8_t pull_type)
{
    uint32_t io_sta;
    uint32_t io_mode = (pull_type == PULL_UP) ? IOM_IE_UP_MOD : IOM_IE_DOWN_MOD;

    GPIO_DIR_CLR(io_mask); // Output Disable
    gpioInputMode(io_mask, io_mode); // Input with Pull mode

    io_sta = GPIO_PIN_STA_GET() & io_mask; // Read Input
    if (pull_type == PULL_UP)
    {
        io_sta ^= io_mask; // Pull Up, XOR
    }

    return io_sta;
}

static uint32_t gpioNearTest(uint32_t io_mask)
{
    uint32_t io_sta = 0;

    gpioInputMode(io_mask, IOM_IE_MOD); // Input mode
    GPIO_DIR_SET(io_mask); // Output mode

    for (uint8_t io = 0; io < IO_MAX_CNT; io++)
    {
        if (io_mask & (1UL << io))
        {
            GPIO_DAT_SET(io_mask); // First Output All High
            GPIO_DAT_CLR(1UL << io); // Then Output 'io' to Low
            
            __NOP();__NOP();  // Sram 16M
            
            io_sta |= (GPIO_PIN_STA_GET() & (1UL << io)); // 'io' be Low Pass, else Fail
        }
    }

    return io_sta;
}

uint8_t gpio_test(uint32_t *masks, uint8_t modes)
{
    uint32_t io_stat;
    uint32_t io_mask = *masks;

    if (modes & IO_TEST_SHORTGND)
    {
        io_stat = gpioShortTest(io_mask, SHORT_GND);
        if (io_stat)
        {
            *masks = io_stat;
            return IO_TEST_SHORTGND;
        }
    }

    if (modes & IO_TEST_SHORTVDD)
    {
        io_stat = gpioShortTest(io_mask, SHORT_VDD);
        if (io_stat)
        {
            *masks = io_stat;
            return IO_TEST_SHORTVDD;
        }
    }

    if (modes & IO_TEST_PULLUP)
    {
        io_stat = gpioPullTest(io_mask, PULL_UP);
        if (io_stat)
        {
            *masks = io_stat;
            return IO_TEST_PULLUP;
        }
    }

    if (modes & IO_TEST_PULLDOWN)
    {
        io_stat = gpioPullTest(io_mask, PULL_DOWN);
        if (io_stat)
        {
            *masks = io_stat;
            return IO_TEST_PULLDOWN;
        }
    }

    if (modes & IO_TEST_NEAR)
    {
        io_stat = gpioNearTest(io_mask);
        if (io_stat)
        {
            *masks = io_stat;
            return IO_TEST_NEAR;
        }
    }

    return IO_TEST_PASS;
}

#endif //(CFG_TEST)
