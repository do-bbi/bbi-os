#include "Types.h"
#include "AssemblyUtil.h"
#include "Keyboard.h"

// 출력 버퍼(포트 0x60) 수신 데이터 확인
BOOL kIsOutputBufferFull(void) {
    // 상태 레지스터(포트 0x64)에서 읽은 값의 출력 버퍼 상태 비트(BIT 0)가
    // 1로 설정 되어 있는 경우, 출력 버퍼에 키보드가 전송한 데이터가 존재
    return (kInPortByte(0x64) & 0x1) ? TRUE : FALSE;
}

// 입력 버퍼(포트 0x64) 프로세서가 쓴 데이터 확인
BOOL kIsInputBufferFull(void) {
    // 상태 레지스터(포트 0x64)에서 읽은 값의 입력 버퍼 상태 비트(BIT 1)가
    // 1로 설정 되어 있는 경우, 출력 버퍼에 키보드가 전송한 데이터가 존재
    return (kInPortByte(0x64) & 0x2) ? TRUE : FALSE;
}

BOOL kActivateKeyboard(void) {
    int i, j;

    // 컨트롤 레지스터(포트 0x64)에 키보드 활성화 커맨드(0xAE)를 전달하여 키보드 디바이스 활성화
    kOutPortByte(0x64, 0xAE);

    // 입력 버퍼(포트 0x60)가 빌 때까지 기다렸다가 키보드에 활성화 커맨드 전송
    // 0xFFFF 번 루프를 수행할 시간이면 충분히 커맨드 전송이 끝남
    // 0xFFFF 번 루프를 수행 이 후에도 입력 버퍼(포트 0x60)이 비어있지 않으면 무시하고 전송

    for(i = 0; i < 0xFFFF; ++i) {
        if(!kIsInputBufferFull())
            break;
    }

    // 입력 버퍼(0x60)로 키보드 활성화 커맨드(0xF4)를 전송
    kOutPortByte(0x60, 0xF4);

    // ACK 수신 대기
    // ACK 수신 전에 키보드 출력 버퍼(포트 0x60)에 Key 값이 저장되어 있을 수 있으므로
    // 키보드에서 전달 된 데이터를 최대 100개 까지 수신하여 ACK 확인
    for(j = 0; j < 100; ++j) {
        // 0xFFFF 번 루프를 수행할 시간이면 충분히 커맨드 전송이 끝남
        // 0xFFFF 번 루프를 수행 이 후에도 입력 버퍼(포트 0x60)이 쓰여있지 않으면 무시하고 읽음
        for(i = 0; i < 0xFFFF; ++i) {
            if(kIsOutputBufferFull())
                break;
        }

        // 출력 버퍼(포트 0x60)에서 읽은 데이터가 ACK(0xFA)이면 성공
        if(kInPortByte(0x60) == 0xFA)
            return TRUE;
    }

    return FALSE;
}

// 출력 버퍼(포트 0x60)에서 키를 읽음
BYTE kGetKeyboardScanCode(void) {
    // 출력 버퍼(포트 0x60)에 데이터가 있을 때 까지 대기
    while(!kIsOutputBufferFull());

    return kInPortByte(0x60);
}
