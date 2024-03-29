#ifndef __FS_H__
#define __FS_H__

#include "Types.h"
#include "Task.h"
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
// Maximum count of Handle
#define FS_HANDLE_MAX_COUNT         (TASK_MAX_COUNT * 3)

// Maximum length of File Name
#define FS_MAX_FILENAME_LENGTH      (24)

// Handle Type
#define FS_TYPE_FREE                (0)
#define FS_TYPE_FILE                (1)
#define FS_TYPE_DIR                 (2)

// Seek Option Flag
#define FS_SEEK_SET                 (0)
#define FS_SEEK_CUR                 (1)
#define FS_SEEK_END                 (2)

typedef BOOL (*fReadHDDInfo)(BOOL isPrimary, BOOL isMaster, HDD_INFO *pHDDInfo);
typedef BOOL (*fReadHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf);
typedef BOOL (*fWriteHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf);

// C Standard API -> BBI OS Internal FS API
#define fopen       kOpenFile
#define fread       kReadFile
#define fwrite      kWriteFile
#define fseek       kSeekFile
#define fclose      kCloseFile
#define remove      kRemoveFile

#define opendir     kOpenDir
#define readdir     kReadDir
#define rewinddir   kRewindDir
#define closedir    kCloseDir

// C Standard Type -> BBI OS FS Type
#define SEEK_SET    FS_SEEK_SET
#define SEEK_CUR    FS_SEEK_CUR
#define SEEK_END    FS_SEEK_END

#define size_t      DWORD
#define dirent      DIR_ENTRY
#define d_name      filename

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

// File handle
typedef struct kFileHandleStruct {
    int dirEntryOffset;
    DWORD filesize;
    DWORD startClusterIdx;
    DWORD curClusterIdx;
    DWORD prevClusterIdx;
    DWORD curOffset;
} FILE_HANDLE;

// Directory Handle
typedef struct kDirectoryHandleStruct {
    DIR_ENTRY *pDirBuf;
    int curOffset;
} DIR_HANDLE;

// File, Directory Information Struct
typedef struct kFileDirectoryHandleStruct {
    BYTE type;

    union {
        FILE_HANDLE fileHandle;
        DIR_HANDLE  dirHandle;
    };
} FILE, DIR;

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

    // Handle pool address
    FILE *pHandlePool
} FS_MANAGER;

#pragma pack(pop);

// Low Level Functions
BOOL kInitializeFS(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInfo(HDD_INFO *pInfo);

static BOOL kReadClusterLinkTable(DWORD offset, BYTE *pBuf);
static BOOL kWriteClusterLinkTable(DWORD offset, BYTE *pBuf);
static BOOL kReadCluster(DWORD offset, BYTE *pBuf);
static BOOL kWriteCluster(DWORD offset, BYTE *pBuf);
static DWORD kFindFreeCluster(void);
static BOOL kSetClusterLinkData(DWORD clusterIdx, DWORD data);
static BOOL kGetClusterLinkData(DWORD clusterIdx, DWORD *pData);
static int kFindFreeDirEntry(void);
static BOOL kSetDirEntryData(int idx, DIR_ENTRY *pEntry);
static BOOL kGetDirEntryData(int idx, DIR_ENTRY *pEntry);
static int kFindDirEntry(const char *pFilename, DIR_ENTRY *pEntry);
void kGetFSInfo(FS_MANAGER *pManager);

// High Level Functions
FILE* kOpenFile(const char *filename, const char *pMode);
DWORD kReadFile(void *pBuf, DWORD size, DWORD cnt, FILE *pFile);
DWORD kWriteFile(const void *pBuf, DWORD size, DWORD cnt, FILE *pFile);
int kSeekFile(FILE *pFile, int offset, int origin);
int kCloseFile(FILE *pFile);
int kRemoveFile(const char *filename);

DIR *kOpenDir(const char *dirname);
DIR_ENTRY *kReadDir(DIR *pDir);
void *kRewindDir(DIR *pDir);
int kCloseDir(DIR *pDir);

BOOL kWriteZero(FILE *pFile, DWORD cnt);
DIR *kIsFileOpened(const DIR_ENTRY *pEntry);

static void *kAllocFileDirHandle(void);
static void kFreeFileDirectoryHandle(FILE *pFile);
static BOOL kCreateFile(const char *filename, DIR_ENTRY *pEntry, int *pDirEntryIdx);
static BOOL kFreeClusterUntilEnd(DWORD clusterIdx);
static BOOL kUpdateDirEntry(FILE_HANDLE *pFileHandle);

#endif  // __FS_H__