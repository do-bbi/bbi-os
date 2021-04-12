#include "FS.h"
#include "HDD.h"
#include "BuddyMemory.h"
#include "Task.h"
#include "Utility.h"

static FS_MANAGER gFSManager;
static BYTE gTmpBuf[FS_CLUTSER_SIZE];

fReadHDDInfo    pFReadHDDInfo = NULL;
fReadHDDSector  pFReadHDDSector = NULL;
fWriteHDDSector pFWriteHDDSector = NULL;

BOOL kInitializeFS(void) {
    kMemSet(&gFSManager, 0, sizeof(gFSManager));
    kInitializeMutex(&gFSManager.mutex);

    // Init HDD
    if(kInitializeHDD() == FALSE)
        return FALSE;

    pFReadHDDInfo       = kReadHDDInfo;
    pFReadHDDSector     = kReadHDDSector;
    pFWriteHDDSector    = kWriteHDDSector;

    if(kMount() == FALSE)
        return FALSE;

    // Allocate memory for handler
    gFSManager.pHandlePool = (FILE *)kAllocateMemory(FS_HANDLE_MAX_COUNT * sizeof(FILE));

    // 메모리 할당 실패 시 하드디스크 인식 실패로 설정
    if(gFSManager.pHandlePool == NULL) {
        gFSManager.isMouted = FALSE;
        return FALSE;
    }

    kMemSet(gFSManager.pHandlePool, 0, FS_HANDLE_MAX_COUNT * sizeof(FILE));

    return TRUE;
}

BOOL kFormat(void) {
    HDD_INFO *pHDDInfo;
    MBR *pMBR;
    DWORD totalSectorCnt, remainSectorCnt;
    DWORD maxClusterCnt, clusterCnt;
    DWORD clusterLinkSectorCnt;
    DWORD i;

    kLock(&gFSManager.mutex);

    pHDDInfo = (HDD_INFO *)gTmpBuf;
    if(pFReadHDDInfo(TRUE, TRUE, pHDDInfo) == FALSE) {
        kUnlock(&gFSManager.mutex);
        return FALSE;
    }

    totalSectorCnt = pHDDInfo->totalSectors;
    maxClusterCnt = totalSectorCnt / FS_SECTORS_PER_CLUSTER;
    
    // sizeof(LinkData) = 4 Bytes
    // Max #LinkData in a sector
    // = FS_SECTOR_SIZE / sizeof(LinkData) 
    // = 512 / 4 = 128
    clusterLinkSectorCnt = (maxClusterCnt + 127) / 128;

    remainSectorCnt = totalSectorCnt - clusterLinkSectorCnt - 1;
    clusterCnt = remainSectorCnt / FS_SECTORS_PER_CLUSTER;

    // Read MBR
    if(pFReadHDDSector(TRUE, TRUE, 0, 1, gTmpBuf) == FALSE) {
        kUnlock(&gFSManager.mutex);
        return FALSE;
    }

    pMBR = (MBR *)gTmpBuf;
    kMemSet(pMBR->partionTable, 0, sizeof(pMBR->partionTable));

    pMBR->fsSignature = FS_SIGNATURE;
    pMBR->reservedSectorCnt = 0;
    pMBR->clusterLinkSectorCnt = clusterLinkSectorCnt;
    pMBR->totalClusterCnt = clusterCnt;

    if(pFWriteHDDSector(TRUE, TRUE, 0, 1, gTmpBuf) == FALSE) {
        kUnlock(&gFSManager.mutex);
        return FALSE;
    }

    // Init after MBR area until Root Directory
    kMemSet(gTmpBuf, 0, FS_SECTOR_SIZE);
    for(i = 0; i < clusterLinkSectorCnt + FS_SECTORS_PER_CLUSTER; ++i) {
        ((DWORD *)(gTmpBuf))[0] = i ? FS_FREE_CLUSTER : FS_LAST_CLUSTER;
        
        // Write sector by sector
        if(pFWriteHDDSector(TRUE, TRUE, i + 1, 1, gTmpBuf) == FALSE) {
            kUnlock(&gFSManager.mutex);
            return FALSE;
        }
    }

    kUnlock(&gFSManager.mutex);
    return TRUE;
}

