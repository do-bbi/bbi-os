#include "BuddyMemory.h"
#include "Utility.h"
#include "Task.h"

static BUDDY_MEMORY gBuddyMemory;

// 동적 메모리 영역 초기화
void kInitializeBuddyMemory(void) {
    QWORD buddyMemSize;
    int i, j;

    BYTE *pCurPosBitmap;
    int blockCntLevel, metaBlockCnt;

    // 동적 메모리 영역으로 사용할 메모리 크기를 이용해 블록 관리
    // 필요 메모리 크기를, 최소 블록 단위로 계산
    buddyMemSize = kCalcBuddyMemorySize();
    metaBlockCnt = kCalcMetaBlockCount(buddyMemSize);

    // 전체 블록 개수에서, 관리에 필요한 메타블록 개수를 제외
    gBuddyMemory.minBlockCnt = (buddyMemSize / MIN_BUDDY_MEMORY_SIZE) - metaBlockCnt;

    // 최대 몇 개의 블록 리스트로 구성되는지 계산
    for(i = 0; (gBuddyMemory.minBlockCnt >> i) > 0; ++i);
    gBuddyMemory.maxLevelCnt = i;

    // 블록 리스트의 인덱스 저장 영역 초기화
    gBuddyMemory.pAllocBlockListIdx = (BYTE *)BUDDY_MEMORY_START_ADDR;
    for(i = 0; i < gBuddyMemory.minBlockCnt; ++i)
        gBuddyMemory.pAllocBlockListIdx[i] = 0xFF;

    // 비트맵 자료구조의 시작 주소 지정
    gBuddyMemory.pLevelBitmap = (BITMAP *)(BUDDY_MEMORY_START_ADDR + 
                                           gBuddyMemory.minBlockCnt * sizeof(BYTE));
    
    // 실제 Bitmap 주소 지정
    pCurPosBitmap = (BYTE *)(gBuddyMemory.pLevelBitmap + sizeof(BITMAP) * gBuddyMemory.maxLevelCnt);

    // 블록 리스트 별로 Bitmap 생성
    // 초기 상태는 가장 큰 블록과 잔여 블록들로만 구성
    for(j = 0; j < gBuddyMemory.minBlockCnt; ++j) {
        gBuddyMemory.pLevelBitmap[j].pBitmap = pCurPosBitmap;
        gBuddyMemory.pLevelBitmap[j].existBitCnt = 0;

        blockCntLevel = gBuddyMemory.minBlockCnt >> j;

        // 블록이 8개 이상 남았다면, 상위 블록과 결합 가능하므로, 모두 비어 있는 것으로 설정
        for(i = 0; i < blockCntLevel / 8; ++i)
            *pCurPosBitmap++ = 0x00;
        
        // 8개 미만의 잔여 블록 처리
        if(blockCntLevel % 8) {
            *pCurPosBitmap = 0x00;

            // if 남은 블록 개수 == 홀수 -> 마지막 한 블록은 상위 블록 이동 불가
            i = blockCntLevel % 8;
            if(i % 2) {
                *pCurPosBitmap |= (BUDDY_MEMORY_EXIST << (i - 1));
                gBuddyMemory.pLevelBitmap[j].existBitCnt = 1;
            }

            pCurPosBitmap++;
        }
    }

    // 블록 풀의 주소와 사용된 메모리 크기 설정
    gBuddyMemory.startAddr = BUDDY_MEMORY_START_ADDR + metaBlockCnt * MIN_BUDDY_MEMORY_SIZE;
    gBuddyMemory.endAddr = BUDDY_MEMORY_START_ADDR + kCalcBuddyMemorySize();
    gBuddyMemory.usedSize = 0;
}

void *kAllocateMemory(QWORD size);
BOOL kFreeMemory(void *pAddr);
void kGetBuddyMemoryInfo(QWORD *pBuddyMemoryStartAddr, QWORD *pBuddyMemoryTotalSize, 
                        QWORD *pMetaDataSize, QWORD *pUsedMemorySize);

BUDDY_MEMORY *kGetBuddyMemoryManager(void);

static QWORD kCalcBuddyMemorySize(void);
static int kCalcMetaBlockCount(QWORD memorySize);
static int kAllocBuddyBlock(QWORD alignedSize);
static QWORD kGetBuddyBlockSize(QWORD size);
static int kGetBlockListIdxofMatchSize(QWORD alignedSize);
static int kFindFreeBlockInBitmap(int blockListIdx);
static void kSetFlagInBitmap(int blockListIdx, int offset, BYTE flag);
static BOOL kFreeBuddyBlock(int blockListIdx, int blkOffset);
static BYTE kGetFlagInBitmap(int blockListIdx, int offset);