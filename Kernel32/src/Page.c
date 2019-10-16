#include "Page.h"

#define PAGE_TABLE_ADDR         (0x100000)
#define PAGE_DIR_PTR_TABLE_ADDR (0x101000)
#define PAGE_DIR_TABLE_ADDR     (0x102000)

#define DIR_PTR_ENTRY_NUM       (64)

// IA-32e 모드 커널을 위한 페이지 테이블 생성
void kInitializePageTables(void) {
    PML4TENTRY *pPML4TEntry;
    PDPTENTRY *pPDPTEntry;
    PDENTRY *pPDEntry;
    DWORD mappingAddr;

    int i;

    // Create PML4 Table
    // Initialize entries except first to 0
    pPML4TEntry = (PML4TENTRY *)PAGE_TABLE_ADDR;
    kSetPageEntryData(&(pPML4TEntry[0]), 0x00, PAGE_DIR_PTR_TABLE_ADDR, PAGE_FLAGS_DEF, 0);

    for(i = 1; i < PAGE_MAXENTRYCOUNT; ++i)
        kSetPageEntryData(&pPML4TEntry[i], 0, 0, 0, 0);

    // PDPT(Page Directory Pointer Table) 생성
    // 하나의 PDPT로 512GB 까지 Mapping이 가능하므로, 하나의 PDPT만 생성하는걸로 충분
    // 64개의 Entry를 설정해 64GB 까지 Mapping 즉, 1개의 Entry는 1GB Mapping 가능
    pPDPTEntry = (PDPTENTRY *)PAGE_DIR_PTR_TABLE_ADDR;

    for(i = 0; i < DIR_PTR_ENTRY_NUM; ++i)
        kSetPageEntryData(&(pPDPTEntry[i]), 0, PAGE_DIR_TABLE_ADDR + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEF, 0);
    
    for(i = DIR_PTR_ENTRY_NUM; i < PAGE_MAXENTRYCOUNT; ++i)
        kSetPageEntryData(&(pPDPTEntry[i]), 0, 0, 0, 0);

    // PDP(Page Directory Table) 생성
    // 하나의 Page Directory가 1GB 까지 Mapping 가능
    // 64개의 Page Directory를 생성하여 총 64GB 까지 지원함
    pPDEntry = (PDENTRY *)PAGE_DIR_TABLE_ADDR;
    mappingAddr = 0;

    for(i = 0; i < PAGE_MAXENTRYCOUNT * DIR_PTR_ENTRY_NUM; ++i) {
        // 32bit 로는 상위 주소를 표현할 수 없으므로, MB 단위로 계산한 후에
        // 최종 결과를 다시 4KB(2^12)로 나눠 32bit 이상의 주소를 계산함
        kSetPageEntryData(&(pPDEntry[i]), (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12, mappingAddr, PAGE_FLAGS_DEF | PAGE_FLAGS_PS, 0);
        mappingAddr += PAGE_DEFAULT_SIZE;
    }
}

// Page Entry에 기준 주소와 속성 플래그를 설정
void kSetPageEntryData(PTENTRY *pEntry, DWORD upperBaseAddr, DWORD lowerBaseAddr, DWORD lowerFlags, DWORD upperFlags) {
    pEntry->attributeAndLowerBaseAddr = lowerBaseAddr | lowerFlags;
    pEntry->upperBaseAddrAndEXB = (upperBaseAddr & 0xFF) | upperFlags;  // upperBaseAddr[39:32]
}