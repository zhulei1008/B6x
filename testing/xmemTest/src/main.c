/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

void sysInit(void)
{    
    #if (FPGA_TEST)
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 2; // BB Must 16M
    #else
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV    = 0; // BB Must 16M
    #endif
}

void projectCodeInfo(void)
{
    extern uint32_t Load$$ER_IROM1$$Base;
    extern uint32_t Image$$ER_IROM1$$Base;
    extern uint32_t Load$$ER_IROM1$$Length;
    
    uint32_t *load_base  = (uint32_t*)&Load$$ER_IROM1$$Base;
    uint32_t *image_base = (uint32_t*)&Image$$ER_IROM1$$Base;
    uint32_t *load_len   = (uint32_t*)&Load$$ER_IROM1$$Length;

    debug("flash_info 0x0000:%X, %X, %X, %X\r\n", RD_32(0x18000000), RD_32(0x18000004), RD_32(0x18000008), RD_32(0x1800000C));
    debug("flash_info 0x1000:%X, %X, %X, %X\r\n", RD_32(0x18001000), RD_32(0x18001004), RD_32(0x18001008), RD_32(0x1800100C));

    debug("BACKUP0:%x, SYS_BACKUP0:%08x\r\n", AON->BACKUP0, SYSCFG->SYS_BACKUP0);
    
    if ((AON->BACKUP0 & 0x1FUL) != 0)
        AON->BACKUP0 &= ~0x11FUL;

    if ((SYSCFG->SYS_BACKUP0 & 0x3FUL) != 0)
        SYSCFG->SYS_BACKUP0 &= ~0x3FUL;

    debug("LoadBase: %08x\r\n", (uint32_t)load_base);
    debug("ImageBase:%08x\r\n", (uint32_t)image_base);
    debug("LoadLen:  %x\r\n",   (uint32_t)load_len);
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    #if (CACHE_ENABLE)
    fshc_cache_conf(0x18008000);
    #endif
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
//    projectCodeInfo();
}
#undef debugHex
#define debugHex(dat,len)     do{                                 \
                                  for (int i=0; i<len; i++){      \
                                      debug("%02X", *((dat)+i)); \
                                  }                               \
                                  debug("\r\n");                  \
                              } while (0)
#define BUFF_LEN 4096

