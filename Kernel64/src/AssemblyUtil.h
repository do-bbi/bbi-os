#ifndef __ASSEMBLYUTIL_H__
#define __ASSEMBLYUTIL_H__

#include "Types.h"

BYTE kInPortByte(WORD port);
BYTE kOutPortByte(WORD port, BYTE data);
void kLoadGDTR(QWORD pGDTR);
void kLoadTR(WORD offset);
void kLoadIDTR(QWORD pIDTR);

#endif  // __ASSEMBLYUTIL_H__