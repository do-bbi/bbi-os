#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "Types.h"

void kMemSet(void *pDst, BYTE data, int size);
int kMemCpy(void *pDst, const void *pSrc, int size);
int kMemCmp(const void *pDst, const void *pSrc, int size);

#endif /*__UTILITY_H__*/
