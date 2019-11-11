#include "Utility.h"

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