BOOL kMount(void) {
    MBR *pMBR;

    kLock(&gFSManager.mutex);

    // Read MBR
    if(pFReadHDDSector(TRUE, TRUE, 0, 1, gTmpBuf) == FALSE) {
        kUnlock(&gFSManager.mutex);
        return FALSE;
    }

    pMBR = (MBR *)gTmpBuf;
    if(pMBR->fsSignature != FS_SIGNATURE) {
        kUnlock(&gFSManager.mutex);
        return FALSE;
    }

    // Mount success
    gFSManager.isMouted = TRUE;

    // Calculate start LBA address, #sectors
    gFSManager.reservedSectorCnt = pMBR->reservedSectorCnt;
    gFSManager.clusterLinkAreaStartAddr = pMBR->reservedSectorCnt + 1;
    gFSManager.clusterLinkAreaSize = pMBR->clusterLinkSectorCnt;
    gFSManager.dataAreaStartAddr = pMBR->reservedSectorCnt + pMBR->clusterLinkSectorCnt + 1;
    gFSManager.totalClusterCnt = pMBR->totalClusterCnt;
    
    kUnlock(&gFSManager.mutex);

    return TRUE;
}

BOOL kGetHDDInfo(HDD_INFO *pInfo) {
    BOOL result;

    kLock(&gFSManager.mutex);

    result = pFReadHDDInfo(TRUE, TRUE, pInfo);

    kUnlock(&gFSManager.mutex);

    return result;
}

static BOOL kReadClusterLinkTable(DWORD offset, BYTE *pBuf) {
    return pFReadHDDSector(TRUE, TRUE, 
            offset + gFSManager.clusterLinkAreaStartAddr, 1, pBuf);
}

static BOOL kWriteClusterLinkTable(DWORD offset, BYTE *pBuf) {
    return pFWriteHDDSector(TRUE, TRUE, 
            offset + gFSManager.clusterLinkAreaStartAddr, 1, pBuf);
}
static BOOL kReadCluster(DWORD offset, BYTE *pBuf) {
    return pFReadHDDSector(TRUE, TRUE, 
            (offset * FS_SECTORS_PER_CLUSTER) + gFSManager.dataAreaStartAddr, 
            FS_SECTORS_PER_CLUSTER, pBuf) == FS_SECTORS_PER_CLUSTER;
}

static BOOL kWriteCluster(DWORD offset, BYTE *pBuf) {
    return pFWriteHDDSector(TRUE, TRUE, 
            (offset * FS_SECTORS_PER_CLUSTER) + gFSManager.dataAreaStartAddr, 
            FS_SECTORS_PER_CLUSTER, pBuf) == FS_SECTORS_PER_CLUSTER;
}

static DWORD kFindFreeCluster(void) {
    DWORD linkCntInSector;
    DWORD lastSectorOffset, curSectorOffset;
    DWORD i, j;

    if(gFSManager.isMouted == FALSE)
        return FS_LAST_CLUSTER;
    
    lastSectorOffset = gFSManager.lastAllocClusterLinkSectorOffset;

    // Start from Offset of Last Allocated Sector
    // Find free cluster
    for(i = 0; i < gFSManager.clusterLinkAreaSize; ++i) {
        if(lastSectorOffset + i == gFSManager.clusterLinkAreaSize - 1)
            linkCntInSector = gFSManager.totalClusterCnt % 128;
        else
            linkCntInSector = 128;
        
        curSectorOffset = (lastSectorOffset + i) % gFSManager.clusterLinkAreaSize;
    
        if(kReadClusterLinkTable(curSectorOffset, gTmpBuf) == FALSE)
            return FS_LAST_CLUSTER;
        
        for(j = 0; j < linkCntInSector; ++j) {
            if(((DWORD *)gTmpBuf)[j] == FS_FREE_CLUSTER)
                break;
        }

        // Free Cluster is found 
        // -> Return free cluster index
        if(j != linkCntInSector) {
            gFSManager.lastAllocClusterLinkSectorOffset = curSectorOffset;

            return (curSectorOffset * 128) + j;
        }
    }

    return FS_LAST_CLUSTER;
}

