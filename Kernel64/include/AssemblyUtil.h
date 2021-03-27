#ifndef __ASSEMBLYUTIL_H__
#define __ASSEMBLYUTIL_H__

#include "Types.h"
#include "Task.h"

BYTE kInPortByte(WORD port);
void kOutPortByte(WORD port, BYTE data);
WORD kInPortWord(WORD port);
void kOutPortWord(WORD port, WORD data);
void kLoadGDTR(QWORD pGDTR);
void kLoadTR(WORD offset);
void kLoadIDTR(QWORD pIDTR);
void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);
QWORD kReadTSC(void);
void kSwitchContext(CONTEXT *pCurrentContext, CONTEXT *pNextContext);
void kHlt(void);
BOOL kTestAndSet(volatile BYTE *pDst, BYTE compValue, BYTE src);

void kInitializeFPU(void);
void kSaveFPUContext(void *pFPUContext);
void kLoadFPUContext(void *pFPUContext);
void kSetTS(void);
void kClearTS(void);

#endif  // __ASSEMBLYUTIL_H__