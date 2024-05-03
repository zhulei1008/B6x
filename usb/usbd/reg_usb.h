#ifndef _REG_USB_H_
#define _REG_USB_H_

#include <stdint.h>

#pragma anon_unions

#define __I                     volatile const  /* defines 'read only' permissions */
#define __O                     volatile        /* defines 'write only' permissions */
#define __IO                    volatile        /* defines 'read/write' permissions */

#define USB_BASE                (0x40003000)
#define USB                     ((volatile USB_TypeDef *)USB_BASE)
    

/**
  * @brief Register map for USB peripheral
  */
typedef struct
{
    /* 00~0F: Common USB registers */
    __IO uint32_t  FADDR;       // 00 Function address register.
    __IO uint32_t  POWER;       // 01 Power management register.
    __I  uint32_t  INTRIN1;     // 02 Interrupt register for Endpoint 0 plus IN Endpoints 1 to 7.
    __I  uint32_t  INTRIN2;     // 03 Interrupt register for IN Endpoints 8 to 15. Only present if more than 7 IN endpoints are included.
    __I  uint32_t  INTROUT1;    // 04 Interrupt register for OUT Endpoints 1 to 7.
    __I  uint32_t  INTROUT2;    // 05 Interrupt register for OUT Endpoints 8 to 15. Only present if more than 7 OUT endpoints are included.
    __I  uint32_t  INTRUSB;     // 06 Interrupt register for common USB interrupts.
    __IO uint32_t  INTRIN1E;    // 07 Interrupt enable register for IntrIn1.
    __IO uint32_t  INTRIN2E;    // 08 Interrupt enable register for IntrIn2. Only present if more than 7 IN endpoints are included.
    __IO uint32_t  INTROUT1E;   // 09 Interrupt enable register for IntrOut1.
    __IO uint32_t  INTROUT2E;   // 0A Interrupt enable register for IntrOut2. Only present if more than 7 OUT endpoints are included.
    __IO uint32_t  INTRUSBE;    // 0B Interrupt enable register for IntrUSB.
    __I  uint32_t  FRAME1;      // 0C Frame number bits 0 to 7.
    __I  uint32_t  FRAME2;      // 0D Frame number bits 8 to 10.
    __IO uint32_t  EPIDX;       // 0E Index register for selecting the endpoint status and control registers.
    __IO uint32_t  RESV0;       // 0F Unused, always returns 0.

    /* 10~1F: Indexed registers(Control Status registers for endpoint selected by the Index register) */
    __IO uint32_t  INMAXP;      // 10 Maximum packet size for IN endpoint. (Index register set to select Endpoints 1 – 15 only)

    union {
    __IO uint32_t  CSR0;        // 11 CSR0 Control Status register for Endpoint 0. (Index register set to select Endpoint 0)
    __IO uint32_t  INCSR1;      // 11 Control Status register 1 for IN endpoint. (Index register set to select Endpoints 1 – 15)
    };
    
    __IO uint32_t  INCSR2;      // 12 Control Status register 2 for IN endpoint. (Index register set to select Endpoints 1 – 15 only)
    __IO uint32_t  OUTMAXP;     // 13 Maximum packet size for OUT endpoint. (Index register set to select Endpoints 1 – 15 only)
    __IO uint32_t  OUTCSR1;     // 14 Control Status register 1 for OUT endpoint. (Index register set to select Endpoints 1 – 15 only)
    __IO uint32_t  OUTCSR2;     // 15 Control Status register 2 for OUT endpoint. (Index register set to select Endpoints 1 – 15 only)

    union {
    __IO uint32_t  COUNT0;      // 16 COUNT0 Number of received bytes in Endpoint 0 FIFO. (Index register set to select Endpoint 0)
    __IO uint32_t  OUTCOUNT1;   // 16 Number of bytes in OUT endpoint FIFO (lower byte). (Index register set to select Endpoints 1 –115)
    };
    
    __IO uint32_t  OUTCOUNT2;   // 17 Number of bytes in OUT endpoint FIFO (upper byte). (Index register set to select Endpoints 1 –115 only)
    __IO uint32_t  RESV1[8];    // 18-1F Unused, always returns 0

    /* 20~2F: FIFOx registers */
    __IO uint32_t  FIFO[16];    // 20~2F FIFOs for Endpoints 0 to 15.
} USB_TypeDef;


//*****************************************************************************
//
// The following are defines for the bit fields in the USB_FADDR register.
//
//*****************************************************************************
#define USB_FADDR_UPDATE        0x80  // r  Update
#define USB_FADDR_ADDR_M        0x7F  // rw Function Address
#define USB_FADDR_ADDR_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_POWER register.
//
//*****************************************************************************
#define USB_POWER_ISOUP         0x80  // rw Isochronous Update
#define USB_POWER_RESET         0x08  // r  RESET Signaling
#define USB_POWER_RESUME        0x04  // rw RESUME Signaling
#define USB_POWER_SUSPEND       0x02  // r  SUSPEND Mode
#define USB_POWER_ENSUSPEND     0x01  // rw Enable Suspend

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_INTRxxxx register.
//
//*****************************************************************************

