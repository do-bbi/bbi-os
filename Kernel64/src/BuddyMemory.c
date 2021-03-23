#include "BuddyMemory.h"
#include "Sync.h"
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
                                    (gBuddyMemory.minBlockCnt * sizeof(BYTE)));
    
    // 실제 Bitmap 주소 지정
    pCurPosBitmap = ((BYTE *)gBuddyMemory.pLevelBitmap) + (sizeof(BITMAP) * gBuddyMemory.maxLevelCnt);

    // 블록 리스트 별로 Bitmap 생성
    // 초기 상태는 가장 큰 블록과 잔여 블록들로만 구성
    for(j = 0; j < gBuddyMemory.maxLevelCnt; ++j) {
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

// 버디 메모리 영역의 크기 계산
static QWORD kCalcBuddyMemorySize(void) {
    QWORD ramSize;

    // 비디오 메모리, 프로세서의 레지스터가 3GB 이상의 메모리 영역 존재
    // Buddy Memory는 최대 3GB 까지만 사용
    ramSize = (kGetTotalRAMSize() * 1024 * 1024);
    if(ramSize > (QWORD) 3 * 1024 * 1024 * 1024)
        ramSize = (QWORD) 3 * 1024 * 1024 * 1024;

    return ramSize - BUDDY_MEMORY_START_ADDR;
}

// 버디 메모리 관리에 필요한 정보가 차지하는 공간을 계산
// (최소 블록 단위로 정렬해 반환)
static int kCalcMetaBlockCount(QWORD buddyMemSize) {
    long minBlockCnt;
    DWORD allocBlockListIdxSize;
    DWORD bitmapSize;

    long i;

    // 가장 크기가 작은 블록의 개수를 계산하여 이를 기준으로 
    // 비트맵 영역과 할당된 크기를 저장하는 영역을 계산
    minBlockCnt = buddyMemSize / MIN_BUDDY_MEMORY_SIZE;

    // 할당된 블록이 속한 블록 리스트의 인덱스를 보관 필요 영역 계산
    allocBlockListIdxSize = minBlockCnt * sizeof(BYTE);

    // 비트맵을 저장하는데 필요한 공간 계산
    bitmapSize = 0;
    for(i = 0; (minBlockCnt >> i) > 0; ++i) {
        bitmapSize += sizeof(BITMAP);
        bitmapSize += ((minBlockCnt >> i) + 7) / 8;
    }

    return (allocBlockListIdxSize + bitmapSize + MIN_BUDDY_MEMORY_SIZE - 1) / MIN_BUDDY_MEMORY_SIZE;
}

// 메모리 할당
void *kAllocateMemory(QWORD size) {
    QWORD alignedSize;
    QWORD relativeAddr;
    long offset;
    int blockListIdx;

    // 메모리 크기를 버디 블록의 크기로 맞춤
    alignedSize = kGetBuddyBlockSize(size);

    if(alignedSize == 0)
        return NULL;
    
    // 여유 공간이 없다면 할당 불가
    if((gBuddyMemory.startAddr + gBuddyMemory.usedSize + alignedSize) > gBuddyMemory.endAddr)
        return NULL;
    
    // 버디 블록 할당 후, 할당한 블록이 속한 블록 리스트의 Index 반환
    offset = kAllocBuddyBlock(alignedSize);
    if(offset == -1)
        return NULL;

    blockListIdx = kGetBlockListIdxOfMatchSize(alignedSize);

    // 블록 크기를 저장하는 영역에 실제로 할당한 버디 블록이 속한 블록 리스트의 인덱스 저장
    // 메모리 해제할 때 이 블록 리스트의 인덱스 사용
    relativeAddr = alignedSize * offset;
    
    offset = relativeAddr / MIN_BUDDY_MEMORY_SIZE;
    gBuddyMemory.pAllocBlockListIdx[offset] = (BYTE)blockListIdx;
    gBuddyMemory.usedSize += alignedSize;

    return (void *)(relativeAddr + gBuddyMemory.startAddr);
}

// 가장 가까운 버디 블록 크기로 정렬된 크기 반환
static QWORD kGetBuddyBlockSize(QWORD size) {
    long i;

    for(i = 0; i < gBuddyMemory.maxLevelCnt; ++i) {
        if(size <= (MIN_BUDDY_MEMORY_SIZE << i))
            return (MIN_BUDDY_MEMORY_SIZE << i);
    }

    return 0;
}

// 버디 블록 알고리즘으로, 메모리 블록을 할당해 반환(정렬 된 크기로 요청)
static int kAllocBuddyBlock(QWORD alignedSize) {
    int blockListIdx;
    int i, offset;
    BOOL isPrevInterruptFlag;

    // 블록 크기를 만족하는 블록 리스트의 인덱스 검색
    blockListIdx = kGetBlockListIdxOfMatchSize(alignedSize);
    if(blockListIdx == -1)
        return -1;

    // 동기화
    isPrevInterruptFlag = kLockForSystemData();

    // 조건을 만족하는 블록 리스트부터 최상위 블록 리스트까지 검색하여 선택
    for(i = blockListIdx; i < gBuddyMemory.maxLevelCnt; ++i) {
        // 블록 리스트의 비트맵을 확인해 블록 존재 확인
        offset = kFindFreeBlockInBitmap(i);
        if(offset != -1)
            break;
    }

    if(offset == -1) {
        kUnlockForSystemData(isPrevInterruptFlag);
        return -1;
    }

    // 할당할 블록을 할당한 것으로 표시
    kSetFlagInBitmap(i, offset, BUDDY_MEMORY_EMPTY);

    // 상위 블록에서 블록을 찾았다면 상위 블록 분할
    if(i > blockListIdx) {
        // 검색 된 블록 리스트 ~ 검색 시작 블록 리스트까지 내려가면서
        // 왼쪽 블록은 Exist, 오른쪽 블록 Empty로 변경
        for(i = i - 1; i >= blockListIdx; --i) {
            // Left -> EMPTY
            kSetFlagInBitmap(i, offset << 1, BUDDY_MEMORY_EMPTY);
            // Right -> EXIST
            kSetFlagInBitmap(i, (offset << 1) | 1, BUDDY_MEMORY_EXIST);

            // 왼쪽 블록으로 이동
            offset = offset << 1;
        }
    }

    kUnlockForSystemData(isPrevInterruptFlag);

    return offset;
}

// 전달된 크기와 가장 근접한 블록 리스트 Index 반환
static int kGetBlockListIdxOfMatchSize(QWORD alignedSize) {
    int i;

    for(i = 0; i < gBuddyMemory.maxLevelCnt; ++i) {
        if(alignedSize <= (MIN_BUDDY_MEMORY_SIZE << i))
            return i;
    }

    return -1;
}

// 블록 리스트에서 Bitmap 검색 후, 블록이 존재하면 offset을 반환
static int kFindFreeBlockInBitmap(int blockListIdx) {
    int i, maxCnt;
    BYTE *pBitmap;
    QWORD *pBitmap64;

    // 비트맵에 데이터가 존재하지 않는다면 실패
    if(gBuddyMemory.pLevelBitmap[blockListIdx].existBitCnt == 0)
        return -1;
    
    // 블록 리스트에 존재하는 총 블록의 수를 구한 후, 그 개수만큼 비트맵 검색
    maxCnt = gBuddyMemory.minBlockCnt >> blockListIdx;
    pBitmap = gBuddyMemory.pLevelBitmap[blockListIdx].pBitmap;

    for(i = 0; i < maxCnt; ++i) {
        // QWORD는 8 * 8 = 64bit이므로, 64bit를 한꺼번에 검사해 Set(1) Bit가 있는지 확인
        if((maxCnt - i) / 64 > 0) {
            pBitmap64 = (QWORD *)&(pBitmap[i / 8]);
            // 만약 8 Bytes 모두 0이면 모두 제외
            if(*pBitmap64 == 0) {
                i += 63;
                continue;
            }
        }

        if(pBitmap[i / 8] & (BUDDY_MEMORY_EXIST << (i % 8)))
            return i;
    }

    return -1;
}

// Bitmap에 Flag 설정
static void kSetFlagInBitmap(int blockListIdx, int offset, BYTE flag) {
    BYTE *pBitmap;

    pBitmap = gBuddyMemory.pLevelBitmap[blockListIdx].pBitmap;

    if(flag == BUDDY_MEMORY_EXIST) {
        // 해당 위치에 데이터가 비어 있다면 ++ExistBitCount
        if((pBitmap[offset / 8] & (0x01 << (offset % 8))) == 0)
            gBuddyMemory.pLevelBitmap[blockListIdx].existBitCnt++;
        
        pBitmap[offset / 8] |= (0x1 << (offset % 8));
    }
    else {
        // 해당 위치에 데이터가 존재한다면 existBitCnt--
        if((pBitmap[offset / 8] & (0x01 << (offset % 8))) != 0)
            gBuddyMemory.pLevelBitmap[blockListIdx].existBitCnt--;
        pBitmap[offset / 8] &= ~(0x1 << (offset % 8));
    }
}

// 할당받은 메모리 해제
BOOL kFreeMemory(void *pAddr) {
    QWORD relativeAddr;
    int offset;
    QWORD blockSize;
    int blockListIdx;

    if(pAddr == NULL) {
        kPrintf("Free Buddy Block Failed (pAddr == NULL)\n");
        return FALSE;
    }
    
    // 넘겨 받은 주소를 Block Pool을 기준으로 하는 주소로 변환하여
    // 할당했던 블록의 Size를 검색
    relativeAddr = (QWORD)pAddr - gBuddyMemory.startAddr;
    offset = relativeAddr / MIN_BUDDY_MEMORY_SIZE;

    // 할당되어 있지 않은 경우 Free 실패
    if(gBuddyMemory.pAllocBlockListIdx[offset] == 0xFF) {
        kPrintf("Free Buddy Block Failed (BL == 0xFF)\n", offset);
        return FALSE;
    }
    
    // 할당된 블록이 속한 블록 리스트의 인덱스가 저장된 곳을 초기화
    blockListIdx = (int)gBuddyMemory.pAllocBlockListIdx[offset];
    gBuddyMemory.pAllocBlockListIdx[offset] = 0xFF;

    // 버디 블록의 최소 크기를 블록 리스트 인덱스로 Shift하여 할당한 블록 크기 계산
    blockSize = MIN_BUDDY_MEMORY_SIZE << blockListIdx;

    // 블록 리스트 내의 블록 Offset을 구해 블록 해제
    offset = relativeAddr / blockSize;
    if(kFreeBuddyBlock(blockListIdx, offset)) {
        gBuddyMemory.usedSize -= blockSize;
        return TRUE;
    }
    else
        kPrintf("Free Buddy Block Failed (%d, %d)\n", blockListIdx, offset);

    return FALSE;
}

static BOOL kFreeBuddyBlock(int blockListIdx, int blkOffset) {
    int i, offset;
    BOOL flag, isPrevInterruptFlag;

    // 동기화
    isPrevInterruptFlag = kLockForSystemData();

    // 블록 리스트의 끝까지 인접한 블록을 검사하여 결합할 수 없을 때까지 반복
    for(i = blockListIdx; i < gBuddyMemory.maxLevelCnt; ++i) {
        // 현재 블록은 Exist로 설정
        kSetFlagInBitmap(i, blkOffset, BUDDY_MEMORY_EXIST);

        // 블록 Offset이 Even(=왼쪽) -> 오른쪽 블록 검사
        // 블록 Offset이 Odd(=오른쪽) -> 왼쪽 블록 검사
        // 비트맵을 검사해 인접한 블록 존재시 결합
        offset = blkOffset ^ 0x1;

        flag = kGetFlagInBitmap(i, offset);

        // 블록이 Empty면 종료
        if(flag == BUDDY_MEMORY_EMPTY)
            break;
        
        // 인접한 블록 존재
        // 블록 결합 후 상위 블록으로 이동, 기존 블록은 모두 빈것으로 만듦
        kSetFlagInBitmap(i, offset, BUDDY_MEMORY_EMPTY);
        kSetFlagInBitmap(i, blkOffset, BUDDY_MEMORY_EMPTY);

        // 상위 블록 리스트의 블록 Offset 변경 후 상위 과정 반복
        blkOffset /= 2;
    }

    kUnlockForSystemData(isPrevInterruptFlag);

    return TRUE;
}

static BYTE kGetFlagInBitmap(int blockListIdx, int offset) {
    BYTE *pBitmap;

    pBitmap = gBuddyMemory.pLevelBitmap[blockListIdx].pBitmap;
    if( (pBitmap[offset / 8] & (0x01 << (offset % 8))) != 0x0)
        return BUDDY_MEMORY_EXIST;
    
    return BUDDY_MEMORY_EMPTY;
}

void kGetBuddyMemoryInfo(QWORD *pBuddyMemoryStartAddr, QWORD *pBuddyMemoryTotalSize, 
                        QWORD *pMetaDataSize, QWORD *pUsedMemorySize) {
    *pBuddyMemoryStartAddr = BUDDY_MEMORY_START_ADDR;
    *pBuddyMemoryTotalSize = kCalcBuddyMemorySize();
    *pMetaDataSize = kCalcMetaBlockCount(*pBuddyMemoryTotalSize) * MIN_BUDDY_MEMORY_SIZE;
    *pUsedMemorySize = gBuddyMemory.usedSize;
}

BUDDY_MEMORY *kGetBuddyMemoryManager(void) {
    return &gBuddyMemory;
}