#ifndef __HDD_H__
#define __HDD_H__

#include "Types.h"
#include "Sync.h"

// Macro
// Primary & Secondary PATA Port Info
#define HDD_PORT_PRIMARY_BASE       (0x1F0)
#define HDD_PORT_SECONDARY_BASE     (0x170)

// Port Index
#define HDD_PORT_IDX_DATA           (0x00)
#define HDD_PORT_IDX_SECTORCNT      (0x02)
#define HDD_PORT_IDX_SECTORNUM      (0x03)
#define HDD_PORT_IDX_CYLINDERLSB    (0x04)
#define HDD_PORT_IDX_CYLINDERMSB    (0x05)
#define HDD_PORT_IDX_DRIVEANDHEAD   (0x06)
#define HDD_PORT_IDX_STATUS         (0x07)
#define HDD_PORT_IDX_COMMAND        (0x07)
#define HDD_PORT_IDX_DIGITALOUTPUT  (0x206)

// Command Register
#define HDD_COMMAND_READ            (0x20)
#define HDD_COMMAND_WRITE           (0x30)
#define HDD_COMMAND_IDENTIFY        (0xEC)

// Status Register
#define HDD_STATUS_ERROR            (0x01)
#define HDD_STATUS_INDEX            (0x02)
#define HDD_STATUS_CORRECTED_DATA   (0x04)
#define HDD_STATUS_DATA_REQUEST     (0x08)
#define HDD_STATUS_SEEK_COMPLETE    (0x10)
#define HDD_STATUS_WRITE_FAULT      (0x20)
#define HDD_STATUS_READY            (0x40)
#define HDD_STATUS_BUSY             (0x80)

// Drive & Head Register
#define HDD_DRIVE_HEAD_LBA          (0xE0)
#define HDD_DRIVE_HEAD_SLAVE        (0x10)

// Digital Output Register
#define HDD_DIGITAL_OUTPUT_RESET                (0x04)
#define HDD_DIGITAL_OUTPUT_DISABLE_INTERRUPT    (0x01)

// Response Wait Time(millisecond)
#define HDD_WAIT_TIME               (500)

// #Sectors can be read or written from/to HDD at one time
#define HDD_MAX_BULK_SECTOR_COUNT   (256)

// Sturct
// 1 Byte Align
#pragma pack(push, 1)

typedef struct kHDDInformationStruct {
    // 설정 값
    WORD config;

    // 실린더 수
    WORD numberOfCylinder;
    WORD reserved1;

    // 헤드 수
    WORD numberOfHead;
    WORD unformatBytesPerTrack;
    WORD unformatBytesPerSector;

    // 실린더당 섹터 수
    WORD numberOfSectorPerCylinder;
    WORD interSectorGap;
    WORD bytesInPhaseLock;
    WORD numberOfVendorUniqueStatusWord;

    // HDD Serial Number
    WORD serialNumber[10];
    WORD controllerType;
    WORD bufSize;
    WORD numberOfECCBytes;
    WORD firmwareRevision[4];

    // HDD Model Number
    WORD modelNumber[20];
    WORD reserved2[13];

    // Total number of Sectors in HDD
    DWORD totalSectors;
    WORD reserved3[196];
} HDD_INFO;

#pragma pack(pop);

// HDD Manager
typedef struct kHDDManagerStruct {
    // HDD exist? writable?
    BOOL isDetected;
    BOOL isWritable;

    // Interruptable
    volatile BOOL isPrimaryPortInterruptable;
    volatile BOOL isSecondaryPortInterruptable;

    // Mutex
    MUTEX mutex;

    // HDD Info
    HDD_INFO info;
} HDD_MANAGER;

// Function
BOOL kInitializeHDD(void);
BOOL kReadHDDInfo(BOOL isPrimary, BOOL isMaster, HDD_INFO *pHDDInfo);
int kReadHDDSector(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf);
int kWriteHDDSector(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf);
void kSetHDDInterruptFlag(BOOL isPrimary, BOOL flag);
unsigned char kWaitForHDDNotBusyAndReadHDDStatus(BOOL isPrimary);

static void kSwapByteInWord(WORD *pData, int wordCnt);
static BYTE kReadHDDStatus(BOOL isPrimary);
static BOOL kIsHDDBusy(BYTE status);
static BOOL kIsHDDReady(BYTE status);
static BOOL kWaitForHDDNotBusy(BOOL isPrimary);
static BOOL kWaitForHDDReady(BOOL isPrimary);
static BOOL kWaitForHDDInterrupt(BOOL isPrimary);

#endif  // __HDD_H__