static BOOL kSetClusterLinkData(DWORD clusterIdx, DWORD data) {
    DWORD sectorOffset;

    if(gFSManager.isMouted == FALSE)
        return FALSE;
    
    // 128 Clusters in 1 Sector, Cluster Index / 128 = Sector Offset
    sectorOffset = clusterIdx / 128;

    // 해당 섹터를 읽어, 링크 정보 설정 후 다시 저장
    if(kReadClusterLinkTable(sectorOffset, gTmpBuf) == FALSE) {
        kPrintf("kReadClusterLinkTable failed...\n");
        return FALSE;
    }
    
    ((DWORD *)gTmpBuf)[clusterIdx % 128] = data;

    if(kWriteClusterLinkTable(sectorOffset, gTmpBuf) == FALSE) {
        kPrintf("kWriteClusterLinkTable failed...\n");
        return FALSE;
    }
    
    return TRUE;
}
static BOOL kGetClusterLinkData(DWORD clusterIdx, DWORD *pData) {
    DWORD sectorOffset;

    if(gFSManager.isMouted == FALSE)
        return FALSE;
    
    // 128 Clusters in 1 Sector, Cluster Index / 128 = Sector Offset
    sectorOffset = clusterIdx / 128;

    if(sectorOffset > gFSManager.clusterLinkAreaSize)
        return FALSE;

    // 해당 섹터를 읽어, 링크 정보 확인
    if(kReadClusterLinkTable(sectorOffset, gTmpBuf) == FALSE)
        return FALSE;
    
    *pData = ((DWORD *)gTmpBuf)[clusterIdx % 128];
    
    return TRUE;
}

static int kFindFreeDirEntry(void) {
    DIR_ENTRY *pEntry;
    int i;

    if(gFSManager.isMouted == FALSE)
        return -1;
    
    if(kReadCluster(0, gTmpBuf) == FALSE)
        return -1;
    
    pEntry = (DIR_ENTRY *)gTmpBuf;
    for(i = 0; i < FS_MAX_DIR_ENTRY_COUNT; ++i) {
        if(pEntry[i].startClusterIdx == 0)
            return i;
    }

    return -1;
}

static BOOL kSetDirEntryData(int idx, DIR_ENTRY *pEntry) {
    DIR_ENTRY *pRootEntry;

    if(gFSManager.isMouted == FALSE || idx < 0 || FS_MAX_DIR_ENTRY_COUNT <= idx)
        return FALSE;
    
    // Read Root Directory
    if(kReadCluster(0, gTmpBuf) == FALSE)
        return FALSE;

    // Update Directory Entry
    pRootEntry = (DIR_ENTRY *)gTmpBuf;
    kMemCpy(pRootEntry + idx, pEntry, sizeof(DIR_ENTRY));

    // Write Root Directory
    if(kWriteCluster(0, gTmpBuf) == FALSE) {
        kPrintf("kWriteCluster failed...\n");
        return FALSE;
    }
    
    return TRUE;
}

static BOOL kGetDirEntryData(int idx, DIR_ENTRY *pEntry) {
    DIR_ENTRY *pRootEntry;

    if(gFSManager.isMouted == FALSE || idx < 0 || FS_MAX_DIR_ENTRY_COUNT <= idx)
        return FALSE;
    
    // Read Root Directory
    if(kReadCluster(0, gTmpBuf) == FALSE)
        return FALSE;

    // Update Directory Entry
    pRootEntry = (DIR_ENTRY *)gTmpBuf;
    kMemCpy(pEntry, pRootEntry + idx, sizeof(DIR_ENTRY));
    return TRUE;
}

