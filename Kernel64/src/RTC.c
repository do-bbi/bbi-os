#include "RTC.h"
#include "AssemblyUtil.h"

// CMOS Memory에서 RTC Controller가 저장한 현재 시간을 읽음
void kReadRTCTime(BYTE *pHour, BYTE *pMinute, BYTE *pSecond) {
    BYTE data;

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);    // CMOS Memory Address 레지스터(Port 0x70)에 시간을 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                   // CMOS Data 레지스터(Port 0x71)에서 시간을 읽음
    *pHour = RTC_BCD_TO_BIN(data);

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);  // CMOS Memory Address 레지스터(Port 0x70)에 분을 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                   // CMOS Data 레지스터(Port 0x71)에서 분을 읽음
    *pMinute = RTC_BCD_TO_BIN(data);

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);    // CMOS Memory Address 레지스터(Port 0x70)에 시간을 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                   // CMOS Data 레지스터(Port 0x71)에서 초를 읽음
    *pSecond = RTC_BCD_TO_BIN(data);
}

void kReadRTCDate(WORD *pYear, BYTE *pMonth, BYTE *pDayOfMonth, BYTE *pDayOfWeek) {
    BYTE data;

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);    // CMOS Memory Address 레지스터(Port 0x70)에 연도를 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                   // CMOS Data 레지스터(Port 0x71)에서 연도를 읽음
    *pYear = RTC_BCD_TO_BIN(data) + 2000;

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);   // CMOS Memory Address 레지스터(Port 0x70)에 월을 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                   // CMOS Data 레지스터(Port 0x71)에서 월을 읽음
    *pMonth = RTC_BCD_TO_BIN(data);

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);  // CMOS Memory Address 레지스터(Port 0x70)에 일을 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                       // CMOS Data 레지스터(Port 0x71)에서 일을 읽음
    *pDayOfMonth = RTC_BCD_TO_BIN(data);

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);   // CMOS Memory Address 레지스터(Port 0x70)에 요일을 저장하도록 지정
    data = kInPortByte(RTC_CMOSDATA);                       // CMOS Data 레지스터(Port 0x71)에서 요일을 읽음
    *pDayOfWeek = RTC_BCD_TO_BIN(data);
}

char *kConvertDayOfWeekToString(BYTE dayOfWeek) {
    static char *pDatOfWeekStr[8] = { "Error", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

    // 요일 범위가 넘어가면 Error 반환
    return pDatOfWeekStr[(0 < dayOfWeek && dayOfWeek < 8) ? dayOfWeek : 0];
}