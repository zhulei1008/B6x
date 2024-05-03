/**
 ****************************************************************************************
 *
 * @file reg_access.h
 *
 * @brief File implementing the basic primitives for register accesses
 *
 * Copyright (C) 2019. HungYi Microelectronics Co.,Ltd
 *
 *
 ****************************************************************************************
 */

#ifndef REG_ACCESS_H_
#define REG_ACCESS_H_

/**
 ****************************************************************************************
 * @addtogroup REG REG_ACCESS
 * @ingroup DRIVERS
 *
 * @brief Basic primitives for register access
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "string.h"            // string functions

#if defined(CFG_EMB)
#include "utils.h"
#include "em_map.h"       // EM Map
#endif // defined(CFG_EMB)
/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * MACROS
 ****************************************************************************************
 */
/// Macro to read a platform register
#define REG_PL_RD(addr)              (*(volatile uint32_t *)(addr))

/// Macro to write a platform register
#define REG_PL_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a common ip register
#define REG_IP_RD(addr)              (*(volatile uint32_t *)(addr))

/// Macro to write a common ip register
#define REG_IP_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a BLE register
#define REG_BLE_RD(addr)             (*(volatile uint32_t *)(addr))

/// Macro to write a BLE register
#define REG_BLE_WR(addr, value)      (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a BLE control structure field (16-bit wide)
/**********************************************************************************************/
// Comment and ADD. 2021.9.16 --- wq.
#define EM_BLE_RD(addr)              (*(volatile uint16_t *)(addr))
//#include <stdio.h>
//#include <stdint.h>
//#include "compiler.h"
//extern uint32_t em_ble_rd_cnt;
//extern uint32_t em_ble_wr_cnt;

//__INLINE__ uint16_t EM_BLE_RD(uint32_t addr)  
//{
//    volatile uint16_t temp1 = 0, temp2 = 0;
//    do 
//    {
//        temp1 = (*(volatile uint16_t *)(addr));
//        temp2 = (*(volatile uint16_t *)(addr));
//        if (temp1 != temp2)
//        {
//            em_ble_rd_cnt++;
//            printf("EM_BLE_RD(addr:0x%X,temp1:0x%X,temp2:0x%X,CNT:%d)\r\n", addr, temp1, temp2, em_ble_rd_cnt);
//        }        
//    } while(temp1 != temp2); 
//    
//    return temp1;
//}

/// Macro to write a BLE control structure field (16-bit wide)
#define EM_BLE_WR(addr, value)       (*(volatile uint16_t *)(addr)) = (value)
//__INLINE__ void EM_BLE_WR(uint32_t addr, uint16_t value)
//{
//    volatile uint16_t temp1 = 0;
////    do 
////    {
//        (*(volatile uint16_t *)(addr)) = (value);
//        temp1 = (*(volatile uint16_t *)(addr));
//        if (temp1 != value)
//        {
//            em_ble_wr_cnt++;
//            printf("EM_BLE_WR(addr:0x%X,value:0x%X,temp1:0x%X,CNT:%d)\r\n", addr, value, temp1, em_ble_wr_cnt);
//        }        
////    } while(temp1 != value); 
//}
/**********************************************************************************************/
/// Macro to read a BT register
#define REG_BT_RD(addr)              (*(volatile uint32_t *)(addr))

/// Macro to write a BT register
#define REG_BT_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a BT control structure field (16-bit wide)
#define EM_BT_RD(addr)               (*(volatile uint16_t *)(addr))

/// Macro to write a BT control structure field (16-bit wide)
#define EM_BT_WR(addr, value)        (*(volatile uint16_t *)(addr)) = (value)

/// Macro to read a EM field (16-bit wide)
#define EM_RD(addr)               (*(volatile uint16_t *)(addr))

/// Macro to write a EM field (16-bit wide)
#define EM_WR(addr, value)        (*(volatile uint16_t *)(addr)) = (value)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if (defined(CFG_BT) || (defined(CFG_BLE) && defined(CFG_EMB)))
/// Read bytes from EM
__INLINE__ void em_rd(void *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy(sys_addr, (void *)(em_addr + EM_BASE_ADDR), len);
}
/// Write bytes to EM
__INLINE__ void em_wr(void const *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy((void *)(em_addr + EM_BASE_ADDR), sys_addr, len);
}

// copy two exchange memory area
__INLINE__ void em_cpy(uint16_t dst_em_addr, uint16_t src_em_addr, uint16_t len)
{
    memcpy((void *)(dst_em_addr + EM_BASE_ADDR), (void *)(src_em_addr + EM_BASE_ADDR), len);
}

/// Fill an EM space with the same value
__INLINE__ void em_set(int value, uint16_t em_addr, uint16_t len)
{
    memset((void *)(em_addr + EM_BASE_ADDR), value, len);
}


/// Read 32-bits value from EM
__INLINE__ uint32_t em_rd32p(uint16_t em_addr)
{
    return read32p((void *)(em_addr + EM_BASE_ADDR));
}
/// Write 32-bits value to EM
__INLINE__ void em_wr32p(uint16_t em_addr, uint32_t value)
{
    write32p((void *)(em_addr + EM_BASE_ADDR), value);
}

/// Read 24-bits value from EM
__INLINE__ uint32_t em_rd24p(uint16_t em_addr)
{
    return read24p((void *)(em_addr + EM_BASE_ADDR));
}
/// Write 24-bits value to EM
__INLINE__ void em_wr24p(uint16_t em_addr, uint32_t value)
{
    write24p((void *)(em_addr + EM_BASE_ADDR), value);
}

/// Read 16-bits value from EM
__INLINE__ uint16_t em_rd16p(uint16_t em_addr)
{
    return read16p((void *)(em_addr + EM_BASE_ADDR));
}
/// Write 16-bits value to EM
__INLINE__ void em_wr16p(uint16_t em_addr, uint16_t value)
{
    write16p((void *)(em_addr + EM_BASE_ADDR), value);
}

/// Read 8-bits value from EM
__INLINE__ uint8_t em_rd8p(uint16_t em_addr)
{
    return *((uint8_t *)(em_addr + EM_BASE_ADDR));
}
/// Write 8-bits value to EM
__INLINE__ void em_wr8p(uint16_t em_addr, uint8_t value)
{
    *(uint8_t *)(em_addr + EM_BASE_ADDR) = value;
}
#endif // (defined(CFG_BT) || (defined(CFG_BLE) && defined(CFG_EMB)))

/// @} REG

#endif // REG_ACCESS_H_
