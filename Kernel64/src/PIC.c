#include "PIC.h"

// Initialize PIC
void kInitializePIC(void) {
    // Initialize Master PIC
    // ICW1(Port 0x20), IC4[0] = 1
    kOutPortByte(PIC_MASTER_PORT1, 0x11);

    // ICW2(Port 0x21), Interrupt Vector(0x20)
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQ_VECTOR_OFFSET);

    // ICW3(Port 0x21), Slave PIC -> Master PIC Pin 2 연결 (= 0x1 << 2)
    kOutPortByte(PIC_MASTER_PORT2, (0x1 << 2));

    // ICW4(Port 0x21), uPM[0] = 1
    kOutPortByte(PIC_MASTER_PORT2, (0x1 << 0));

    // Initialize Slave PIC
    // ICW1(Port 0xA0), IC4[0] = 1
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);

    // ICW2(Port 0xA1), Interrupt Vector(0x20 + 8)
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQ_VECTOR_OFFSET + 8);

    // ICW3(Port 0xA1), Slave PIC -> Master PIC Pin 2 연결 (= 0x2)
    kOutPortByte(PIC_SLAVE_PORT2, (0x2));

    // ICW4(Port 0x21), uPM[0] = 1
    kOutPortByte(PIC_SLAVE_PORT2, (0x1 << 0));
}

// Maksing Interrupt to block another interrupt
void kMaskPICInterrupt(WORD irqBitmask) {
    // Set IMR to Master PIC
    // OCW1(Port 0x21) IRQ #0 ~ #7
    kOutPortByte(PIC_MASTER_PORT2, (BYTE)irqBitmask);
    
    // Set IMR to Slave PIC
    // OCW1(Port 0xA1) IRQ #8 ~ #15
    kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(irqBitmask >> 8));
}

// Send EOI to processor 
// if Master PIC    -> Send EOI to Master PIC only
// if Slave PIC     -> Send EOI to Both Master and Slave PICs
void kSendEOI2PIC(int irqNumber) {
    // Send EOI to PIC
    // OCW2(Port 0x20), EOI[5] = 1
    kOutPortByte(PIC_MASTER_PORT1, (0x1 << 5));

    // if Slave PIC, Send EOI to Both Master and Slave PICs
    if(irqNumber > 7)
        kOutPortByte(PIC_SLAVE_PORT1, (0x1 << 5));  // OCW2(Port 0xA0), EOI[5] = 1
}