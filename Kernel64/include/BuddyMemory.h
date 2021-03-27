#ifndef __BUDDY_MEMORY_H__
#define __BUDDY_MEMORY_H__

#include "Types.h"
#include "Task.h"

// Macro
// 동적 메모리 영역 시작 주소(1 MB Aligned)
#define BUDDY_MEMORY_START_ADDR   ((TASK_STACK_POOL_ADDR + \
    (TASK_STACK_SIZE * TASK_MAX_COUNT) + 0xFFFFF) & 0xFFFFFFFFFFF00000)

// Size of Smallest Buddy Block = 1 KB
#define MIN_BUDDY_MEMORY_SIZE     (1 * 1024)

// Bitmap Flag
#define BUDDY_MEMORY_EXIST        (0x01)
#define BUDDY_MEMORY_EMPTY        (0x00)

// Struct
typedef struct kBitmapStruct {
    BYTE *pBitmap;
    QWORD existBitCnt;
} BITMAP;

// Buddy Block Manager
typedef struct kBuddyMemoryManagerStruct {
    int maxLevelCnt;    // 블록 리스트 총 개수
    int minBlockCnt;    // 최소크기 블록 개수
    QWORD usedSize;     // 할당 된 메모리 크기

    // Address of Block Pool
    QWORD startAddr;
    QWORD endAddr;

    BYTE *pAllocBlockListIdx;
    BITMAP *pLevelBitmap;
} BUDDY_MEMORY;

// Function
void kInitializeBuddyMemory(void);
void *kAllocateMemory(QWORD size);
BOOL kFreeMemory(void *pAddr);
void kGetBuddyMemoryInfo(QWORD *pBuddyMemoryStartAddr, QWORD *pBuddyMemoryTotalSize, 
                        QWORD *pMetaDataSize, QWORD *pUsedMemorySize);

BUDDY_MEMORY *kGetBuddyMemoryManager(void);

static QWORD kCalcBuddyMemorySize(void);
static int kCalcMetaBlockCount(QWORD memorySize);
static int kAllocBuddyBlock(QWORD alignedSize);
static QWORD kGetBuddyBlockSize(QWORD size);
static int kGetBlockListIdxOfMatchSize(QWORD alignedSize);
static int kFindFreeBlockInBitmap(int blockListIdx);
static void kSetFlagInBitmap(int blockListIdx, int offset, BYTE flag);
static BOOL kFreeBuddyBlock(int blockListIdx, int blkOffset);
static BYTE kGetFlagInBitmap(int blockListIdx, int offset);

#endif  // __BUDDY_MEMORY_H__