#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"

// Handler for Common Exceptions
void kCommonExceptionHandler(int vectorNumber, QWORD errorCode) {
    int posY = 0;
    char buf[3] = {0, };

    // Print Interrupt Vector number at ↗(Top right corner) of screen
    buf[0] = '0' + vectorNumber / 10;
    buf[1] = '0' + vectorNumber % 10;

    kPrintStringXY(0, posY++, "===================================================================");
    kPrintStringXY(0, posY++, "================ Common Exception Handler Executed ================");
    kPrintStringXY(0, posY++, "================ Exception Vector Number --------------------->[  ]");
    kPrintStringXY(0, posY++, "===================================================================");
    kPrintStringXY(64, 2, buf);
    
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
    kPrintStringXY(70, 2, buf);
    
    kSendEOI2PIC(vectorNumber - PIC_IRQ_VECTOR_OFFSET);
}

// Handler for Common Interrupts
void kKeyboardHandler(int vectorNumber) {
    char buf[] = "[INT:  , ]";
    BYTE temp;

    static int gKeyboardInterruptCount;

    // Print Interrupt Vector number at ↗(Top right corner) of screen
    buf[5] = '0' + vectorNumber / 10;
    buf[6] = '0' + vectorNumber % 10;

    // Count of Interrupts
    buf[8] = '0' + gKeyboardInterruptCount;
    gKeyboardInterruptCount = (++gKeyboardInterruptCount) % 10;
    kPrintStringXY(70, 1, buf);

    // Read data from keyboard controller & convert it ASCII code to push into queue
    if(kIsOutputBufferFull()) {
        temp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(temp);
    }
    
    kSendEOI2PIC(vectorNumber - PIC_IRQ_VECTOR_OFFSET);
}

// Handler for Timer Interrupt
void kTimerHandler(int vectorNumber) {
    char msgBuffer[] = "[INT:  , ]";
    static int gTimerInterruptCount = 0;

    msgBuffer[5] = '0' + vectorNumber / 10;
    msgBuffer[6] = '0' + vectorNumber % 10;

    msgBuffer[8] = '0' + gTimerInterruptCount;
    gTimerInterruptCount = (gTimerInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, msgBuffer);

    // Send EOI
    kSendEOI2PIC(vectorNumber - PIC_IRQ_VECTOR_OFFSET);

    // Increase Tick Count
    gTickCount++;

    // Decrease Available Processor Time of Task
    kDecreaseProcessorTime();

    // Context Switching if Processor Time is expired
    if( kIsProcessorTimeExpired() )
        kScheduleInInterrupt();
}