void xmemcpy_test(void)
{
    uint8_t dst[BUFF_LEN + 3] = {1};
    uint8_t *p_src = (uint8_t*)(0x00);
    uint8_t *p_dst = dst;
    uint16_t len = BUFF_LEN;
    
    debug("***********dst&src word align********\r\n");
    debug("xmemcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len+1);
    debug("**************************\r\n");
    debug("memcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len+1);
    
#if (0)
    debug("***********dst Odd********\r\n");
    p_dst = dst + 1;
    debug("xmemcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
    
    debug("***********dst half word********\r\n");
    p_dst = dst + 2;
    debug("xmemcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
    
    debug("***********src Odd********\r\n");
    p_dst = dst;
    p_src = (uint8_t*)(0x01);
    len = BUFF_LEN - 1;
    
    debug("xmemcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
    
    debug("***********src half word********\r\n");
    p_dst = dst;
    p_src = (uint8_t*)(0x02);
    len = BUFF_LEN - 2;
    
    debug("xmemcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memcpy[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memcpy(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
#endif
}

void xmemset_test(void)
{
    uint8_t dst[BUFF_LEN + 3] = {1};
    uint8_t *p_dst = dst;
    uint16_t len = BUFF_LEN;
    
//#undef debugHex
//#define debugHex(dat,len)
    
    debug("***********dst&src word align********\r\n");
    debug("xmemset[dst:%x, len:%d]\r\n", (uint32_t)p_dst, len);
    GPIO->DAT_SET = 0x100;
    xmemset(p_dst, 0x31, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memset[dst:%x, len:%d]\r\n", (uint32_t)p_dst, len);
    GPIO->DAT_SET = 0x200;
    memset(p_dst, 0x32, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
    
    debug("***********dst Odd********\r\n");
    p_dst = dst + 1;
    debug("xmemset[dst:%x, len:%d]\r\n", (uint32_t)p_dst, len);
    GPIO->DAT_SET = 0x100;
    xmemset(p_dst, 0x33, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memset[dst:%x, len:%d]\r\n", (uint32_t)p_dst, len);
    GPIO->DAT_SET = 0x200;
    memset(p_dst, 0x34, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
    
    debug("***********dst half word********\r\n");
    p_dst = dst + 2;
    debug("xmemset[dst:%x, len:%d]\r\n", (uint32_t)p_dst, len);
    GPIO->DAT_SET = 0x100;
    xmemset(p_dst, 0x35, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, len);
    debug("**************************\r\n");
    debug("memset[dst:%x, len:%d]\r\n", (uint32_t)p_dst, len);
    GPIO->DAT_SET = 0x200;
    memset(p_dst, 0x36, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, len);
}

void xmemcmp_test(void)
{
    uint8_t dst[BUFF_LEN + 3] = {1};
    uint8_t *p_src = (uint8_t*)(0x00);
    uint8_t *p_dst = dst;
    uint16_t len = BUFF_LEN;
    int cmp_res = 0;
    
    xmemcpy(p_dst, p_src, len);
#if (0)  
    debug("***********dst&src word align********\r\n");
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);
    
    debug("***********dst Odd********\r\n");
    p_dst = dst + 1;
    xmemcpy(p_dst, p_src, len);
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);

    debug("***********dst half word********\r\n");
    p_dst = dst + 2;
    xmemcpy(p_dst, p_src, len);
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);
   
    debug("***********src Odd********\r\n");
    p_dst = dst;
    p_src = (uint8_t*)(0x01);
    len = BUFF_LEN - 1;
    xmemcpy(p_dst, p_src, len);
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);
    
    debug("***********src half word********\r\n");
    p_dst = dst;
    p_src = (uint8_t*)(0x02);
    len = BUFF_LEN - 2;
    xmemcpy(p_dst, p_src, len);
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);
#endif    
    debug("***********first byte diff********\r\n");
    debug("***********first byte diff********\r\n");
    p_dst = dst;
    p_src = (uint8_t*)(0x00);
    len = BUFF_LEN;
    xmemcpy(p_dst, p_src, len);
    dst[0] = 0x01;
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);
    
    debug("***********dst Odd********\r\n");
    p_dst = dst + 1;
    xmemcpy(p_dst, p_src, len);
    *p_dst = 0x01;
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);

    debug("***********dst half word********\r\n");
    p_dst = dst + 2;
    xmemcpy(p_dst, p_src, len);
    *p_dst = 0x01;
    debug("xmemcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    cmp_res = xmemcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debug("cmp_res:%x\r\n", cmp_res);
    debug("**************************\r\n");
    debug("memcmp[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    cmp_res = memcmp(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debug("cmp_res:%x\r\n", cmp_res);
}


void xmemove_test(void)
{
    uint8_t dst[BUFF_LEN + 3] = {1};
    uint8_t *p_src = dst + 128;
    uint8_t *p_dst = dst;
    uint16_t len = 128;
    uint16_t i = 0;
    
    for (i = 0; i < 0x100; ++i)
    {
        dst[i] = i;
    }
    debugHex(p_dst, 256);
    debug("***********dst&src word align********\r\n");
    debug("xmemmove[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemmove(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, 256);
    debug("**************************\r\n");
    for (i = 0; i < 0x100; ++i)
    {
        dst[i] = i;
    }
    debugHex(p_dst, 256);
    debug("memmove[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memmove(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, 256);
    
    debug("**************************\r\n");
    debug("**************************\r\n");
    
    for (i = 0; i < 0x100; ++i)
    {
        dst[i] = i;
    }
    debugHex(p_dst, 256);
    debug("***********dst Odd********\r\n");
    p_dst = dst + 1;
    debug("xmemmove[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x100;
    xmemmove(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x100;
    debugHex(p_dst, 256);
    debug("**************************\r\n");
    for (i = 0; i < 0x100; ++i)
    {
        dst[i] = i;
    }
    debugHex(p_dst, 256);
    debug("memmove[dst:%x, src:%x, len:%d]\r\n", (uint32_t)p_dst, (uint32_t)p_src, len);
    GPIO->DAT_SET = 0x200;
    memmove(p_dst, p_src, len);
    GPIO->DAT_CLR = 0x200;
    debugHex(p_dst, 256);
}


void xmem_test(void)
{
    xmemcpy_test();
//    xmemset_test();
//    xmemcmp_test();
//    xmemove_test();
}

int main(void)
{
    sysInit();
    devInit();
    
    GPIO->DAT_CLR = 0x300;
    GPIO->DIR_SET = 0x300;
    
    GLOBAL_INT_START();
    xmem_test();
    while(1)
    {
    }
}
