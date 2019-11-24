#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"

/**
 *  GDT(Global Descriptor Table)
 *  Segment들의 크기, 시작 주소(Base Address), 권한(쓰기/읽기/실행, 커널 모드 등)
 *  GDT에 있는 시작주소를 참조해서 각 세그먼트들의 주소로 접근 가능
 *  GDT에 대한 정보는 CPU내 GDTR 레지스터에 저장 됨
 */
// Initialize GDT(Global Descriptor Table)
void kInitializeGDTableAndTSS(void) {
    GDTR *pGDTR;
    GDTENTRY8 *pEntry;
    TSSEGMENT *pTSS;
    
    int i;

    // Set GDTR
    pGDTR = (GDTR *)GDTR_BASE_ADDR;
    pEntry = (GDTENTRY8 *)(GDTR_BASE_ADDR + sizeof(GDTR));
    pGDTR->limit = GDT_TABLE_SIZE - 1;
    pGDTR->baseAddr = (QWORD)pEntry;

    // Set TSS
    pTSS = (TSSEGMENT *)((QWORD)pEntry + GDT_TABLE_SIZE);

    // NULL, 64bit Code/Data, TS 4종류의 Segment 생성
    kSetGDTEntry8(&(pEntry[0]), 0, 0, 0, 0, 0);                                             // NULL
    kSetGDTEntry8(&(pEntry[1]), 0, 0xFFFFF, 
                    GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNEL_CODE, GDT_TYPE_CODE);      // CODE
    kSetGDTEntry8(&(pEntry[2]), 0, 0xFFFFF, 
                    GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNEL_DATA, GDT_TYPE_DATA);      // DATA
    kSetGDTEntry16((GDTENTRY16 *)&(pEntry[3]), (QWORD)pTSS, sizeof(TSSEGMENT) - 1, 
                    GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);                // TSS

    // Initialize TSS 
    kInitializeTSSegment(pTSS);
}

// Set GDT Entry(8-Bytes-Size)
// For setting Code/Data Segment Descriptor
void kSetGDTEntry8(GDTENTRY8 *pEntry8, DWORD baseAddr, DWORD limit, 
                    BYTE upperFlags, BYTE lowerFlags, BYTE type) {
    pEntry8->lowerLimit = limit & 0xFFFF;
    pEntry8->lowerBaseAddr = baseAddr & 0xFFFF;
    pEntry8->upperBaseAddr1 = (baseAddr >> 16) & 0xFF;
    pEntry8->typeAndLowerFlag = lowerFlags | type;
    pEntry8->upperLimitAndUpperFlag = ((limit >> 16) & 0x0F) | upperFlags;
    pEntry8->upperBaseAddr2 = (baseAddr >> 24) & 0xFF;
}

void kSetGDTEntry16(GDTENTRY16 *pEntry16, QWORD baseAddr, DWORD limit, 
                    BYTE upperFlags, BYTE lowerFlags, BYTE type) {
    pEntry16->lowerLimit = limit & 0xFFFF;
    pEntry16->lowerBaseAddr = baseAddr & 0xFFFF;
    pEntry16->middleBaseAddr1 = (baseAddr >> 16) & 0xFF;
    pEntry16->typeAndLowerFlag = lowerFlags | type;
    pEntry16->upperLimitAndUpperFlag = ((limit >> 16) & 0x0F) | upperFlags;
    pEntry16->middleBaseAddr2 = (baseAddr >> 24) & 0xFF;
    pEntry16->upperBaseAddr = baseAddr >> 32;
    pEntry16->reserved = 0;

}

void kInitializeTSSegment(TSSEGMENT *pTSS) {
    kMemSet(pTSS, 0, sizeof(TSSEGMENT));
    pTSS->ist[0] = IST_BASE_ADDR + IST_SIZE;

    // IO 허용 범위를 TSS limit 값보다 크게(0xFFFF) 설정하여 IO Map을 사용하지 않도록 함
    pTSS->ioMapBaseAddr = 0xFFFF;
}