#define USB_ISR_EP(n)           (1 << (n))

#define USB_ISR_EP7             0x80  // bit7
#define USB_ISR_EP6             0x40  // bit6
#define USB_ISR_EP5             0x20  // bit5
#define USB_ISR_EP4             0x10  // bit4
#define USB_ISR_EP3             0x08  // bit3
#define USB_ISR_EP2             0x04  // bit2
#define USB_ISR_EP1             0x02  // bit1
#define USB_ISR_EP0             0x01  // bit0


//*****************************************************************************
//
// The following are defines for the bit fields in the USB_IS register.
//
//*****************************************************************************
#define USB_ISR_SOF             0x08  // rw Start of Frame
#define USB_ISR_RESET           0x04  // rw RESET Signaling Detected
#define USB_ISR_RESUME          0x02  // rw RESUME Signaling Detected
#define USB_ISR_SUSPEND         0x01  // rw SUSPEND Signaling Detected

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_EPIDX register.
//
//*****************************************************************************
#define USB_EPIDX_M             0x0F  // Endpoint Index
#define USB_EPIDX_S             0

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_CSR0 register.
//
//*****************************************************************************
#define USB_CSR0_SETUPENDCLR    0x80  // s  Setup End Clear
#define USB_CSR0_OUTRDYCLR      0x40  // s  RXRDY Clear
#define USB_CSR0_STALL          0x20  // s  Send Stall
#define USB_CSR0_SETUPEND       0x10  // r  Setup End
#define USB_CSR0_DATAEND        0x08  // s  Data End
#define USB_CSR0_STALLED        0x04  // rc Endpoint Stalled
#define USB_CSR0_INRDY          0x02  // rs Transmit Packet Ready
#define USB_CSR0_OUTRDY         0x01  // r  Receive Packet Ready

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_INMAXP register.
//
//*****************************************************************************
#define USB_INMAXP_M   0xFF  // Maximum Payload
#define USB_INMAXP_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_INCSR1 register.
//
//*****************************************************************************
#define USB_INCSR1_CLRDT        0x40  // s  Clear Data Toggle
#define USB_INCSR1_STALLED      0x20  // rc Endpoint Stalled
#define USB_INCSR1_STALL        0x10  // rw Send STALL
#define USB_INCSR1_FLUSH        0x08  // s  Flush FIFO
#define USB_INCSR1_UNDRN        0x04  // rc Underrun
#define USB_INCSR1_FIFONE       0x02  // rc FIFO Not Empty
#define USB_INCSR1_INRDY        0x01  // rs Transmit Packet Ready

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_INCSR2 register.
//
//*****************************************************************************
#define USB_INCSR2_AUTOSET      0x80  // rw Auto Set
#define USB_INCSR2_ISO          0x40  // rw Isochronous Transfers
#define USB_INCSR2_MODE         0x20  // rw Mode
#define USB_INCSR2_DMAEN        0x10  // rw DMA Request Enable
#define USB_INCSR2_FRCDT        0x08  // rw Force Data Toggle

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_OUTMAXP register.
//
//*****************************************************************************
#define USB_OUTMAXP_M   0xFF  // Maximum Payload
#define USB_OUTMAXP_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_OUTCSR1 register.
//
//*****************************************************************************
#define USB_OUTCSR1_CLRDT       0x80  // s  Clear Data Toggle
#define USB_OUTCSR1_STALLED     0x40  // rc Endpoint Stalled
#define USB_OUTCSR1_STALL       0x20  // rw Send STALL
#define USB_OUTCSR1_FLUSH       0x10  // s  Flush FIFO
#define USB_OUTCSR1_DATAERR     0x08  // r  Data Error
#define USB_OUTCSR1_OVERRUN     0x04  // rc Overrun
#define USB_OUTCSR1_FULL        0x02  // r  FIFO Full
#define USB_OUTCSR1_OUTRDY      0x01  // rc Receive Packet Ready

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_OUTCSR2 register.
//
//*****************************************************************************
#define USB_OUTCSR2_AUTOCLR     0x80  // Auto Clear
#define USB_OUTCSR2_ISO         0x40  // Isochronous Transfers
#define USB_OUTCSR2_DMAEN       0x20  // DMA Request Enable
#define USB_OUTCSR2_DMAMODE     0x10  // DMA Request Mode

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_COUNT0 register.
//
//*****************************************************************************
#define USB_COUNT0_COUNT_M      0x7F  // FIFO Count
#define USB_COUNT0_COUNT_S      0

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_RXCOUNT1 register.
//
//*****************************************************************************
#define USB_OUTCOUNT1_COUNT_M    0xFF  // Receive Packet Count
#define USB_OUTCOUNT1_COUNT_S    0

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_RXCOUNT1 register.
//
//*****************************************************************************
#define USB_OUTCOUNT2_COUNT_M    0x07  // Receive Packet Count
#define USB_OUTCOUNT2_COUNT_S    0

#endif
