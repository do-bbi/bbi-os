#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"

CONSOLEMANAGER gConsoleManager = {0, };

void kInitializeConsole(int x, int y) {
    kMemSet(gConsoleManager, 0, sizeof(gConsoleManager));
}

void kSetCursor(int x, int y) {
    int pos;

    pos = y * CONSOLE_WIDTH + x;

    // CRTC 컨트롤 Address 레지스터(Port 0x3D4)에 0x0E 전송 -> 상위 커서 위치 레지스터 선택
    kOutPortByte(VGA_PORT_IDX, VGA_INDEX_UPPERCURSOR);

    // CRTC 컨트롤 Data 레지스터(Port 0x3D5)에 커서 상위 Byte 출력
    kOutPortByte(VGA_PORT_DATA, pos >> 8);

    // CRTC 컨트롤 Address 레지스터(Port 0x3D4)에 0x0F 전송 -> 하위 커서 위치 레지스터 선택
    kOutPortByte(VGA_PORT_IDX, VGA_INDEX_UPPERCURSOR);

    // CRTC 컨트롤 Data 레지스터(Port 0x3D5)에 커서 하위 Byte 출력
    kOutPortByte(VGA_PORT_DATA, pos & 0xFF);

    gConsoleManager.currentPrintOffset = pos;
}

void kGetCursor(int *pX, int *pY) {
    *pX = gConsoleManager.currentPrintOffset % CONSOLE_WIDTH;
    *pY = gConsoleManager.currentPrintOffset % CONSOLE_WIDTH;
}

void kPrintf(const char *pFormatStr, ...);
int kConsolePrintString(const char *pBuf);
void kClearScreen(void);
BYTE kGetCh(void);
void kPrintStringXY(int x, int y, const char *pStr);