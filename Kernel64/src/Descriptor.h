#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

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

// Upper Flags[Granulaty]
#define GDT_FLAGS_UPPER_CODE        (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA        (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS         (GDT_FLAGS_UPPER_G)

// Segment Descriptor Offset
#define GDT_KERNEL_CODE_SEGMENT (0x08)
#define GDT_KERNEL_DATA_SEGMENT (0x10)
#define GDT_TSS_SEGMENT         (0x18)

// ENV
#define GDTR_BASE_ADDR          (0x142000)  // 1MB + 264KB

// Number of 8 Bytes Entries -> NULL Descriptor, Kernel Code, Kernel Data
#define GDT_MAX_ENTRY8_COUNT    (3)

// Number of 16 Bytes Entries -> TSS
#define GDT_MAX_ENTRY16_COUNT   (1)

#define GDT_TABLE_SIZE          ( (sizeof(GDTENTRY8) * GDT_MAX_ENTRY8_COUNT) + \
                                  (sizeof(GDTENTRY16) * GDT_MAX_ENTRY16_COUNT) )

#define TSS_SEGMENT_SIZE        (sizeof(TSSSEGMENT))


#endif  // __DESCRIPTOR_H__