static int kFindDirEntry(const char *pFilename, DIR_ENTRY *pEntry) {
    DIR_ENTRY *pRootEntry;
    int i, len;

    if(gFSManager.isMouted == FALSE)
        return -1;
    
    // Read Root Directory
    if(kReadCluster(0, gTmpBuf) == FALSE)
        return FALSE;

    len = kStrLen(pFilename);

    pRootEntry = (DIR_ENTRY *)gTmpBuf;
    for(i = 0; i < FS_MAX_DIR_ENTRY_COUNT; ++i) {
        if(kMemCmp(pRootEntry[i].filename, pFilename, len) == 0) {
            kMemCpy(pEntry, pRootEntry + i, sizeof(DIR_ENTRY));
            return i;
        }
    }

    return -1;

    // Write Root Directory
    if(kWriteCluster(0, gTmpBuf) == FALSE) {
        kPrintf("kWriteCluster failed...\n");
        return FALSE;
    }
    
    return TRUE;
}

void kGetFSInfo(FS_MANAGER *pManager) {
    kMemCpy(pManager, &gFSManager, sizeof(gFSManager));
}

FILE* kOpenFile(const char *filename, const char *pMode) {
    DIR_ENTRY entry;
    int len, dirEntryOffset;
    DWORD secondCluster;

    FILE *pFile;

    len = kStrLen(filename);
    if(len == 0 || sizeof(entry.filename) <= len)
        return NULL;

    kLock(&gFSManager.mutex);

    dirEntryOffset = kFindDirEntry(filename, &entry);
    if(dirEntryOffset == -1) {
        // Read failed, cause file isn't exist
        if(pMode[0] == 'r') {
            kUnlock(&gFSManager.mutex);
            return NULL;
        }

        if(kCreateFile(filename, &entry, &dirEntryOffset) == FALSE) {
            kUnlock(&gFSManager.mutex);
            return NULL;
        }
    }
    else if(pMode[0] == 'w') {
       if(kGetClusterLinkData(entry.startClusterIdx, &secondCluster) == FALSE) {
            kUnlock(&gFSManager.mutex);
            return NULL;
       }

       if(kSetClusterLinkData(entry.startClusterIdx, FS_LAST_CLUSTER) == FALSE) {
           kUnlock(&gFSManager.mutex);
           return NULL;
       }

       if(kFreeClusterUntilEnd(secondCluster) == FALSE) {
           kUnlock(&gFSManager.mutex);
           return NULL;
       }

       entry.filesize = 0;
       if(kSetDirEntryData(dirEntryOffset, &entry) == FALSE) {
           kUnlock(&gFSManager.mutex);
           return NULL;
       }
    }

    pFile = kAllocFileDirHandle();
    if(pFile == NULL) {
        kUnlock(&gFSManager.mutex);
        return NULL;
    }

    pFile->type                         = FS_TYPE_FILE;
    pFile->fileHandle.dirEntryOffset    = dirEntryOffset;
    pFile->fileHandle.filesize          = entry.filesize;
    pFile->fileHandle.startClusterIdx   = entry.startClusterIdx;
    pFile->fileHandle.curClusterIdx     = entry.startClusterIdx;
    pFile->fileHandle.prevClusterIdx    = entry.startClusterIdx;
    pFile->fileHandle.curOffset         = 0;

    // 추가 옵션('a')으로 열은 경우, 파일 끝으로 이동
    if(pMode[0] == 'a')
        kSeekFile(pFile, 0, FS_SEEK_END);
    
    kUnlock(&gFSManager.mutex);
    return pFile;
}

