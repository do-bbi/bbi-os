#include "FS.h"
#include "HDD.h"
#include "BuddyMemory.h"
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

    return kMount();
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

BOOL kReadClusterLinkTable(DWORD offset, BYTE *pBuf) {
    return pFReadHDDSector(TRUE, TRUE, 
            offset + gFSManager.clusterLinkAreaStartAddr, 1, pBuf);
}

BOOL kWriteClusterLinkTable(DWORD offset, BYTE *pBuf) {
    return pFWriteHDDSector(TRUE, TRUE, 
            offset + gFSManager.clusterLinkAreaStartAddr, 1, pBuf);
}
BOOL kReadCluster(DWORD offset, BYTE *pBuf) {
    return pFReadHDDSector(TRUE, TRUE, 
            (offset * FS_SECTORS_PER_CLUSTER) + gFSManager.clusterLinkAreaStartAddr, 
            FS_SECTORS_PER_CLUSTER, pBuf);
}

BOOL kWriteCluster(DWORD offset, BYTE *pBuf) {
    return pFWriteHDDSector(TRUE, TRUE, 
            (offset * FS_SECTORS_PER_CLUSTER) + gFSManager.clusterLinkAreaStartAddr, 
            FS_SECTORS_PER_CLUSTER, pBuf);
}

DWORD kFindFreeCluster(void) {
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

BOOL kSetClusterLinkData(DWORD clusterIdx, DWORD data) {
    DWORD sectorOffset;

    if(gFSManager.isMouted == FALSE)
        return FALSE;
    
    // 128 Clusters in 1 Sector, Cluster Index / 128 = Sector Offset
    sectorOffset = clusterIdx / 128;

    // 해당 섹터를 읽어, 링크 정보 설정 후 다시 저장
    if(kReadClusterLinkTable(sectorOffset, gTmpBuf) == FALSE)
        return FALSE;
    
    ((DWORD *)gTmpBuf)[clusterIdx % 128] = data;

    if(kWriteClusterLinkTable(sectorOffset, gTmpBuf) == FALSE)
        return FALSE;
    
    return TRUE;
}
BOOL kGetClusterLinkData(DWORD clusterIdx, DWORD *pData) {
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

int kFindFreeDirEntry(void) {
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

BOOL kSetDirEntryData(int idx, DIR_ENTRY *pEntry) {
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
    if(kWriteCluster(0, gTmpBuf) == FALSE)
        return FALSE;
    
    return TRUE;
}

BOOL kGetDirEntryData(int idx, DIR_ENTRY *pEntry) {
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

int kFindDirEntry(const char *pFilename, DIR_ENTRY *pEntry) {
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
    if(kWriteCluster(0, gTmpBuf) == FALSE)
        return FALSE;
    
    return TRUE;
}

void kGetFSInfo(FS_MANAGER *pManager) {
    kMemCpy(pManager, &gFSManager, sizeof(gFSManager));
}