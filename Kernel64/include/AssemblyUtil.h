#ifndef __ASSEMBLYUTIL_H__
#define __ASSEMBLYUTIL_H__

#include "Types.h"
#include "Task.h"

BYTE kInPortByte(WORD port);
BYTE kOutPortByte(WORD port, BYTE data);
void kLoadGDTR(QWORD pGDTR);
void kLoadTR(WORD offset);
void kLoadIDTR(QWORD pIDTR);
void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);
QWORD kReadTSC(void);
void kSwitchContext(CONTEXT *pCurrentContext, CONTEXT *pNextContext);

#endif  // __ASSEMBLYUTIL_H__