DWORD kReadFile(void *pBuf, DWORD size, DWORD cnt, FILE *pFile) {
    DWORD totalCnt;
    DWORD readCnt;
    DWORD offsetInCluster;
    DWORD copySize;
    FILE_HANDLE *pFileHandle;
    DWORD nextClusterIdx;

    if(pFile == NULL || pFile->type != FS_TYPE_FILE)
        return 0;
    pFileHandle = &(pFile->fileHandle);

    // if offset if end of file OR last cluster, then finish
    if(pFileHandle->curOffset == pFileHandle->filesize || pFileHandle->curClusterIdx == FS_LAST_CLUSTER)
        return 0;
    
    totalCnt = MIN(size * cnt, pFileHandle->filesize - pFileHandle->curOffset);

    kLock(&gFSManager.mutex);

    readCnt = 0;
    while(readCnt < totalCnt) {
        if(kReadCluster(pFileHandle->curClusterIdx, gTmpBuf) == FALSE)
            break;
        
        offsetInCluster = pFileHandle->curOffset % FS_CLUTSER_SIZE;

        copySize = MIN(FS_CLUTSER_SIZE - offsetInCluster, totalCnt - readCnt);
        kMemCpy((char *)pBuf + readCnt, gTmpBuf + offsetInCluster, copySize);

        readCnt += copySize;
        pFileHandle->curOffset += copySize;
        
        // Move to next cluster, if reading curret cluster is finished
        if(pFileHandle->curOffset % FS_CLUTSER_SIZE == 0) {
            if(kGetClusterLinkData(pFileHandle->curClusterIdx, &nextClusterIdx) == FALSE) {
                kPrintf("kGetClusterLinkData(%d, 0x%p) FALSE\n", pFileHandle->curClusterIdx, &nextClusterIdx);
                break;
            }
            
            pFileHandle->prevClusterIdx     = pFileHandle->curClusterIdx;
            pFileHandle->curClusterIdx      = nextClusterIdx;
        }
    }

    kUnlock(&gFSManager.mutex);

    return readCnt;
}

DWORD kWriteFile(const void *pBuf, DWORD size, DWORD cnt, FILE *pFile) {
    DWORD writeCnt;
    DWORD totalCnt;
    DWORD offsetInCluster;
    DWORD copySize;
    DWORD allocClusterIdx;
    DWORD nextClusterIdx;

    FILE_HANDLE *pFileHandle;

    if(pFile == NULL || pFile->type != FS_TYPE_FILE)
        return 0;
    
    pFileHandle = &(pFile->fileHandle);

    totalCnt = size * cnt;

    kLock(&gFSManager.mutex);

    writeCnt = 0;
    while(writeCnt < totalCnt) {
        if(pFileHandle->curClusterIdx == FS_LAST_CLUSTER) {
            // Find free cluster
            allocClusterIdx = kFindFreeCluster();
            if(allocClusterIdx == FS_LAST_CLUSTER)
                break;
            
            // Set the found cluster to last cluster
            if(kSetClusterLinkData(allocClusterIdx, FS_LAST_CLUSTER) == FALSE)
                break;
            
            if(kSetClusterLinkData(pFileHandle->prevClusterIdx, allocClusterIdx) == FALSE) {
                // Return the found cluster, if failed
                kSetClusterLinkData(allocClusterIdx, FS_FREE_CLUSTER);
                break;
            }

            // Update current cluster to the found cluster
            pFileHandle->curClusterIdx = allocClusterIdx;

            // ???
            kMemSet(gTmpBuf, 0, FS_CLUTSER_SIZE);
        }
        else if((pFileHandle->curOffset % FS_CLUTSER_SIZE) != 0 ||
                totalCnt - writeCnt < FS_CLUTSER_SIZE) {
            if(kReadCluster(pFileHandle->curClusterIdx, gTmpBuf) == FALSE)
                break;
        }

        offsetInCluster = pFileHandle->curOffset % FS_CLUTSER_SIZE;

        copySize = MIN(FS_CLUTSER_SIZE - offsetInCluster, totalCnt - writeCnt);

        kMemCpy(gTmpBuf + offsetInCluster, (char *)pBuf + writeCnt, copySize);

        if(kWriteCluster(pFileHandle->curClusterIdx, gTmpBuf) == FALSE)
            break;
        
        writeCnt += copySize;
        pFileHandle->curOffset += copySize;

        if(pFileHandle->curOffset % FS_CLUTSER_SIZE == 0)  {
            if(kGetClusterLinkData(pFileHandle->curClusterIdx, &nextClusterIdx) == FALSE)
                break;
            
            pFileHandle->prevClusterIdx = pFileHandle->curClusterIdx;
            pFileHandle->curClusterIdx = nextClusterIdx;
        }
    }

    if(pFileHandle->filesize < pFileHandle->curOffset) {
        pFileHandle->filesize = pFileHandle->curOffset;
        kUpdateDirEntry(pFileHandle);
    }

    kUnlock(&gFSManager.mutex);

    return writeCnt;
}

