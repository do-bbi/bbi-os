#include "Utility.h"
#include "AssemblyUtil.h"

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
