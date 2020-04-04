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
#define TASK_TCB_POOL_ADDR      (0x800000)
#define TASK_MAX_COUNT          (1024)

// Address & Size For Stack Pool
#define TASK_STACK_POOL_ADDR    (TASK_TCB_POOL_ADDR + sizeof(TCB) * TASK_MAX_COUNT)
#define TASK_STACK_SIZE         (8192)

// Invalid Task ID
#define TASK_INVALID_ID         (0xFFFFFFFFFFFFFFFF)

// Processor Time that Task Can Use Maximum
#define TASK_PROCESSOR_TIME     (5) // ms

// Struct
#pragma pack(push, 1)

typedef struct kContextStruct {
    QWORD registers[TASK_REGISTER_COUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct {
    LISTLINK link;
    CONTEXT context;
    
    QWORD flags;

    void *pStackAddr;
    QWORD stackSize;
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
    LIST readyList;
} SCHEDULER;

#pragma pack(pop)

// Function For Task & Task Pool
void kInitializeTCBPool(void);
TCB *kAllocateTCB(void);
void kFreeTCB(QWORD id);
TCB *kCreateTask(QWORD flags, QWORD entryPointAddr);
void kSetUpTask(TCB *pTCB, QWORD flags, QWORD entryPointerAddr, void *pStackAddr, QWORD stackSize);

// Function For Scheduler
void kInitializeScheduler(void);
void kSetRunningTask(TCB *pTask);
TCB *kGetRunningTask(void);
TCB *kGetNextTaskToRun(void);
void kAddTaskToReadyList(TCB *pTask);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);

#endif  // __TASK_H__