void kInitializeIDTables(void) {
    IDTR *pIDTR;
    IDTENTRY *pEntry;
    
    int i;

    // Base Address of IDTR
    pIDTR = (IDTR *)IDTR_BASE_ADDR;

    // Create IDT Information
    pEntry = (IDTENTRY *)(IDTR_BASE_ADDR + sizeof(IDTR));
    pIDTR->baseAddr = (QWORD)pEntry;
    pIDTR->limit = IDT_TABLE_SIZE - 1;

    // // 0 ~ 99번 Vector를 모두 Dummy Handler로 임시로 연결
    // for(i = 0; i < IDT_MAX_ENTRY_COUNT; ++i) {
    //     kSetIDTEntry(&(pEntry[i]), kDummyHandler, 0x08, IDT_FLAGS_IST1, 
    //                 IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    // }

    // ISR for Exceptions
    kSetIDTEntry(&(pEntry[0]),  kISRDivideError,                0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[1]),  kISRDebug,                      0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[2]),  kISRNMI,                        0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[3]),  kISRBreakPoint,                 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[4]),  kISROverflow,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[5]),  kISRBoundRangeExceeded,         0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[6]),  kISRInvalidOpcode,              0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[7]),  kISRDeviceNotAvaliable,         0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[8]),  kISRDoubleFault,                0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[9]),  kISRCoprocessorSegmentOverrun,  0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[10]), kISRInvalidTSS,                 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[11]), kISRSegmentNotPresent,          0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[12]), kISRStackSegmentFault,          0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[13]), kISRGeneralProtection,          0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[14]), kISRPageFault,                  0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[15]), kISR15,                         0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[16]), kISRFPUError,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[17]), kISRAlignmentCheck,             0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[18]), kISRMachineCheck,               0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[19]), kISRSIMDError,                  0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[20]), kISRETCException,               0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

    for(i = 21; i < 32; ++i)
        kSetIDTEntry(&(pEntry[i]), kISRETCException,            0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    
    // ISR for Interrupts
    kSetIDTEntry(&(pEntry[32]), kISRTimer,                      0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[33]), kISRKeyboard,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[34]), kISRSlavePIC,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[35]), kISRSerial2,                    0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[36]), kISRSerial1,                    0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[37]), kISRParallel2,                  0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[38]), kISRFloopy,                     0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[39]), kISRParallel1,                  0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[40]), kISRRTC,                        0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[41]), kISRReserved,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[42]), kISRNotUsed1,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[43]), kISRNotUsed2,                   0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[44]), kISRMouse,                      0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[45]), kISRCoprocessor,                0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[46]), kISRHDD1,                       0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pEntry[47]), kISRHDD2,                       0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

    for(i = 48; i < IDT_MAX_ENTRY_COUNT; ++i)
        kSetIDTEntry(&(pEntry[i]), kISRETCInterrupt,            0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
}

void kSetIDTEntry(IDTENTRY *pEntry, void *pHandler, WORD selector, 
                    BYTE ist, BYTE flags, BYTE type) {
    pEntry->lowerBaseAddr = (QWORD)pHandler & 0xFFFF;
    pEntry->segmentSelector = selector;
    pEntry->ist = ist & 0x3;
    pEntry->typeAndFlags = type | flags;
    pEntry->middleBaseAddr = ((QWORD)pHandler >> 16) & 0xFFFF;
    pEntry->upperBaseAddr = (QWORD)pHandler >> 32;
    pEntry->reserved = 0;

}

void kDummyHandler(void) {
    int posY = 0;

    kPrintString(0, posY++, "===================================================================");
    kPrintString(0, posY++, "================ Dummy Interrupt Handler Executed =================");
    kPrintString(0, posY++, "================  Interrupt or Exception Occured  =================");
    kPrintString(0, posY++, "===================================================================");

    while(TRUE);    
}