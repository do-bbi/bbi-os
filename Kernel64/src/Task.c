#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "Sync.h"

// Structure for Scheduling
static SCHEDULER        gScheduler;
static TCBPOOLMANAGER   gTCBPoolManager;

// Initialize Task Pool
static void kInitializeTCBPool(void) {
    int i;

    kMemSet(&gTCBPoolManager, 0, sizeof(gTCBPoolManager));

    gTCBPoolManager.pStartAddr = (TCB *)TASK_TCB_POOL_ADDR;
    kMemSet(TASK_TCB_POOL_ADDR, 0, sizeof(TCB) * TASK_MAX_COUNT);

    for(i = 0; i < TASK_MAX_COUNT; ++i)
        gTCBPoolManager.pStartAddr[i].link.id = i;

    gTCBPoolManager.maxCount = TASK_MAX_COUNT;
    gTCBPoolManager.allocCount = 1;
}

// Allocate TCB
static TCB *kAllocateTCB(void) {
    TCB *pEmptyTCB;
    int i;

    if(gTCBPoolManager.useCount == gTCBPoolManager.maxCount)
        return NULL;

    for(i = 0; i < gTCBPoolManager.maxCount; ++i) {
        // if id[32] == 0, then non-allocated 
        if((gTCBPoolManager.pStartAddr[i].link.id >> 32) == 0) {
            pEmptyTCB = &(gTCBPoolManager.pStartAddr[i]);
            break;
        }
    }

    pEmptyTCB->link.id = ((QWORD) gTCBPoolManager.allocCount << 32) | i;
    gTCBPoolManager.useCount++;
    gTCBPoolManager.allocCount++;

    if(gTCBPoolManager.allocCount == 0)
        gTCBPoolManager.allocCount = 1;

    return pEmptyTCB;
}

// Release TCB
static void kFreeTCB(QWORD id) {
    int i;

    // Lower bits[31:0] of TASK ID is index
    i = GETTCBOFFSET(id);

    // Init TCB & Set ID
    kMemSet(&gTCBPoolManager.pStartAddr[i].context, 0, sizeof(CONTEXT));
    gTCBPoolManager.pStartAddr[i].link.id = i;

    gTCBPoolManager.useCount--;
}

// Create Task
TCB *kCreateTask(QWORD flags, QWORD entryPointAddr) {
    TCB *pTask;
    void *pStackAddr;
    BOOL prevFlag;

    prevFlag = kLockForSystemData();

    pTask = kAllocateTCB();
    if(pTask == NULL) {
        kUnlockForSystemData(prevFlag);
        return NULL;
    }
    
    kUnlockForSystemData(prevFlag);
    pStackAddr = (void *)(TASK_STACK_POOL_ADDR + (TASK_STACK_SIZE * GETTCBOFFSET(pTask->link.id)));

    // TCB를 설정한 후 준비 리스트에 삽입하여 스케줄링될 수 있도록 함
    kSetUpTask(pTask, flags, entryPointAddr, pStackAddr, TASK_STACK_SIZE);

    prevFlag = kLockForSystemData();
    kAddTaskToReadyList(pTask);
    kUnlockForSystemData(prevFlag);

    return pTask;
}

