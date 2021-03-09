 #ifndef __PIC_H__
 #define __PIC_H__

 #include "Types.h"

 // I/O Port
 #define PIC_MASTER_PORT1       (0x20)
 #define PIC_MASTER_PORT2       (0x21)
 #define PIC_SLAVE_PORT1        (0xA0) 
 #define PIC_SLAVE_PORT2        (0xA1) 

 // Interrupt Vector Offset from IDT(Interrupt Descriptor Table)
 #define PIC_IRQ_VECTOR_OFFSET  (0x20)

 void kInitializePIC(void);
 void kMaskPICInterrupt(WORD irqBitmask);
 void kSendEOI2PIC(int irqNumber);

 #endif // __PIC_H__