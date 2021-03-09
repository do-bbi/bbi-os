#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"

void kCommonExceptionHandler(int vectorNumber, QWORD errorCode);
void kCommonInterruptHandler(int vectorNumber);
void kKeyboardHandler(int vectorNumber);
void kTimerHandler(int vectorNumber);

#endif  // __INTERRUPTHANDLER_H__