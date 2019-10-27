#ifndef __ASSEMBLYUTIL_H__
#define __ASSEMBLYUTIL_H__

#include "Types.h"

BYTE kInPortByte(WORD port);
BYTE kOutPortByte(WORD port, BYTE data);

#endif  // __ASSEMBLYUTIL_H__