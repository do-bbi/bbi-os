#include "HDD.h"
#include "AssemblyUtil.h"
#include "Utility.h"
#include "Console.h"

// HDD Manager
static HDD_MANAGER gHDDManager;

// Initialize HDD Device Driver
BOOL kInitializeHDD(void) {
    // Init Mutex
    kInitializeMutex(&gHDDManager.mutex);

    // Init Interrupt Flag
    gHDDManager.isPrimaryPortInterruptable = FALSE;
    gHDDManager.isSecondaryPortInterruptable = FALSE;

    // 첫 번째와 두 번째 PATA 포트의 디지털 출력 레지스터에 0 출력
    // HDD Controller의 인터럽트 활성화
    kOutPortByte(HDD_PORT_PRIMARY_BASE + HDD_PORT_IDX_DIGITALOUTPUT, 0);    // 0x3F6
    kOutPortByte(HDD_PORT_SECONDARY_BASE + HDD_PORT_IDX_DIGITALOUTPUT, 0);  // 0x376

    // Request HDD Info
    if(kReadHDDInfo(TRUE, TRUE, &gHDDManager.info) == FALSE) {
        gHDDManager.isDetected = FALSE;
        gHDDManager.isWritable = FALSE;

        return FALSE;
    }

    // HDD 검색 성공 시, QEMU에서만 사용 가능하도록 설정
    gHDDManager.isDetected = TRUE;
    if(kMemCmp(gHDDManager.info.modelNumber, "QEMU", 4) == 0)
        gHDDManager.isWritable = TRUE;
    else
        gHDDManager.isWritable = FALSE;
    
    return TRUE;
}

