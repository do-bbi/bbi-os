#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

// I/O Port
#define RTC_CMOSADDRESS         (0x70)
#define RTC_CMOSDATA            (0x71)

// CMOS Memory Address
#define RTC_ADDRESS_SECOND      (0x00)
#define RTC_ADDRESS_MINUTE      (0x02)
#define RTC_ADDRESS_HOUR        (0x04)

#define RTC_ADDRESS_DAYOFWEEK   (0x06)
#define RTC_ADDRESS_DAYOFMONTH  (0x07)
#define RTC_ADDRESS_MONTH       (0x08)
#define RTC_ADDRESS_YEAR        (0x09)

// Macros
#define RTC_BCD_TO_BIN(x)          ((((x) >> 4) * 10) + ((x) & 0x0F))

// Functions
void kReadRTCTime(BYTE *pHour, BYTE *pMinute, BYTE *pSecond);
void kReadRTCDate(WORD *pYear, BYTE *pMonth, BYTE *pDayOfMonth, BYTE *pDayOfWeek);
char *kConvertDayOfWeekToString(BYTE dayOfWeek);

#endif  // __RTC_H__