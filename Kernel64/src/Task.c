#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"

// Structure for Scheduling
static SCHEDULER        gScheduler;
static TCBPOOLMANAGER   gTCBPoolManager;

// Initialize Task Pool
void kInitializeTCBPool(void) {
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
TCB *kAllocateTCB(void) {
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
void kFreeTCB(QWORD id) {
    int i;

    // Lower bits[31:0] of TASK ID is index
    i = id & 0xFFFFFFFF;

    // Init TCB & Set ID
    kMemSet(&gTCBPoolManager.pStartAddr[i].context, 0, sizeof(CONTEXT));
    gTCBPoolManager.pStartAddr[i].link.id = i;

    gTCBPoolManager.useCount--;
}

// Create Task
TCB *kCreateTask(QWORD flags, QWORD entryPointAddr) {
    TCB *pTask;
    void *pStackAddr;

    pTask = kAllocateTCB();
    if(pTask == NULL)
        return NULL;

    pStackAddr = (void *)(TASK_STACK_POOL_ADDR + (TASK_STACK_SIZE * (pTask->link.id & 0xFFFFFFFF)));

    // TCB를 설정한 후 준비 리스트에 삽입하여 스케줄링될 수 있도록 함
    kSetUpTask(pTask, flags, entryPointAddr, pStackAddr, TASK_STACK_SIZE);
    kAddTaskToReadyList(pTask);

    return pTask;
}

// Set TCB using Function Paramter
void kSetUpTask(TCB *pTCB, QWORD flags, QWORD entryPointAddr, void *pStackAddr, QWORD stackSize) {
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
    kInitializeTCBPool();

    kInitializeList(&(gScheduler.readyList));

    gScheduler.pRunningTask = kAllocateTCB();
}

// Set Task which is Running
void kSetRunningTask(TCB *pTask) {
    gScheduler.pRunningTask = pTask;
}

// Get Task which is Running
TCB *kGetRunningTask(void) {
    return gScheduler.pRunningTask;
}

// Get Next Task from Task List
TCB *kGetNextTaskToRun(void) {
    if(kGetListCount(&(gScheduler.readyList)) == 0)
        return NULL;

    return (TCB *)kRemoveListFromHead(&(gScheduler.readyList));
}

// Add Task to Tail of Task List
void kAddTaskToReadyList(TCB *pTask) {
    kAddListToTail(&(gScheduler.readyList), pTask);
}

// Context Switching - Don't call this function at interrupt/exception handler
void kSchedule(void) {
    TCB *pRunningTask, *pNextTask;
    BOOL bPrevFlag;

    // Check is there any task to run
    if(kGetListCount(&(gScheduler.readyList)) == 0)
        return;
    
    // Deactivate Interrupt to block context switching by interrupts
    bPrevFlag = kSetInterruptFlag(FALSE);

    // Get Next Task to run
    pNextTask = kGetNextTaskToRun();
    if(pNextTask != NULL) {
        pRunningTask = gScheduler.pRunningTask;
        kAddTaskToReadyList(pRunningTask);

        // Set "next task" as the Currently Running Task & Context Switching
        gScheduler.pRunningTask = pNextTask;
        kSwitchContext(&(pRunningTask->context), &(pNextTask->context));   

        // Update Available Processor time
        gScheduler.processorTime = TASK_PROCESSOR_TIME; // 5 tick(ms)
    }

    kSetInterruptFlag(bPrevFlag);
}

// Schedule only For Interrupt/Exception Handler
BOOL kScheduleInInterrupt(void) {
    TCB *pRunningTask, *pNextTask;
    char *pContextAddress;

    // Get Next Task to run
    pNextTask = kGetNextTaskToRun();
    if(pNextTask == NULL)
        return FALSE;

    // Context Switching to Interrupt/Exception Handler
    pContextAddress = (char *)IST_BASE_ADDR + IST_SIZE - sizeof(CONTEXT);

    pRunningTask = gScheduler.pRunningTask;
    kMemCpy(&(pRunningTask->context), pContextAddress, sizeof(CONTEXT));
    kAddTaskToReadyList(pRunningTask);

    // Set "next task" as the Currently Running Task & Context Switching
    gScheduler.pRunningTask = pNextTask;
    kMemCpy(pContextAddress, &(pNextTask->context), sizeof(CONTEXT));

    // Update Available Processor time
    gScheduler.processorTime = TASK_PROCESSOR_TIME; // 5 tick(ms)

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