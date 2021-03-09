#ifndef __SYNC_H__
#define __SYNC_H__

#include "Types.h"

// Struct
#pragma pack(push, 1)

typedef struct kMutexStruct {
    volatile QWORD taskId;
    volatile DWORD lockCnt;

    volatile BOOL lockFlag;

    BYTE pad[3];    // Padding added for 8 byte alignment
} MUTEX;

#pragma pack(pop);

// Function
BOOL kLockForSystemData(void);
void kUnlockForSystemData(BOOL interruptFlag);

void kInitializeMutex(MUTEX *pMutex);
void kLock(MUTEX *pMutex);
void kUnock(MUTEX *pMutex);

#endif  // __SYNC_H__