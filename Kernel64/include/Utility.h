#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>
#include "Types.h"

extern volatile QWORD gTickCount;

void kMemSet(void *pDst, BYTE data, int size);
int kMemCpy(void *pDst, const void *pSrc, int size);
int kMemCmp(const void *pDst, const void *pSrc, int size);
BOOL kSetInterruptFlag(BOOL enableInterrupt);
int kStrLen(const char *pStr);
void kCheckTotalSizeofRAM(void);
QWORD kGetTotalRAMSize(void);
long kAtoI(const char *pBuf, int radix);
QWORD kHexStringToQword(const char *pBuf);
long kDecStringToLong(const char *pBuf);
int kItoA(long value, char *pBuf, int radix);
QWORD kHexToString(QWORD value, char *pBuf);
long kDecToString(long value, char *pBuf);
void kReverseString(char *pBuf);
int kSPrintf(char *pBuf, const char *pFormatStr, ...);
int kVSPrintf(char *pBuf, const char *pFormatStr, va_list ap);
QWORD kGetTickCount(void);

#endif /*__UTILITY_H__*/
