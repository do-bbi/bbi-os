#ifndef __FS_H__
#define __FS_H__

#include "Types.h"
#include "Sync.h"
#include "HDD.h"

// Macro, Function

// BBI FS Signature
#define FS_SIGNATURE                (0xBBF5C0DE)
// #Sectors per cluster
#define FS_SECTORS_PER_CLUSTER      (8)
// Mark for last cluster
#define FS_LAST_CLUSTER             (0xFFFFFFFF)
// Makr for free cluster
#define FS_FREE_CLUSTER             (0x00)
// Sizeof Sector
#define FS_SECTOR_SIZE              (512)
// Sizeof Cluster
#define FS_CLUTSER_SIZE             (FS_SECTORS_PER_CLUSTER * FS_SECTOR_SIZE)
// Maximum count of directories
#define FS_MAX_DIR_ENTRY_COUNT      (FS_CLUTSER_SIZE / sizeof(DIR_ENTRY))

// Maximum length of File Name
#define FS_MAX_FILENAME_LENGTH      (24)

typedef BOOL (*fReadHDDInfo)(BOOL isPrimary, BOOL isMaster, HDD_INFO *pHDDInfo);
typedef BOOL (*fReadHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf);
typedef BOOL (*fWriteHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf);

// Struct
#pragma pack(push, 1)

// Partition
typedef struct kPartitionStruct {
    // Bootable Flag, 0x80 -> Possible, 0x00 -> Impossible
    BYTE bootFlag;
    // Start partiton Address in CHS mode
    BYTE startCHSAddr[3];
    // Partition Type
    BYTE partitionType;
    // End partiton Address in CHS mode
    BYTE endCHSAddr[3];
    // Start Parition Address in LBA mode
    DWORD startLBAAddr;
    // #Sectors per partition
    DWORD sectorCntInPartition;
} PARTITION;

// MBR
typedef struct kMBRStruct {
    // Bootloader Code
    BYTE bootcode[430];

    // FS Signature, 0xBB05C0DE
    DWORD fsSignature;
    // #Sectors for reserved
    DWORD reservedSectorCnt;
    // #Sectors for Cluster Link Table
    DWORD clusterLinkSectorCnt;
    // Total #Clusters
    DWORD totalClusterCnt;

    // Partition Table
    PARTITION partionTable[4];

    // BootLoader Signature, 0x55, 0xAA
    BYTE blSignature[2];
} MBR;

// Directory Entry
typedef struct kDirectoryEntryStruct {
    char filename[FS_MAX_FILENAME_LENGTH];
    DWORD filesize;
    DWORD startClusterIdx;
} DIR_ENTRY;

#pragma pack(pop);

// FS Manager
typedef struct kFSManagerStruct {
    // is FS mounted
    BOOL isMouted;
    
    // Each Sector's Count, Start LBA Address
    DWORD reservedSectorCnt;
    DWORD clusterLinkAreaStartAddr;
    DWORD clusterLinkAreaSize;
    DWORD dataAreaStartAddr;
    // Total #Cluster for Data Area
    DWORD totalClusterCnt;

    // Sector offset for Cluster link table allocating last cluster
    DWORD lastAllocClusterLinkSectorOffset;

    // FS mutex for sync
    MUTEX mutex;
} FS_MANAGER;

// Functions
BOOL kInitializeFS(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInfo(HDD_INFO *pInfo);

BOOL kReadClusterLinkTable(DWORD offset, BYTE *pBuf);
BOOL kWriteClusterLinkTable(DWORD offset, BYTE *pBuf);
BOOL kReadCluster(DWORD offset, BYTE *pBuf);
BOOL kWriteCluster(DWORD offset, BYTE *pBuf);
DWORD kFindFreeCluster(void);
BOOL kSetClusterLinkData(DWORD clusterIdx, DWORD data);
BOOL kGetClusterLinkData(DWORD clusterIdx, DWORD *pData);
int kFindFreeDirEntry(void);
BOOL kSetDirEntryData(int idx, DIR_ENTRY *pEntry);
BOOL kGetDirEntryData(int idx, DIR_ENTRY *pEntry);
int kFindDirEntry(const char *pFilename, DIR_ENTRY *pEntry);
void kGetFSInfo(FS_MANAGER *pManager);

#endif  // __FS_H__