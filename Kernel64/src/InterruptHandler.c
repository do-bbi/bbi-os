#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtil.h"

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

// Device Not Available Exception Handler
void kDeviceNotAvailableHandler(int vectorNumber) {
    TCB *pFPUTask, *pCurTask;
    QWORD lastFPUTaskID;

    /****************************************************/
    // Print FPU Exception occurred
    char msgBuffer[] = "[EXC:  , ]";
    static int gFPUInterruptCnt = 0;

    // Print Exception Vector ↗ of screen
    msgBuffer[5] = '0' + vectorNumber / 10;
    msgBuffer[6] = '0' + vectorNumber % 10;
    // Print Exception occurred
    msgBuffer[8] = '0' + gFPUInterruptCnt;
    gFPUInterruptCnt = (gFPUInterruptCnt + 1) % 10;

    kPrintStringXY(70, 0, msgBuffer);

    // CR0 컨트롤 레지스터의 TS 비트를 0으로 Clear
    kClearTS();

    // 이전에 FPU를 사용한 태스크가 있는지 확인해 있다면 FPU의 상태를 태스크에 저장
    lastFPUTaskID = kGetLastFPUUsedTaskID();
    pCurTask = kGetRunningTask();

    // 이전에 FPU를 사용했던 Task가 현재 Task라면 Handler 종료
    if(lastFPUTaskID == pCurTask->link.id)
        return;
    
    // 이전 FPU 사용 Task가 있다면 FPU Context를 저장
    if(lastFPUTaskID != TASK_INVALID_ID) {
        pFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(lastFPUTaskID));
        if(pFPUTask != NULL && pFPUTask->link.id == lastFPUTaskID)
            kSaveFPUContext(pFPUTask->fpuCtx);
    }

    // 현재 태스크가 FPU를 사용한 적이 있는지 확인해
    // FPU를 사용한 적이 없다면 초기화
    // 사용한 적이 있다면 FPU 콘텍스트를 복원
    if( pCurTask->isFPUUsed == FALSE) {
        kInitializeFPU();
        pCurTask->isFPUUsed = TRUE;
    }
    else
        kLoadFPUContext(pCurTask->fpuCtx);
    
    // FPU를 사용한 Task ID를 현재 태스크로 변경
    kSetLastFPUUsedTaskID(pCurTask->link.id);
}

void kHDDHandler(int vectorNumber) {
    /****************************************************/
    // Print FPU Exception occurred
    char msgBuffer[] = "[INT:  , ]";
    static int gHDDInterruptCnt = 0;

    // Print Exception Vector ↗ of screen
    msgBuffer[5] = '0' + vectorNumber / 10;
    msgBuffer[6] = '0' + vectorNumber % 10;
    // Print Exception occurred
    msgBuffer[8] = '0' + gHDDInterruptCnt;
    gHDDInterruptCnt = (gHDDInterruptCnt + 1) % 10;

    kPrintStringXY(70, 2, msgBuffer);

    // Primary PATA 포트 인터럽트 발생으로 처리
    if(vectorNumber - PIC_IRQ_VECTOR_OFFSET == 14)
        kSetHDDInterruptFlag(TRUE, TRUE);
    else // Secondary PATA 포트 인터럽트 발생으로 처리
        kSetHDDInterruptFlag(FALSE, TRUE);

    kSendEOI2PIC(vectorNumber - PIC_IRQ_VECTOR_OFFSET);
}