int kSeekFile(FILE *pFile, int offset, int origin) {
    DWORD dstOffset;
    DWORD curClusterOffset;
    DWORD lastClusterOffset;
    DWORD dstClusterOffset;
    DWORD moveCnt;

    DWORD i;
    DWORD startClusterIdx;
    DWORD prevClusterIdx;
    DWORD curClusterIdx;

    FILE_HANDLE *pFileHandle;

    if(pFile == NULL || pFile->type != FS_TYPE_FILE)
        return 0;
    
    pFileHandle = &(pFile->fileHandle);

    // Calculate destination offset by using origin + offset
    switch (origin)
    {
    case FS_SEEK_SET:
        dstOffset = offset < 0 ? 0 : offset;
        break;
    
    case FS_SEEK_CUR:
        if(offset < 0 && pFileHandle->curOffset + offset < 0)
            dstOffset = 0;
        else
            dstOffset = pFileHandle->curOffset + offset;
        break;
    
    case FS_SEEK_END:
        if(offset < 0 && pFileHandle->filesize + offset < 0)
            dstOffset = 0;
        else
            dstOffset = pFileHandle->filesize + offset;
        break;
    }

    lastClusterOffset   = pFileHandle->filesize / FS_CLUTSER_SIZE;
    dstClusterOffset    = dstOffset / FS_CLUTSER_SIZE;
    curClusterOffset    = pFileHandle->curOffset / FS_CLUTSER_SIZE;

    // Calculate moveCnt, startClusterIdx 
    if(lastClusterOffset < dstClusterOffset) {
        moveCnt = lastClusterOffset - curClusterOffset;
        startClusterIdx = pFileHandle->curClusterIdx;
    }
    else if(curClusterOffset <= dstClusterOffset) {
        moveCnt = dstClusterOffset - curClusterOffset;
        startClusterIdx = pFileHandle->curClusterIdx;
    }
    else {
        moveCnt = dstClusterOffset;
        startClusterIdx = pFileHandle->startClusterIdx;
    }

    kLock(&gFSManager.mutex);

    curClusterIdx = startClusterIdx;
    for(i = 0; i < moveCnt; ++i) {
        prevClusterIdx = curClusterIdx;

        if(kGetClusterLinkData(prevClusterIdx, &curClusterIdx) == FALSE) {
            kUnlock(&gFSManager.mutex);
            return -1;
        }
    }
    
    pFileHandle->curOffset = dstOffset;

    kUnlock(&gFSManager.mutex);

    return 0;
}

int kCloseFile(FILE *pFile) {
    if(pFile == NULL || pFile->type != FS_TYPE_FILE)
        return -1;
    
    kFreeFileDirectoryHandle(pFile);

    return 0;
}

int kRemoveFile(const char *filename) {
    DIR_ENTRY entry;
    int dirEntryOffset;
    int len;

    len = kStrLen(filename);
    if(len == 0 || sizeof(entry.filename) <= len) {
        kPrintf("Filename is too long or short, Failed\n");
        return -1;   
    }

    kLock(&gFSManager.mutex);

    // Check file is exist
    dirEntryOffset = kFindDirEntry(filename, &entry);
    if(dirEntryOffset == -1) {
        kPrintf("Can not find directory entry, Failed\n");
        kUnlock(&gFSManager.mutex);
        return -1;
    }
    
    if(kIsFileOpened(&entry)) {
        kPrintf("File is opend... Failed\n");
        kUnlock(&gFSManager.mutex);
        return -1;
    }

    if(kFreeClusterUntilEnd(entry.startClusterIdx) == FALSE) {
        kPrintf("Can not free all clusters... Failed\n");
        kUnlock(&gFSManager.mutex);
        return -1;
    }

    // Set directory entry as free
    kMemSet(&entry, 0, sizeof(entry));
    if(kSetDirEntryData(dirEntryOffset, &entry) == FALSE) {
        kPrintf("Can not set directory entry data as 0... Failed\n");
        kUnlock(&gFSManager.mutex);
        return -1;
    }

    kUnlock(&gFSManager.mutex);

    return 0;
}

