#ifndef __PIT_H__
#define __PIT_H__

#include "Types.h"

#define PIT_FREQUENCY           (1193182)
#define MSTOCOUNT(x)            ( PIT_FREQUENCY * (x) / 1000 )
#define USTOCOUNT(x)            ( PIT_FREQUENCY * (x) / 1000000 )

// I/O Port
#define PIT_PORT_CONTROL        (0x43)
#define PIT_PORT_COUNTER0       (0x40)
#define PIT_PORT_COUNTER1       (0x41)
#define PIT_PORT_COUNTER2       (0x42)

// Mode
#define PIT_CONTROL_COUNTER0    (0x00)
#define PIT_CONTROL_COUNTER1    (0x40)
#define PIT_CONTROL_COUNTER2    (0x80)

#define PIT_CONTROL_LSBMSBRW    (0x30)
#define PIT_CONTROL_LATCH       (0x00)
#define PIT_CONTROL_MODE0       (0x00)
#define PIT_CONTROL_MODE2       (0x04)

// BIN/BCD
#define PIT_CONTROL_BIN_COUNTER  (0x00)
#define PIT_CONTROL_BCD_COUNTER  (0x01)

// Combination
#define PIT_COUNTER0_ONCE       (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | \
                                 PIT_CONTROL_MODE0 | PIT_CONTROL_BIN_COUNTER)

#define PIT_COUNTER0_PERIODIC   (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | \
                                 PIT_CONTROL_MODE2 | PIT_CONTROL_BIN_COUNTER)

#define PIT_COUNTER0_LATCH      (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LATCH)

// Function
void kInitializePIT(WORD count, BOOL isPeriodic);
WORD kReadCounter0(void);
void kWaitUsingDirectPIT(WORD count);

#endif  // __PIT_H__