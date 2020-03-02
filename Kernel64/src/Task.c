#include "Task.h"
#include "Descriptor.h"

// Set TCB using Function Paramter
void kSetUpTask(TCB *pTCB, QWORD id, QWORD flags, QWORD entryPointAddr, void *pStackAddr, QWORD stackSize) {
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
    pTCB->id = id;
    pTCB->pStackAddr = pStackAddr;
    pTCB->stackSize = stackSize;
    pTCB->flags = flags;
}