// Set TCB using Function Paramter
static void kSetUpTask(TCB *pTCB, QWORD flags, QWORD entryPointAddr, void *pStackAddr, QWORD stackSize) {
    // 콘텍스트 초기화
    kMemSet(pTCB->context.registers, 0, sizeof(pTCB->context.registers));
    
    // 스택에 관련된 RSP, RBP 레지스터 설정
    pTCB->context.registers[TASK_RSP_OFFSET] = (QWORD)pStackAddr + stackSize;
    pTCB->context.registers[TASK_RBP_OFFSET] = (QWORD)pStackAddr + stackSize;

    // 세그먼트 셀렉터 설정
    pTCB->context.registers[TASK_CS_OFFSET] = GDT_KERNEL_CODE_SEGMENT;
    pTCB->context.registers[TASK_DS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
    pTCB->context.registers[TASK_ES_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
    pTCB->context.registers[TASK_FS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
    pTCB->context.registers[TASK_GS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
    pTCB->context.registers[TASK_SS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;

    // RIP 레지스터와 인터럽트 플래그 설정
    pTCB->context.registers[TASK_RIP_OFFSET] = entryPointAddr;

    // RFLAGS 레지스터의 IF 비트(비트 9)를 1로 설정하여 인터럽트 활성화
    pTCB->context.registers[TASK_RFLAGS_OFFSET] |= (0x1 << 9);
    
    // ID 및 스택, 그리고 플래그 저장
    pTCB->pStackAddr = pStackAddr;
    pTCB->stackSize = stackSize;
    pTCB->flags = flags;
}

// Function For Scheduler
// Initialize Scheduler
void kInitializeScheduler(void) {
    int i;

    kInitializeTCBPool();

    for(i = 0; i < TASK_MAX_READY_LIST_COUNT; ++i) {
        kInitializeList(&(gScheduler.readyList[i]));
        gScheduler.execFrequency[i] = 0;
    }
    kInitializeList(&(gScheduler.waitList));

    // Allocate Task & Set Priority to HIGHEST
    gScheduler.pRunningTask = kAllocateTCB();
    gScheduler.pRunningTask->flags = TASK_PRIORITY_HIGHEST;

    // Used to calculate Processor Usage rate
    gScheduler.idleTime = 0;
    gScheduler.processorLoad = 0;
}

// Set Task which is Running
void kSetRunningTask(TCB *pTask) {
    BOOL prevFlag;

    prevFlag = kLockForSystemData();
    gScheduler.pRunningTask = pTask;
    kUnlockForSystemData(prevFlag);
}

// Get Task which is Running
TCB *kGetRunningTask(void) {
    BOOL prevFlag;
    TCB *pRunningTask;

    prevFlag = kLockForSystemData();
    pRunningTask = gScheduler.pRunningTask;
    kUnlockForSystemData(prevFlag);

    return pRunningTask;
}

// Get Next Task from Task List
static TCB *kGetNextTaskToRun(void) {
    TCB *pTarget = NULL;
    int taskCount, i, j;

    // If there is a task in the task queue, 
    // or tasks in all queues are executed once, all queues may yield to the processor
    // so that the task cannot be selected.
    for(j = 0; j < 2; ++j) {
        // Select a task to schedule by checking the list from high priority to low priority
        for(i = 0; i < TASK_MAX_READY_LIST_COUNT; ++i) {
            taskCount = kGetListCount(&(gScheduler.readyList[i]));

            if(taskCount > gScheduler.execFrequency[i]) {
                pTarget = (TCB *)kRemoveListFromHead(&(gScheduler.readyList[i]));
                gScheduler.execFrequency[i]++;
                break;
            }
            else
                gScheduler.execFrequency[i] = 0;
        }

        if(pTarget != NULL)
            break;
    }

    return pTarget;
}

// Add Task to Tail of Task List
static BOOL kAddTaskToReadyList(TCB *pTask) {
    BYTE priority;

    priority = GETPRIORITY(pTask->flags);
    if(TASK_MAX_READY_LIST_COUNT <= priority)
        return FALSE;

    kAddListToTail(&(gScheduler.readyList[priority]), pTask);
    return TRUE;
}

// Context Switching - Don't call this function at interrupt/exception handler
void kSchedule(void) {
    TCB *pRunningTask, *pNextTask;
    BOOL prevFlag;

    // Check is there any task to run
    if(kGetReadyTaskCount() < 1)
        return;
    
    // Deactivate Interrupt to block context switching by interrupts
    prevFlag = kLockForSystemData();

    // Get Next Task to run
    pNextTask = kGetNextTaskToRun();
    if(pNextTask != NULL) {
        pRunningTask = gScheduler.pRunningTask;
        gScheduler.pRunningTask = pNextTask;

        if((pRunningTask->flags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
            gScheduler.idleTime += TASK_PROCESSOR_TIME - gScheduler.processorTime;

        if(pRunningTask->flags & TASK_FLAGS_ENDTASK) {
            kAddListToTail(&(gScheduler.waitList), pRunningTask);
            kSwitchContext(NULL, &(pNextTask->context));
        }
        else {
            kAddTaskToReadyList(pRunningTask);
            kSwitchContext(&(pRunningTask->context), &(pNextTask->context));
        }

        gScheduler.processorTime = TASK_PROCESSOR_TIME;  // 5 tick(ms)
    }

    kUnlockForSystemData(prevFlag);
}

// Schedule only For Interrupt/Exception Handler
BOOL kScheduleInInterrupt(void) {
    TCB *pRunningTask, *pNextTask;
    char *pContextAddress;
    BOOL prevFlag;

    prevFlag = kLockForSystemData();

    // Get Next Task to run
    pNextTask = kGetNextTaskToRun();
    if(pNextTask == NULL) {
        kUnlockForSystemData(prevFlag);
        return FALSE;
    }

    // Context Switching to Interrupt/Exception Handler
    pContextAddress = (char *)IST_BASE_ADDR + IST_SIZE - sizeof(CONTEXT);

    pRunningTask = gScheduler.pRunningTask;
    gScheduler.pRunningTask = pNextTask;

    if( (pRunningTask->flags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gScheduler.idleTime += TASK_PROCESSOR_TIME;

    // Update Available Processor time
    gScheduler.processorTime = TASK_PROCESSOR_TIME; // 5 tick(ms)

    if( pRunningTask->flags & TASK_FLAGS_ENDTASK)
        kAddListToTail(&(gScheduler.waitList), pRunningTask);
    else {
        kMemCpy(&(pRunningTask->context), pContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pRunningTask);
    }

    kUnlockForSystemData(prevFlag);

    // Set "Running Task" to NextTask(already got)
    // And Memory Copy it to IST for context swtiching automatically
    kMemCpy(pContextAddress, &(pNextTask->context), sizeof(CONTEXT));

    return TRUE;
}

// Decrease Processor Time
void kDecreaseProcessorTime(void) {
    if(gScheduler.processorTime > 0)
        gScheduler.processorTime--;
}

// Check Processor Time is Expired
BOOL kIsProcessorTimeExpired(void) {
    return (gScheduler.processorTime <= 0);
}

static TCB *kRemoveTaskFromReadyList(QWORD id) {
    TCB *pTarget;
    BYTE priority;

    if(GETTCBOFFSET(id) >= TASK_MAX_COUNT)
        return NULL;

    pTarget = &(gTCBPoolManager.pStartAddr[GETTCBOFFSET(id)]);
    if(pTarget->link.id != id)
        return NULL;

    priority = GETPRIORITY(pTarget->flags);
    
    pTarget = kRemoveList(&(gScheduler.readyList[priority]), id);
    
    return pTarget;
}

BOOL kChangePriority(QWORD id, BYTE priority) {
    TCB *pTarget;
    BOOL prevFlag;

    if(TASK_MAX_READY_LIST_COUNT < priority)
        return FALSE;

    prevFlag = kLockForSystemData();

    pTarget = gScheduler.pRunningTask;
    if(pTarget->link.id == id)
        SETPRIORITY(pTarget->flags, priority);
    else {
        pTarget = kRemoveTaskFromReadyList(id);

        if(pTarget == NULL) {
            pTarget = kGetTCBInTCBPool(GETTCBOFFSET(id));
            if(pTarget)
                SETPRIORITY(pTarget->flags, priority);
        }
        else {
            SETPRIORITY(pTarget->flags, priority);
            kAddTaskToReadyList(pTarget);
        }
    }
    kUnlockForSystemData(prevFlag);

    return TRUE;
}

BOOL kEndTask(QWORD id) {
    TCB *pTarget;
    BYTE priority;
    BOOL prevFlag;

    prevFlag = kLockForSystemData();

    pTarget = gScheduler.pRunningTask;
    if(pTarget->link.id == id) {
        pTarget->flags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pTarget->flags, TASK_FLAGS_IDLE);

        kUnlockForSystemData(prevFlag);

        kSchedule();

        // Never Executed, For Debugging
        while(TRUE);
    }
    else {
        pTarget = kRemoveTaskFromReadyList(id);
        if(pTarget == NULL) {
            pTarget = kGetTCBInTCBPool(GETTCBOFFSET(id));
            if(pTarget) {
                pTarget->flags |= TASK_FLAGS_ENDTASK;
                SETPRIORITY(pTarget->flags, TASK_FLAGS_IDLE);
            }
            
            kUnlockForSystemData(prevFlag);
            return FALSE;
        }

        pTarget->flags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pTarget->flags, TASK_FLAGS_IDLE);
        kAddListToTail(&(gScheduler.waitList), pTarget);
    }

    kUnlockForSystemData(prevFlag);
    return TRUE;
}

void kExitTask(void) {
    kEndTask(gScheduler.pRunningTask->link.id);
}

int kGetReadyTaskCount(void) {
    int totalCount = 0;
    int i;
    BOOL prevFlag;

    prevFlag = kLockForSystemData();
    for(i = 0; i < TASK_MAX_READY_LIST_COUNT; ++i)
        totalCount += kGetListCount(&(gScheduler.readyList[i]));
    kUnlockForSystemData(prevFlag);

    return totalCount;
}

int kGetTaskCount(void) {
    int totalCount;
    BOOL prevFlag;

    // Tasks in Ready Queue
    totalCount = kGetReadyTaskCount();

    prevFlag = kLockForSystemData();
    // Tasks in Waiting Queue + 1(Current Task)
    totalCount += kGetListCount(&(gScheduler.waitList)) + 1;
    kUnlockForSystemData(prevFlag);

    return totalCount;
}

TCB *kGetTCBInTCBPool(int offset) {
    if(offset < -1 || TASK_MAX_COUNT < offset)
        return NULL;

    return &(gTCBPoolManager.pStartAddr[offset]);
}

BOOL kIsTaskExist(QWORD id) {
    TCB *pTCB;

    pTCB = kGetTCBInTCBPool(GETTCBOFFSET(id));

    return (pTCB != NULL) && (pTCB->link.id == id);
}

QWORD kGetProcessorLoad(void) {
    return gScheduler.processorLoad;
}

void kIdleTask(void) {
    TCB *pTask;
    QWORD lastMeasureTickCount, lastSpendTickInIdleTask;
    QWORD curMeasureTickCount, curSpendTickInIdleTask;

    BOOL prevFlag;
    QWORD taskId;

    lastSpendTickInIdleTask = gScheduler.idleTime;
    lastMeasureTickCount = kGetTickCount();

    while(TRUE) {
        curMeasureTickCount = kGetTickCount();
        curSpendTickInIdleTask = gScheduler.idleTime;

        // Calculate Processor Usage Rate
        // 100 - (Processor Idle Time) * 100 / (Total Processor Time)
        if((curMeasureTickCount - lastMeasureTickCount) == 0)
            gScheduler.processorLoad = 0;
        else {
            gScheduler.processorLoad = 100 - 
                (curSpendTickInIdleTask - lastSpendTickInIdleTask) * 100 / 
                (curMeasureTickCount - lastMeasureTickCount);
        }

        lastMeasureTickCount = curMeasureTickCount;
        lastSpendTickInIdleTask = curSpendTickInIdleTask;

        kHaltProcessorByLoad();

        if(0 <= kGetListCount(&(gScheduler.waitList))) {
            while(TRUE) {
                prevFlag = kLockForSystemData();
                pTask = kRemoveListFromHead(&(gScheduler.waitList));
                if(pTask == NULL) {
                    kUnlockForSystemData(prevFlag);
                    break;
                }
                taskId = pTask->link.id;
                kFreeTCB(pTask->link.id);

                kUnlockForSystemData(prevFlag);
                
                kPrintf("IDLE - Task #0x%q is completely ended\n", pTask->link.id);
            }
        }

        kSchedule();
    }
}

void kHaltProcessorByLoad(void) {
    if(gScheduler.processorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    }
    else if(gScheduler.processorLoad < 80) {
        kHlt();
        kHlt();
    }
    else if(gScheduler.processorLoad < 95) {
        kHlt();
    }
}