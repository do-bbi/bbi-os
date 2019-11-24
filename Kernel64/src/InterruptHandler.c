#include "InterruptHandler.h"
#include "PIC.h"

// Handler for Common Exceptions
void kCommonExceptionHandler(int vectorNumber, QWORD errorCode) {
    int posY = 0;
    char buf[3] = {0, };

    // Print Interrupt Vector number at ↗(Top right corner) of screen
    buf[0] = '0' + vectorNumber / 10;
    buf[1] = '0' + vectorNumber % 10;


    kPrintString(0, posY++, "===================================================================");
    kPrintString(0, posY++, "================ Common Exception Handler Executed ================");
    kPrintString(0, posY++, "================ Exception Vector Number --------------------->[  ]");
    kPrintString(0, posY++, "===================================================================");
    
    kPrintString(27, 2, buf);
    
    while(TRUE);
}

// Handler for Common Interrupts
void kCommonInterruptHandler(int vectorNumber) {
    char buf[] = "[INT:  , ]";

    static int gCommonInterruptCount;

    // Print Interrupt Vector number at ↗(Top right corner) of screen
    buf[5] = '0' + vectorNumber / 10;
    buf[6] = '0' + vectorNumber % 10;

    // Count of Interrupts
    buf[8] = '0' + gCommonInterruptCount;
    gCommonInterruptCount = (++gCommonInterruptCount) % 10;
    kPrintString(70, 0, buf);
    
    kSendEOI2PIC(vectorNumber - PIC_IRQ_VECTOR_OFFSET);
}

// Handler for Common Interrupts
void kKeyboardHandler(int vectorNumber) {
    char buf[] = "[INT:  , ]";

    static int gKeyboardInterruptCount;

    // Print Interrupt Vector number at ↗(Top right corner) of screen
    buf[5] = '0' + vectorNumber / 10;
    buf[6] = '0' + vectorNumber % 10;

    // Count of Interrupts
    buf[8] = '0' + gKeyboardInterruptCount;
    gKeyboardInterruptCount = (++gKeyboardInterruptCount) % 10;
    kPrintString(70, 1, buf);
    
    kSendEOI2PIC(vectorNumber - PIC_IRQ_VECTOR_OFFSET);
}