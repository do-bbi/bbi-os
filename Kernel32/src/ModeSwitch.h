#ifndef __MODESWITCH_H__
#define __MODESWITCH_H__

#include "Types.h"

void kReadCPUID(DWORD eax_val, DWORD *pEAX, DWORD *pEBX, DWORD *pECX, DWORD *pEDX);
void kSwitchAndExecute64bitKernel(void);

#endif  // __MODESWITCH_H__