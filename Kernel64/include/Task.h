#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"

// Macro
// SS, RSP, RFLAGS, CS, RIP + ISR에서 저장하는 19개 레지스터

#define TASK_REGISTER_COUNT (5 + 19)
#define TASK_REGISTER_SIZE  (8)

// Context Register Offset
#define TASK_GS_OFFSET      (0)
#define TASK_FS_OFFSET      (1)
#define TASK_ES_OFFSET      (2)
#define TASK_DS_OFFSET      (3)
#define TASK_R15_OFFSET     (4)
#define TASK_R14_OFFSET     (5)
#define TASK_R13_OFFSET     (6)
#define TASK_R12_OFFSET     (7)
#define TASK_R11_OFFSET     (8)
#define TASK_R10_OFFSET     (9)
#define TASK_R9_OFFSET      (10)
#define TASK_R8_OFFSET      (11)
#define TASK_RSI_OFFSET     (12)
#define TASK_RDI_OFFSET     (13)
#define TASK_RDX_OFFSET     (14)
#define TASK_RCX_OFFSET     (15)
#define TASK_RBX_OFFSET     (16)
#define TASK_RAX_OFFSET     (17)
#define TASK_RBP_OFFSET     (18)
#define TASK_RIP_OFFSET     (19)
#define TASK_CS_OFFSET      (20)
#define TASK_RFLAGS_OFFSET  (21)
#define TASK_RSP_OFFSET     (22)
#define TASK_SS_OFFSET      (23)

// Address For Task Pool
#define TASK_TCB_POOL_ADDR          (0x800000)
#define TASK_MAX_COUNT              (1024)

// Address & Size For Stack Pool
#define TASK_STACK_POOL_ADDR        (TASK_TCB_POOL_ADDR + sizeof(TCB) * TASK_MAX_COUNT)
#define TASK_STACK_SIZE             (8192)

// Invalid Task ID
#define TASK_INVALID_ID             (0xFFFFFFFFFFFFFFFF)

// Processor Time that Task Can Use Maximum
#define TASK_PROCESSOR_TIME         (5) // ms

// Max Count of Ready List
#define TASK_MAX_READY_LIST_COUNT   (5)

// Priroty List of Tasks
#define TASK_PRIORITY_HIGHEST       (0)
#define TASK_PRIORITY_HIGH          (1)
#define TASK_PRIORITY_MEDIUM        (2)
#define TASK_PRIORITY_LOW           (3)
#define TASK_PRIORITY_LOWEST        (4)
#define TASK_PRIORITY_WAIT          (0xFF)

// Flags of Tasks
#define TASK_FLAGS_ENDTASK          (0x8000000000000000)
#define TASK_FLAGS_SYSTEM           (0x4000000000000000)
#define TASK_FLAGS_PROCESS          (0x2000000000000000)
#define TASK_FLAGS_THREAD           (0x1000000000000000)
#define TASK_FLAGS_IDLE             (0x0800000000000000)

// Macro for Priority
#define GETPRIORITY(x)              ((x) & 0xFF)
#define SETPRIORITY(x, pr)          ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | pr)
#define GETTCBOFFSET(x)             ((x) & 0xFFFFFFFF)

// 자식 스레드 링크에 연결된 threadLink에서 TCB(Task Control Block) 위치를 계산해 반환
#define GETTCBFROMTHREADLINK(x)     (TCB *)((QWORD)(x) - offsetof(TCB, threadLink));

// Struct
#pragma pack(push, 1)

typedef struct kContextStruct {
    QWORD registers[TASK_REGISTER_COUNT];
} CONTEXT;

// FPU Context를 위해, Struct Size를 16 Bytes Aligned
typedef struct kTaskControlBlockStruct {
    LISTLINK link;
    QWORD flags;

    void *pMemoryAddr;
    QWORD memorySize;

    LISTLINK threadLink;
    QWORD parentPid;

    QWORD fpuCtx[512/8];
    
    LIST childThreads;

    CONTEXT context;

    void *pStackAddr;
    QWORD stackSize;

    BOOL isFPUUsed;

    // Make struct TCB 16 Bytes Aligned
    char pad[11];
} TCB;

// Struct For Managing TCB Pool
typedef struct kTCBPoolManagerStruct {
    // Info for Task Pool
    TCB *pStartAddr;
    int maxCount;
    int useCount;

    // Count of allocated TCB
    int allocCount;
} TCBPOOLMANAGER;

// Struct For Managing Scheduler
typedef struct kSchedulerStruct {
    // TCB handle of Running Task
    TCB *pRunningTask;

    // Time that Task in running can use
    int processorTime;

    // Tasks ready to run
    LIST readyList[TASK_MAX_READY_LIST_COUNT];

    // List waiting to be ended
    LIST waitList;

    // Task execution frequency by priority
    int execFrequency[TASK_MAX_READY_LIST_COUNT];
    
    // Load of Processor
    QWORD processorLoad;

    // Processor time used for idle tasks 
    QWORD idleTime;

    // Task ID using FPU last time
    QWORD lastFPUUsedTaskID;
} SCHEDULER;

#pragma pack(pop)

// Function For Task & Task Pool
static void kInitializeTCBPool(void);
static TCB *kAllocateTCB(void);
static void kFreeTCB(QWORD id);
TCB *kCreateTask(QWORD flags, void *pMemoryAddr, QWORD memorySize, QWORD entryPointAddr);
static void kSetUpTask(TCB *pTCB, QWORD flags, QWORD entryPointerAddr, void *pStackAddr, QWORD stackSize);

// Function For Scheduler
void kInitializeScheduler(void);
void kSetRunningTask(TCB *pTask);
TCB *kGetRunningTask(void);
static TCB *kGetNextTaskToRun(void);
static BOOL kAddTaskToReadyList(TCB *pTask);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);
static TCB *kRemoveTaskFromReadyList(QWORD id);
BOOL kChangePriority(QWORD id, BYTE priority);
BOOL kEndTask(QWORD id);
void kExitTask(void);
int kGetReadyTaskCount(void);
int kGetTaskCount(void);
TCB *kGetTCBInTCBPool(int offset);
BOOL kIsTaskExist(QWORD id);
QWORD kGetProcessorLoad(void);
static TCB *kGetProcessByThread(TCB *pThread);

// Function For IDLE Task
void kIdleTask(void);
void kHaltProcessorByLoad(void);

// Function For FPU
QWORD kGetLastFPUUsedTaskID(void);
void kSetLastFPUUsedTaskID(QWORD taskID);

#endif  // __TASK_H__