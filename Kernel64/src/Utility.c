#include "Utility.h"
#include "AssemblyUtil.h"
#include <stdarg.h>

// 메모리를 특정 값으로 채움
void kMemSet(void *pDst, BYTE data, int size) {
    int i;
    
    for(i = 0; i < size; ++i) 
        ((char *)pDst)[i] = data;
}

// 메모리 복사
int kMemCpy(void *pDst, const void *pSrc, int size) {
    int i;
    
    for(i = 0; i < size; ++i) 
        ((char *)pDst)[i] = ((char *)pSrc)[i];
    
    return size;
}

// 메모리 비교
int kMemCmp(const void *pDst, const void *pSrc, int size) {
    int i;
    char tmp;
    
    for(i = 0; i < size; ++i) {
        tmp = ((char *)pDst)[i] - ((char *)pSrc)[i];
        if(tmp)
            return (int)tmp;
    }

    return 0;
}

int kStrLen(const char *pStr) {
    int len = 0;
    while(pStr[len])
        ++len;

    return len;
}

// Total sizeof Main Memory (Unit = MBs)
static int gTotalSizeofRAM = 0;

// Check from 64MBs to end, Must call at booting once
void kCheckTotalSizeofRAM(void) {
    DWORD *pCurrentAddr;
    DWORD previousValue;

    // Check from 0x4000000(64MB) in 4MB increments
    pCurrentAddr = (DWORD *)0x400000;
    while(TRUE) {
        previousValue = *pCurrentAddr;  // Save value of previous memory

        // Write 0x12345678 & Check it is 0x12345678
        *pCurrentAddr = 0x12345678;
        if(*pCurrentAddr != 0x12345678)
            break;

        // Restore previous value
        *pCurrentAddr = previousValue;

        // Increment 4MBs
        pCurrentAddr += (0x400000 / 4);
    }

    // Calculate Total sizeof RAM by dividing 1MB
    gTotalSizeofRAM = (QWORD) pCurrentAddr / 0x100000;
}

// Return Total sizeof RAM
QWORD kGetTotalRAMSize(void) {
    return gTotalSizeofRAM;
}

// Implement atoi()
long kAtoI(const char *pBuf, int radix) {
    long ret;

    switch(radix) {
        case 16:
            ret = kHexStringToQword(pBuf);
            break;
        
        case 10:
        default:
            ret = kDecStringToLong(pBuf);
            break;
    }

    return ret;
}

// Convert Hexa String to QWORD
QWORD kHexStringToQword(const char *pBuf) {
    // Please Implement Me
}

// Convert Decimal String to QWORD
long kDecStringToLong(const char *pBuf) {
    // Please Implement Me
}

int kItoA(long value, char *pBuf, int radix) {
    // Please Implement Me
}

// Convert QWORD to Hexa String
QWORD kHexToString(QWORD value, char *pBuf) {
    // Please Implement Me
}

// Convert QWORD to Decimal String
long kDecToString(long value, char *pBuf) {
    // Please Implement Me
}

// Reverse String
void kReverseString(char *pBuf) {
    // Please Implement Me
}

int kSPrintf(char *pBuf, const char *pFormatStr, ...) {
    // Please Implement Me
}

int kVPrintf(char *pBuf, const char *pFormatStr, va_list ap) {
    // Please Implement Me
}

// RFLAGS 레지스터의 인터럽트 플래그를 변경하고, 이전 인터럽트 플래그이 상태를 반환
BOOL kSetInterruptFlag(BOOL enableInterrupt) {
    QWORD rflags;

    // 이전의 RFLAGS 레지스터 값을 읽은 뒤에 인터럽트 활성/비활성
    rflags = kReadRFLAGS();

    enableInterrupt ? kEnableInterrupt() : kDisableInterrupt();

    // 이전 RFLAGS 레지스터의 IF[9] bit를 확인해서 이전 인터럽트 상태를 반환
    return (rflags & (0x1 << 9)) ? TRUE : FALSE;
}
