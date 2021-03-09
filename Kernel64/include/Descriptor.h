#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

// GDT
#define GDT_TYPE_CODE           (0x0A)
#define GDT_TYPE_DATA           (0x02)
#define GDT_TYPE_TSS            (0x09)

#define GDT_FLAGS_LOWER_S       (0x10)
#define GDT_FLAGS_LOWER_DPL0    (0x00)
#define GDT_FLAGS_LOWER_DPL1    (0x20)
#define GDT_FLAGS_LOWER_DPL2    (0x40)
#define GDT_FLAGS_LOWER_DPL3    (0x60)
#define GDT_FLAGS_LOWER_P       (0x80)

#define GDT_FLAGS_UPPER_L       (0x20)
#define GDT_FLAGS_UPPER_DB      (0x40)
#define GDT_FLAGS_UPPER_G       (0x80)

// Lower Flags[Code/Data/TSS, DPL0, Present]
#define GDT_FLAGS_LOWER_KERNEL_CODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
                                    GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_KERNEL_DATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
                                    GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_TSS         (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_USER_CODE   (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
                                    GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_USER_DATA   (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
                                    GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

// Set Upper Flags as "Granulaty"
#define GDT_FLAGS_UPPER_CODE        (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA        (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS         (GDT_FLAGS_UPPER_G)

// Segment Descriptor Offset
#define GDT_KERNEL_CODE_SEGMENT (0x08)
#define GDT_KERNEL_DATA_SEGMENT (0x10)
#define GDT_TS_SEGMENT         (0x18)

// ENV
#define GDTR_BASE_ADDR          (0x142000)  // 1MB + 264KB

// Number of 8 Bytes Entries -> NULL Descriptor, Kernel Code, Kernel Data
#define GDT_MAX_ENTRY8_COUNT    (3)

// Number of 16 Bytes Entries -> TSS
#define GDT_MAX_ENTRY16_COUNT   (1)

#define GDT_TABLE_SIZE          ( (sizeof(GDTENTRY8) * GDT_MAX_ENTRY8_COUNT) + \
                                  (sizeof(GDTENTRY16) * GDT_MAX_ENTRY16_COUNT) )

#define TS_SEGMENT_SIZE        (sizeof(TSSEGMENT))

// IDT
#define IDT_TYPE_INTERRUPT  (0x0E)
#define IDT_TYPE_TRAP       (0x0F)

#define IDT_FLAGS_DPL0      (0x00)
#define IDT_FLAGS_DPL1      (0x20)
#define IDT_FLAGS_DPL2      (0x40)
#define IDT_FLAGS_DPL3      (0x60)
#define IDT_FLAGS_P         (0x80)

#define IDT_FLAGS_IST0      (0)
#define IDT_FLAGS_IST1      (1)

#define IDT_FLAGS_KERNEL    (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER      (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

#define IDT_MAX_ENTRY_COUNT (100)

#define IDTR_BASE_ADDR      (GDTR_BASE_ADDR + sizeof(GDTR) + \
                             GDT_TABLE_SIZE + TS_SEGMENT_SIZE)

#define IDT_BASE_ADDR       (IDTR_BASE_ADDR + sizeof(IDTR))

#define IDT_TABLE_SIZE      (IDT_MAX_ENTRY_COUNT * sizeof(IDTENTRY))

// IST
#define IST_BASE_ADDR       (0x700000)

#define IST_SIZE            (0x100000)

// 1 Byte aligned Struct
#pragma pack(push, 1)

typedef struct kGDTRStruct {
    WORD limit;
    QWORD baseAddr;
    WORD pading1;                    // For 16 Bytes Aligned
    DWORD pading2;
} GDTR, IDTR;

typedef struct kGDTEntry8Struct {
    WORD lowerLimit;
    WORD lowerBaseAddr;
    BYTE upperBaseAddr1;
    BYTE typeAndLowerFlag;          // Type[7:4] | S[3] | DPL[2:1] | P[0]
    BYTE upperLimitAndUpperFlag;    // Segment Limit[7:4] | AVL[3] | L[2] | D/B[1] | G[0]
    BYTE upperBaseAddr2;
} GDTENTRY8;

typedef struct kGDTEntry16Struct {
    WORD lowerLimit;
    WORD lowerBaseAddr;
    BYTE middleBaseAddr1;
    BYTE typeAndLowerFlag;          // Type[7:4] | 0[3] | DPL[2:1] | P[0]
    BYTE upperLimitAndUpperFlag;    // Segment Limit[7:4] | AVL[3] | 0[2] | 0[1] | G[0]
    BYTE middleBaseAddr2;
    DWORD upperBaseAddr;
    DWORD reserved;
} GDTENTRY16;

// Task State Segment
typedef struct kTSSDataStruct {
    DWORD reserved1;
    QWORD rsp[3];
    QWORD reserved2;
    QWORD ist[7];
    QWORD reserved3;
    WORD reserved;
    WORD ioMapBaseAddr;
} TSSEGMENT;

typedef struct kIDTEntryStruct {
    WORD lowerBaseAddr;
    WORD segmentSelector;
    BYTE ist;                       // IST[7:5] | 0x0[4:0]
    BYTE typeAndFlags;              // Type[7:4] | 0[5] | DPL[2:1] | P[0]
    WORD middleBaseAddr;
    DWORD upperBaseAddr;
    DWORD reserved;
} IDTENTRY;

#pragma pack(pop)

void kInitializeGDTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8 *pEntry8, DWORD baseAddr, DWORD limit, BYTE upperFlags, BYTE lowerFlags, BYTE type);
void kSetGDTEntry16(GDTENTRY16 *pEntry16, QWORD baseAddr, DWORD limit, BYTE upperFlags, BYTE lowerFlags, BYTE type);
void kInitializeTSSegment(TSSEGMENT *pTSS);
void kInitializeIDTables(void);
void kSetIDTEntry(IDTENTRY *pEntry, void *pHandler, WORD selector, BYTE ist, BYTE flags, BYTE type);
void kDummyHandler(void);

#endif  // __DESCRIPTOR_H__