DIR *kOpenDir(const char *dirname) {
    DIR *pDir;
    DIR_ENTRY *pDirBuf;

    kLock(&gFSManager.mutex);
    
    pDir = kAllocFileDirHandle();
    if(pDir == NULL) {
        kUnlock(&gFSManager.mutex);
        return NULL;
    }

    // 루트 디렉토리 밖에 존재 하지 않으므로, 이름을 무시하고 핸들만 받아 반환
    pDirBuf = (DIR_ENTRY *)kAllocateMemory(FS_CLUTSER_SIZE);
    if(pDirBuf == NULL) {
        kFreeFileDirectoryHandle(pDir);
        kUnlock(&gFSManager.mutex);
        return NULL;
    }

    if(kReadCluster(0, (BYTE *)pDirBuf) == FALSE) {
        // Return handles, memory, if failed
        kFreeFileDirectoryHandle(pDir);
        kFreeMemory(pDirBuf);

        kUnlock(&gFSManager.mutex);
        return NULL;
    }

    pDir->type                  = FS_TYPE_DIR;
    pDir->dirHandle.curOffset   = 0;
    pDir->dirHandle.pDirBuf     = pDirBuf;

    kUnlock(&gFSManager.mutex);

    return pDir;
}

DIR_ENTRY *kReadDir(DIR *pDir) {
    DIR_HANDLE *pDirHandle;
    DIR_ENTRY *pEntry;
    
    if(pDir == NULL || pDir->type != FS_TYPE_DIR)
        return NULL;
    
    pDirHandle = &(pDir->dirHandle);

    if(pDirHandle->curOffset < 0 || FS_MAX_DIR_ENTRY_COUNT <= pDirHandle->curOffset)
        return NULL;

    kLock(&gFSManager.mutex);

    pEntry = pDirHandle->pDirBuf;
    while(pDirHandle->curOffset < FS_MAX_DIR_ENTRY_COUNT) {
        // 파일을 찾았다면 반환
        if(pEntry[pDirHandle->curOffset].startClusterIdx != 0) {
            kUnlock(&gFSManager.mutex);
            return &(pEntry[pDirHandle->curOffset++]);
        }

        pDirHandle->curOffset++;
    }

    kUnlock(&gFSManager.mutex);
    return NULL;
}

void *kRewindDir(DIR *pDir) {
    DIR_HANDLE *pDirHandle;

    if(pDir == NULL || pDir->type != FS_TYPE_DIR)
        return;
    pDirHandle = &(pDir->dirHandle);

    kLock(&gFSManager.mutex);

    pDirHandle->curOffset = 0;

    kUnlock(&gFSManager.mutex);
}

int kCloseDir(DIR *pDir) {
    DIR_HANDLE *pDirHandle;

    if(pDir == NULL || pDir->type != FS_TYPE_DIR)
        return -1;
    pDirHandle = &(pDir->dirHandle);
    
    kLock(&gFSManager.mutex);

    kFreeMemory(pDirHandle->pDirBuf);
    kFreeFileDirectoryHandle(pDir);

    kUnlock(&gFSManager.mutex);

    return 0;
}

