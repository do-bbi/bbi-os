#include "Sync.h"
#include "Utility.h"
// #include "AssemblyUtil.h"
#include "Task.h"

// Locking Function For Global System Data
BOOL kLockForSystemData(void) {
    return kSetInterruptFlag(FALSE);
};

void kUnlockForSystemData(BOOL interruptFlag) {
    kSetInterruptFlag(interruptFlag);
};

void kInitializeMutex(MUTEX *pMutex) {
    pMutex->lockFlag = FALSE;
    pMutex->lockCnt = 0;
    pMutex->taskId = TASK_INVALID_ID;
}

void kLock(MUTEX *pMutex) {
    if( kTestAndSet(&pMutex->lockFlag, 0, 1) == FALSE ) {
        if(pMutex->taskId == kGetRunningTask()->link.id) {
            pMutex->lockCnt++;
            return;
        }

        while( kTestAndSet(&pMutex->lockFlag, 0, 1) == FALSE )
            kSchedule();
    }

    pMutex->lockCnt = 1;
    pMutex->taskId = kGetRunningTask()->link.id;
}

void kUnlock(MUTEX *pMutex) {
    if(pMutex->lockFlag == FALSE || pMutex->taskId != kGetRunningTask()->link.id)
        return;

    if(1 < pMutex->lockCnt) {
        pMutex->lockCnt--;
        return;
    }

    pMutex->taskId = TASK_INVALID_ID;
    pMutex->lockCnt = 0;
    pMutex->lockFlag = FALSE;
}
