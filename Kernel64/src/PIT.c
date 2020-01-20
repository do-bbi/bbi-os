#include "PIT.h"
#include "AssemblyUtil.h"

// Initialize PIT
void kInitializePIT(WORD count, BOOL isPeriodic) {
    // PIT 컨트롤 레지스터(Port 0x43)에 값을 초기화하여 카운트를 멈춘 뒤
    // Mode 0 Binary Counter로 설정
    kOutPortByte(PIT_PORT_COUNTER0, PIT_COUNTER0_ONCE);

    // 일정 주기로 반복하는 타이머 -> Mode 2
    if(isPeriodic) {
        // PIT Control Register(Port 0x43)에 Mode 2 Binary Counter 설정
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    }

    // Counter 0(Port 0x40)에 LSB -> MSB 순으로 카운터 초기값 설정
    kOutPortByte(PIT_PORT_COUNTER0, count);
    kOutPortByte(PIT_PORT_COUNTER0, count >> 8);
}

WORD kReadCounter0(void) {
    BYTE lsb, msb;  // LSB(Least Significant Bit), MSB(Most Significant Bit)
    WORD tmp;

    // PIT Control Register(Port 0x43)에 Latch 명령 전송 -> Read Value in Counter 0
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    // Counter 0(Port 0x40) Read LSB firstly and MSB later
    lsb = kInPortByte(PIT_PORT_COUNTER0);
    msb = kInPortByte(PIT_PORT_COUNTER0);

    // Convert to 16bit value
    tmp = msb;
    tmp = (tmp << 8) | lsb;
}

/* 
    Counter 0를 설정하여 일정 시간 이상 대기
    함수를 호출하면 PIT 컨트롤러의 설정이 바뀌므로, 이후 PIT 컨트롤러 재설정 필요
    정확하게 측정하려면 함수 사용 전에 인터럽트 비활성화가 필요하며, 약 50ms까지 측정가능
*/
void kWaitUsingDirectPIT(WORD count) {
    WORD lastCnt, curCnt;

    // PIT 컨트롤러를 0x0000 ~ 0xFFFF를 반복하도록 설정
    kInitializePIT(0, TRUE);

    lastCnt = kReadCounter0();
    while (TRUE) {
        curCnt = kReadCounter0();
        if(((lastCnt - curCnt) & 0xFFFF) >= count) 
            break;
    }
}