DIR *kWriteZero(FILE *pFile, DWORD cnt) {
    BYTE *pBuf;
    DWORD remainCnt;
    DWORD writeCnt;

    if(pFile == NULL)
        return FALSE;
    
    pBuf = (BYTE *)kAllocateMemory(FS_CLUTSER_SIZE);
    if(pBuf == NULL)
        return FALSE;
    
    kMemSet(pBuf, 0, FS_CLUTSER_SIZE);
    remainCnt = cnt;

    while(0 < remainCnt) {
        writeCnt = MIN(remainCnt , FS_CLUTSER_SIZE);
        if(kWriteFile(pBuf, 1, writeCnt, pFile) != writeCnt) {
            kFreeMemory(pBuf);
            return FALSE;
        }
        remainCnt -= writeCnt;
    }

    kFreeMemory(pBuf);
    return TRUE;
}

DIR *kIsFileOpened(const DIR_ENTRY *pEntry) {
    int i;
    FILE *pFile;

    pFile = gFSManager.pHandlePool;
    for(i = 0; i < FS_HANDLE_MAX_COUNT; ++i) {
        if(pFile[i].type == FS_TYPE_FILE && 
            pFile[i].fileHandle.startClusterIdx == pEntry->startClusterIdx)
            return TRUE;
    }

    return FALSE;
}

static void *kAllocFileDirHandle(void) {
    int i;
    FILE *pFile;

    // Find free handle

    pFile = gFSManager.pHandlePool;
    for(i = 0; i < FS_HANDLE_MAX_COUNT; ++i) {
        // Return if handle is free
        if(pFile->type == FS_TYPE_FREE) {
            pFile->type = FS_TYPE_FREE;
            return pFile;
        }

        // goto next;
        pFile++;
    }

    return NULL;
}

static void kFreeFileDirectoryHandle(FILE *pFile) {
    kMemSet(pFile, 0, sizeof(FILE));

    pFile->type = FS_TYPE_FREE;
}

static BOOL kCreateFile(const char *filename, DIR_ENTRY *pEntry, int *pDirEntryIdx) {
    DWORD cluster;

    cluster = kFindFreeCluster();
    if(cluster == FS_LAST_CLUSTER || kSetClusterLinkData(cluster, FS_LAST_CLUSTER) == FALSE)
        return FALSE;
    
    *pDirEntryIdx = kFindFreeDirEntry();
    if(*pDirEntryIdx == -1) {
        kSetClusterLinkData(cluster, FS_FREE_CLUSTER);
        return FALSE;
    }

    // Set Directory Entry
    kMemCpy(pEntry->filename, filename, kStrLen(filename) + 1);
    pEntry->startClusterIdx = cluster;
    pEntry->filesize = 0;

    // Regist Directory Entry
    if(kSetDirEntryData(*pDirEntryIdx, pEntry) == FALSE) {
        // Return cluster if failed
        kSetClusterLinkData(cluster, FS_FREE_CLUSTER);
        return FALSE;
    }

    return TRUE;
}

static BOOL kFreeClusterUntilEnd(DWORD clusterIdx) {
    DWORD curClusterIdx;
    DWORD nextClusterIdx;

    curClusterIdx = clusterIdx;
    while(curClusterIdx != FS_LAST_CLUSTER) {
        // Get next cluster index
        if(kGetClusterLinkData(curClusterIdx, &nextClusterIdx) == FALSE)
            return FALSE;
        
        // Set current cluster as free
        if(kSetClusterLinkData(curClusterIdx, FS_FREE_CLUSTER) == FALSE)
            return FALSE;

        curClusterIdx = nextClusterIdx;
    }

    return TRUE;
}

static BOOL kUpdateDirEntry(FILE_HANDLE *pFileHandle) {
    DIR_ENTRY entry;

    if(pFileHandle == NULL || kGetDirEntryData(pFileHandle->dirEntryOffset, &entry) == FALSE)
        return FALSE;
    
    entry.filesize = pFileHandle->filesize;
    entry.startClusterIdx = pFileHandle->startClusterIdx;

    if(kSetDirEntryData(pFileHandle->dirEntryOffset, &entry) == FALSE)
        return FALSE;
    
    return TRUE;
}