#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"

#define PAGE_FLAGS_P	    (0x1 << 0)	// Present
#define PAGE_FLAGS_RW	    (0x1 << 1)	// Read/Write
#define PAGE_FLAGS_US	    (0x1 << 2)	// User/Supervisor(플래그 설정 시 유저 레벨)
#define PAGE_FLAGS_PWT	    (0x1 << 3)	// Page Level Write-through
#define PAGE_FLAGS_PCD	    (0x1 << 4)	// Page Level Cache Disable
#define PAGE_FLAGS_A	    (0x1 << 5)	// Accessed
#define PAGE_FLAGS_D	    (0x1 << 6)	// Dirty
#define PAGE_FLAGS_PS	    (0x1 << 7)	// Page Size
#define PAGE_FLAGS_G	    (0x1 << 8)	// Global
#define PAGE_FLAGS_AVAIL	(0x7 << 9)	// Avail
#define PAGE_FLAGS_PAT	    (0x1 << 12)	// Page Attribute Table Indx

// 상위 32비트 용 속성 필드
#define PAGE_FLAGS_EXB	    (0x1 << 31)	// Execute Disable 비트

// ETC
#define PAGE_FLAGS_DEF	    (PAGE_FLAGS_P | PAGE_FLAGS_RW)

#define PAGE_TABLESIZE       (0x1000)
#define PAGE_MAXENTRYCOUNT  (512)
#define PAGE_DEFAULT_SIZE   (0x200000)  // 2^21 == 2MB

// Struct
#pragma pack(push, 1)

// Data Structure for PageEntry
typedef struct PageTableEntryStruct {
    // if PML4T & PDPTE
    // Base Addr[40:21] | Reserved[20:13] | PAT[12] | Avail[11:9] | G[8] | PS[7] | D[6] | A[5] | PCD[4] | PWT[3] | US[2] | RW[1] | P[0]
    // if PDE
    // Base Addr[31:21] | Avail[20:13] | PAT[12] | Avail[11:9] | G[8] | 1 [7] | D[6] | A[5] | PCD[4] | PWT[3] | US[2] | RW[1] | P[0]
	DWORD attributeAndLowerBaseAddr;

    // EXB[63] | Avail[62:52] | Reserved[51:40] | Base Addr[39:32]
	DWORD upperBaseAddrAndEXB;
} PML4TENTRY, PDPTENTRY,  PDENTRY, PTENTRY;
#pragma pack(pop)

// Page managing Function
void kInitializePageTables(void);
void kSetPageEntryData(PTENTRY *pEntry, DWORD upperBaseAddr, DWORD lowerBaseAddr, DWORD lowerFlags, DWORD upperFlags);

#endif  // __PAGE_H__