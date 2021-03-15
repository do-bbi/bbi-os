#include "Utility.h"
#include "AssemblyUtil.h"
#include "Console.h"
// #include <stdarg.h>

/*
    va_list: 가변 인자 목록, 가변 인자 주소를 저장한 포인터형 변수
    va_start: 가변 인자를 가져올 수 있도록 주소 설정
    va_arg: 가변 인자 주소에서 특정 Type 크기만큼 값을 읽음
    va_end: 가변 인자 처리 후, ap를 NULL로 초기화
*/

// Total sizeof Main Memory (Unit = MBs)
static int gTotalSizeofRAM = 0;

// Counter for PIT Controller Ticked
volatile QWORD gTickCount = 0;

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

// RFLAGS 레지스터의 인터럽트 플래그를 변경하고, 이전 인터럽트 플래그이 상태를 반환
BOOL kSetInterruptFlag(BOOL enableInterrupt) {
    QWORD rflags;

    // 이전의 RFLAGS 레지스터 값을 읽은 뒤에 인터럽트 활성/비활성
    rflags = kReadRFLAGS();

    enableInterrupt ? kEnableInterrupt() : kDisableInterrupt();

    // 이전 RFLAGS 레지스터의 IF[9] bit를 확인해서 이전 인터럽트 상태를 반환
    return (rflags & (0x1 << 9)) ? TRUE : FALSE;
}

int kStrLen(const char *pStr) {
    int len = 0;
    while(pStr[len])
        ++len;

    return len;
}

// Check from 64MBs to end, Must call at booting once
void kCheckTotalSizeofRAM(void) {
    DWORD *pCurrentAddr;
    DWORD previousValue;

    // Check from 0x4000000(64MB) in 4MB increments
    pCurrentAddr = (DWORD *)0x4000000;
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
    QWORD value = 0;
    int i;

    for(i = 0; i < pBuf[i]; ++i) {
        value <<= 4;
        if('A' <= pBuf[i] && pBuf[i] <= 'Z')
            value += pBuf[i] - 'A' + 10;
        else if('a' <= pBuf[i] && pBuf[i] <= 'z')
            value += pBuf[i] - 'a' + 10;
        else if('0' <= pBuf[i] && pBuf[i] <= '9')
            value += pBuf[i] - '0';
        else
            return -1;
        kPrintf("%d\n", value);
    }

    return value;
}

// Convert Decimal String to QWORD
long kDecStringToLong(const char *pBuf) {
    long value = 0;
    int i;

    BOOL isNegative = (pBuf[0] == '-') ? 1 : 0;

    for(i = isNegative; i < pBuf[i]; ++i) {
        value *= 10;
        value += pBuf[i] - '0';
    }

    return isNegative ? -value : value;
}

int kItoA(long value, char *pBuf, int radix) {
    int ret;

    switch(radix) {
        case 16:
            ret = kHexToString(value, pBuf);
            break;
        
        case 10:
        default:
            ret = kDecToString(value, pBuf);
            break;
    }

    return ret;
}

// Convert QWORD to Hexa String
QWORD kHexToString(QWORD value, char *pBuf) {
    QWORD i, remain;

    if(value == 0) {
        pBuf[0] = '0';
        pBuf[1] = '\0';

        return 1;
    }

    // 버퍼의 1의 자리수 부터 16, 256, ...의 자리 순서로 숫자 삽입
    for(i = 0; value > 0; ++i) {
        remain = value % 16;
        if(10 <= remain)
            pBuf[i] = 'A' + (remain - 10);
        else 
            pBuf[i] = '0' + remain;
        value /= 16;
    }
    pBuf[i] = '\0';

    kReverseString(pBuf);
    
    return i;
}

// Convert QWORD to Decimal String
long kDecToString(long value, char *pBuf) {
    long i;
    BOOL isNegative;

    if(value == 0) {
        pBuf[0] = '0';
        pBuf[1] = '\0';

        return 1;
    }

    isNegative = value < 0 ? 1 : 0;

    if(isNegative) {
        pBuf[0] = '-';
        value = -value;
    }

    // 버퍼의 1의 자리수 부터 16, 256, ...의 자리 순서로 숫자 삽입
    for(i = isNegative; value > 0; ++i) {
        pBuf[i] = '0' + (value % 10);
        value /= 10;
    }
    pBuf[i] = '\0';

    kReverseString(&pBuf[isNegative]);
    
    return i;
}

// Reverse String
void kReverseString(char *pBuf) {
    int i, len;
    char tmp;

    len = kStrLen(pBuf);
    for(i = 0; i < len / 2; ++i) {
        // SWAP str[i] <=> str[(len - 1) - i]
        tmp = pBuf[i];
        pBuf[i] = pBuf[(len - 1) - i];
        pBuf[(len - 1) - i] = tmp;
    }
}

int kSPrintf(char *pBuf, const char *pFormatStr, ...) {
    va_list ap;
    int ret;

    va_start(ap, pFormatStr);
    ret = kVSPrintf(pBuf, pFormatStr, ap);
    va_end(ap);

    return ret;
}

int kVSPrintf(char *pBuf, const char *pFormatStr, va_list ap) {
    QWORD i, j;
    int fmtLen, cpyLen, bufIdx = 0;
    char *pCpyStr;

    union Value {
        QWORD ul;
        int i;    
    } value;

    fmtLen = kStrLen(pFormatStr);
    for(i = 0; i < fmtLen; ++i) {
        // if prefix is '%?', then translates it to data
        if(pFormatStr[i] == '%') {
            switch(pFormatStr[++i]) {
                case 's':
                    pCpyStr = (char *)(va_arg(ap, char *));
                    cpyLen = kStrLen(pCpyStr);

                    kMemCpy(pBuf + bufIdx, pCpyStr, cpyLen);
                    bufIdx += cpyLen;
                    break;
                case 'c':
                    pBuf[bufIdx++] = (char)(va_arg(ap, int));
                    break;
                case 'd':
                case 'i':
                    value.i = (int)(va_arg(ap, int));
                    bufIdx += kItoA(value.i, pBuf + bufIdx, 10);
                    break;
                case 'x':   // Print 4 Bytes Hex Value
                case 'X':
                    value.ul = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
                    bufIdx += kItoA(value.ul, pBuf + bufIdx, 16);
                    break;
                case 'q':   // Print 8 Bytes Hex Value
                case 'Q':
                case 'p':
                    // 가변 인자에 들어 있는 파라미터를 QWORD 타입으로 변환
                    // 출력 버퍼에 복사하고, 출력한 길이만큼 버퍼 인덱스를 이동
                    value.ul = (QWORD)(va_arg(ap, QWORD));
                    bufIdx += kItoA(value.ul, pBuf + bufIdx, 16);
                    break;
                default:
                    pBuf[bufIdx++] = pFormatStr[i];
                    break;
            }
        }
        else 
            pBuf[bufIdx++] = pFormatStr[i];
    }

    // NULL을 추가해 완전한 문자열로 만들고 출력한 문자 길이를 반환
    pBuf[bufIdx] = '\0';
    
    return bufIdx;
}

QWORD kGetTickCount(void) {
    return gTickCount;
}

// milisecond 동안 대기
void kSleep(QWORD ms) {
    QWORD lastTickCnt = gTickCount;

    while(gTickCount - lastTickCnt <= ms)
        kSchedule();
}