BOOL kReadHDDInfo(BOOL isPrimary, BOOL isMaster, HDD_INFO *pHDDInfo) {
    WORD portBase;
    QWORD lastTickCnt;
    BYTE status;
    BYTE flag;

    int i;
    WORD tmp;
    BOOL result;

    // PATA 포트에 따라서 I/O 포트의 기본 주소 설정
    portBase = isPrimary ? HDD_PORT_PRIMARY_BASE : HDD_PORT_SECONDARY_BASE;

    kLock(&gHDDManager.mutex);
    
    if(kWaitForHDDNotBusy(isPrimary) == FALSE) {
        kUnlock(&gHDDManager.mutex);
        kPrintf("kWaitForHDDNotBusy == FALSE\n");
        return FALSE;
    }

    // LBA 주소, 드라이브, 헤드 관련 레지스터 설정
    flag = HDD_DRIVE_HEAD_LBA;
    if(isMaster == FALSE)
        flag = flag | HDD_DRIVE_HEAD_SLAVE;
    
    kOutPortByte(portBase + HDD_PORT_IDX_DRIVEANDHEAD, flag);

    // Send Command & Wait
    if(kWaitForHDDReady(isPrimary) == FALSE) {
        kUnlock(&gHDDManager.mutex);
        kPrintf("kWaitForHDDReady == FALSE\n");
        return FALSE;
    }

    kSetHDDInterruptFlag(isPrimary, FALSE);

    kOutPortByte(portBase + HDD_PORT_IDX_COMMAND, HDD_COMMAND_IDENTIFY);

    result = kWaitForHDDInterrupt(isPrimary);

    status = kReadHDDStatus(isPrimary);

    if(result == FALSE || (status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
        kUnlock(&gHDDManager.mutex);
        kPrintf("result == FALSE || ERROR\n");
        return FALSE;
    }

    for(i = 0; i < 512 / 2; ++i)
        ((WORD *)pHDDInfo)[i] = kInPortWord(portBase + HDD_PORT_IDX_DATA);
    
    kSwapByteInWord(pHDDInfo->modelNumber, sizeof(pHDDInfo->modelNumber) / 2);
    kSwapByteInWord(pHDDInfo->serialNumber, sizeof(pHDDInfo->serialNumber) / 2);

    kUnlock(&gHDDManager.mutex);

    return TRUE;
}

int kReadHDDSector(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf) {
    WORD portBase;
    int i, j;
    BYTE flag;

    BYTE status;
    long readCnt = 0;
    BOOL result;

    // Check range, Maximum 256 Sectors
    if(gHDDManager.isDetected == FALSE || 
        sectorCnt <= 0 || 256 < sectorCnt || 
        lba + sectorCnt >= gHDDManager.info.totalSectors)
        return 0;
    
    portBase = isPrimary ? HDD_PORT_PRIMARY_BASE : HDD_PORT_SECONDARY_BASE;

    kLock(&gHDDManager.mutex);

    // HDD가 수행중인 커맨드가 있을 경우 대기
    if(kWaitForHDDNotBusy(isPrimary) == FALSE) {
        kUnlock(&gHDDManager.mutex);
        kPrintf("kWaitForHDDNotBusy == FALSE\n");
        return FALSE;
    }

    // 데이터 레지스터 설정
    // LBA 모드 = 섹터 번호 -> 실린더 번호 -> 헤드 번호 순으로 LBA 주소 대입
    
    // 섹터 수 레지스터(Port 0x1F2 or 0x172)에 섹터 수 전송
    kOutPortByte(portBase + HDD_PORT_IDX_SECTORCNT, sectorCnt);

    // 섹터 번호 레지스터(Port 0x1F3 or 0x173)에 섹터 위치(LBA 0-7 bit) 전송
    kOutPortByte(portBase + HDD_PORT_IDX_SECTORNUM, lba);

    // 실린더 LSB 레지스터(Port 0x1F4 or 0x174)에 섹터 위치(LBA 8-15 bit) 전송
    kOutPortByte(portBase + HDD_PORT_IDX_CYLINDERLSB, lba >> 8);

    // 실린더 MSB 레지스터(Port 0x1F5 or 0x175)에 섹터 위치(LBA 16-23 bit) 전송
    kOutPortByte(portBase + HDD_PORT_IDX_CYLINDERMSB, lba >> 16);

    flag = HDD_DRIVE_HEAD_LBA;
    if(isMaster == FALSE)
        flag = flag | HDD_DRIVE_HEAD_SLAVE;

    // 드라이브/헤드 레지스터(Port 0x1F6 or 0x176)에 섹터 위치(LBA 24-27bit) | flag 전송
    kOutPortByte(portBase + HDD_PORT_IDX_DRIVEANDHEAD, ((lba >> 24) & 0x0F) | flag);

    // 커맨드 전송이 가능해질 때까지 대기
    if(kWaitForHDDReady(isPrimary) == FALSE) {
        kUnlock(&gHDDManager.mutex);
        kPrintf("kWaitForHDDReady == FALSE\n");
        return FALSE;
    }

    // Interrupt 플래그 초기화
    kSetHDDInterruptFlag(isPrimary, FALSE);

    // Command 레지스터(Port 0x1F7 or 0x177)에 READ(0x20) 명령 전송
    kOutPortByte(portBase + HDD_PORT_IDX_COMMAND, HDD_COMMAND_READ);

    // Interrupt 대기 후, 데이터 수신
    for(i = 0; i < sectorCnt; ++i) {
        status = kReadHDDStatus(isPrimary);

        // 에러 발생 시 종료
        if((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kPrintf("HDD Error Occur\n");
            kUnlock(&gHDDManager.mutex);
            return i;
        }

        // DATA_REQUEST 비트가 설정되지 않았으면, 데이터 수신을 기다림
        if((status & HDD_STATUS_DATA_REQUEST) != HDD_STATUS_DATA_REQUEST) {
            result = kWaitForHDDInterrupt(isPrimary);
            kSetHDDInterruptFlag(isPrimary, FALSE);

            // 인터럽트가 발생하지 않았다면, 비정상 종료
            if(result == FALSE) {
                kPrintf("Read HDD Interrupt Not Occur\n");
                kUnlock(&gHDDManager.mutex);
                break;
            }
        }

        // Read 1 Sector
        for(j = 0; j < 512 / 2; ++j)
            ((WORD *)pBuf)[readCnt++] = kInPortWord(portBase + HDD_PORT_IDX_DATA);
    }

    kUnlock(&gHDDManager.mutex);

    return i;
}

int kWriteHDDSector(BOOL isPrimary, BOOL isMaster, DWORD lba, int sectorCnt, char *pBuf) {
    WORD portBase;
    WORD tmp;
    int i, j;
    BYTE flag;

    BYTE status;
    long writeCnt = 0;
    BOOL result;

    // Check range, Maximum 256 Sectors
    if(gHDDManager.isWritable == FALSE || 
        sectorCnt <= 0 || 256 < sectorCnt || 
        lba + sectorCnt >= gHDDManager.info.totalSectors)
        return 0;
    
    portBase = isPrimary ? HDD_PORT_PRIMARY_BASE : HDD_PORT_SECONDARY_BASE;

    // HDD가 수행중인 커맨드가 있을 경우 대기
    if(kWaitForHDDNotBusy(isPrimary) == FALSE)
        return FALSE;
    
    kLock(&gHDDManager.mutex);

    // 데이터 레지스터 설정
    // LBA 모드 = 섹터 번호 -> 실린더 번호 -> 헤드 번호 순으로 LBA 주소 대입
    
    // 섹터 수 레지스터(Port 0x1F2 or 0x172)에 섹터 수 전송
    kOutPortByte(portBase + HDD_PORT_IDX_SECTORCNT, sectorCnt);

    // 섹터 번호 레지스터(Port 0x1F3 or 0x173)에 섹터 위치(LBA 0-7 bit) 전송
    kOutPortByte(portBase + HDD_PORT_IDX_SECTORNUM, lba);

    // 실린더 LSB 레지스터(Port 0x1F4 or 0x174)에 섹터 위치(LBA 8-15 bit) 전송
    kOutPortByte(portBase + HDD_PORT_IDX_CYLINDERLSB, lba >> 8);

    // 실린더 MSB 레지스터(Port 0x1F5 or 0x175)에 섹터 위치(LBA 16-23 bit) 전송
    kOutPortByte(portBase + HDD_PORT_IDX_CYLINDERMSB, lba >> 16);

    flag = HDD_DRIVE_HEAD_LBA;
    if(isMaster == FALSE)
        flag = flag | HDD_DRIVE_HEAD_SLAVE;

    // 드라이브/헤드 레지스터(Port 0x1F6 or 0x176)에 섹터 위치(LBA 24-27bit) | flag 전송
    kOutPortByte(portBase + HDD_PORT_IDX_DRIVEANDHEAD, ((lba >> 24) & 0x0F) | flag);

    // 커맨드 전송이 가능해질 때까지 대기
    if(kWaitForHDDReady(isPrimary) == FALSE) {
        kUnlock(&gHDDManager.mutex);
        return FALSE;
    }

    // Command 레지스터(Port 0x1F7 or 0x177)에 WRITE(0x30) 명령 전송
    kOutPortByte(portBase + HDD_PORT_IDX_COMMAND, HDD_COMMAND_WRITE);

    // Interrupt 대기 후, 데이터 수신
    while(TRUE) {
        status = kReadHDDStatus(isPrimary);

        // 에러 발생 시 종료
        if((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kPrintf("HDD Error Occur\n");
            kUnlock(&gHDDManager.mutex);
            return 0;
        }

        // DATA_REQUEST 비트가 설정됐다면, 데이터 송신
        if((status & HDD_STATUS_DATA_REQUEST) == HDD_STATUS_DATA_REQUEST)
            break;
        
        kSleep(1);
    }

    // Sector 수 만큼 데이터 전송
    for(i = 0; i < sectorCnt; ++i) {
        // Interrupt 플래그 초기화
        kSetHDDInterruptFlag(isPrimary, FALSE);

        // Write 1 Sector
        for(j = 0; j < 512 / 2; ++j)
            kOutPortWord(portBase + HDD_PORT_IDX_DATA, ((WORD *)pBuf)[writeCnt++]);
        
        status = kReadHDDStatus(isPrimary);
        // 에러 발생 시 종료
        if((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kPrintf("HDD Error Occur\n");
            kUnlock(&gHDDManager.mutex);
            return i;
        }

        // DATA_REQUEST 비트가 설정되지 않았으면, 데이터 수신을 기다림
        if((status & HDD_STATUS_DATA_REQUEST) != HDD_STATUS_DATA_REQUEST) {
            result = kWaitForHDDInterrupt(isPrimary);
            
            kSetHDDInterruptFlag(isPrimary, FALSE);
            // 인터럽트가 발생하지 않았다면, 비정상 종료
            if(result == FALSE) {
                kPrintf("Write HDD Interrupt Not Occur\n");
                kUnlock(&gHDDManager.mutex);
                break;
            }
        }
    }

    kUnlock(&gHDDManager.mutex);

    return i;
}

void kSetHDDInterruptFlag(BOOL isPrimary, BOOL flag) {
    if(isPrimary)
        gHDDManager.isPrimaryPortInterruptable = flag;
    else
        gHDDManager.isSecondaryPortInterruptable = flag;
}

static void kSwapByteInWord(WORD *pData, int wordCnt) {
    int i;
    WORD tmp;

    for(i = 0; i < wordCnt; ++i) {
        tmp = pData[i];
        pData[i] = (tmp << 8) | (tmp >> 8);
    }
}

static BYTE kReadHDDStatus(BOOL isPrimary) {
    if(isPrimary)
        return kInPortByte(HDD_PORT_PRIMARY_BASE + HDD_PORT_IDX_STATUS);

    return kInPortByte(HDD_PORT_SECONDARY_BASE + HDD_PORT_IDX_STATUS);
}

static BOOL kIsHDDBusy(BOOL isPrimary) {
    BYTE status = kReadHDDStatus(isPrimary);
    
    return (status & HDD_STATUS_BUSY) == HDD_STATUS_BUSY;
}

static BOOL kIsHDDReady(BOOL isPrimary) {
    BYTE status = kReadHDDStatus(isPrimary);
    
    return (status & HDD_STATUS_READY) == HDD_STATUS_READY;
}

static BOOL kWaitForHDDNotBusy(BOOL isPrimary) {
    QWORD waitTickCnt;
    BYTE status;

    waitTickCnt = kGetTickCount() + HDD_WAIT_TIME;

    // 일정 시간 동안 HDD의 Busy 상태가 끝날 때까지 대기
    do {
        // Return HDD Status
        status = kReadHDDStatus(isPrimary);

        // Busy BIT가 설정되어 있지 않으면 종료
        if((status & HDD_STATUS_BUSY) != HDD_STATUS_BUSY)
            return TRUE;

        kSleep(1);
    } while(kGetTickCount() <= waitTickCnt);

    return FALSE;
}

static BOOL kWaitForHDDReady(BOOL isPrimary) {
    QWORD waitTickCnt;
    BYTE status;

    waitTickCnt = kGetTickCount() + HDD_WAIT_TIME;

    // 일정 시간 동안 HDD 인터럽트 발생을 대기
    do {
        status = kReadHDDStatus(isPrimary);

        // CHECK HDD_STATUS_READY(0x40)
        if((status & HDD_STATUS_READY) == HDD_STATUS_READY)
            return TRUE;
        kSleep(1);
    } while(kGetTickCount() <= waitTickCnt);

    return FALSE;
}

static BOOL kWaitForHDDInterrupt(BOOL isPrimary) {
    QWORD waitTickCnt;

    // 대기 시작 시간 저장
    waitTickCnt = kGetTickCount() + HDD_WAIT_TIME;

    // 일정 시간 동안 HDD 인터럽트 발생을 대기
    do {
        if(isPrimary && gHDDManager.isPrimaryPortInterruptable)
            return TRUE;
        else if(isPrimary == FALSE && gHDDManager.isSecondaryPortInterruptable)
            return TRUE;
    } while(kGetTickCount() <= waitTickCnt);

    return FALSE;
}