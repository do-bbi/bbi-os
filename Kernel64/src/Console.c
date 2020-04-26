#include <stdarg.h>
#include "Utility.h"
#include "Console.h"
#include "Keyboard.h"
#include "Task.h"

#define MAX_BUF_SIZE    (1024)

CONSOLEMANAGER gConsoleManager = {0, };

void kInitializeConsole(int x, int y) {
    kMemSet(&gConsoleManager, 0, sizeof(gConsoleManager));

    // Set Position of Cursor 
    kSetCursor(x, y);
}

void kSetCursor(int x, int y) {
    int pos;

    pos = y * CONSOLE_WIDTH + x;

    // CRTC 컨트롤 Address 레지스터(Port 0x3D4)에 0x0E 전송 -> 상위 커서 위치 레지스터 선택
    kOutPortByte(VGA_PORT_IDX, VGA_INDEX_UPPERCURSOR);

    // CRTC 컨트롤 Data 레지스터(Port 0x3D5)에 커서 상위 Byte 출력
    kOutPortByte(VGA_PORT_DATA, pos >> 8);

    // CRTC 컨트롤 Address 레지스터(Port 0x3D4)에 0x0F 전송 -> 하위 커서 위치 레지스터 선택
    kOutPortByte(VGA_PORT_IDX, VGA_INDEX_LOWERCURSOR);

    // CRTC 컨트롤 Data 레지스터(Port 0x3D5)에 커서 하위 Byte 출력
    kOutPortByte(VGA_PORT_DATA, pos & 0xFF);

    gConsoleManager.currentPrintOffset = pos;
}

void kGetCursor(int *pX, int *pY) {
    *pX = gConsoleManager.currentPrintOffset % CONSOLE_WIDTH;
    *pY = gConsoleManager.currentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf(const char *pFormatStr, ...) {
    va_list ap;
    
    char pBuf[MAX_BUF_SIZE];
    int nextPrintOffset;

    // Use Variable Parameters

    // #define va_start(AP, LASTARG) (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
    va_start(ap, pFormatStr);
    kVSPrintf(pBuf, pFormatStr, ap);

    // #define va_end(ap) ap = (va_list)NULL
    va_end(ap);

    // Print Format String to console
    nextPrintOffset = kConsolePrintString(pBuf);

    // Update Cursor Position
    kSetCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH); 
 }

int kConsolePrintString(const char *pBuf) {
    VGATEXT *pScreen = (VGATEXT *)CONSOLE_VIDEO_MEM_ADDR;

    int i, j, len;
    int printOffset;

    printOffset = gConsoleManager.currentPrintOffset;

    // Print String
    len = kStrLen(pBuf);
    for(i = 0; i < len; ++i) {
        if(pBuf[i] == '\n') // 줄 바꾸기 + 현재 라인의 남은 문자열을 다음 라인으로 옮김
            printOffset += (CONSOLE_WIDTH - (printOffset % CONSOLE_WIDTH));
        else if(pBuf[i] == '\t')
            printOffset += (8 - (printOffset % 8));
        else {
            pScreen[printOffset].ch = pBuf[i];
            pScreen[printOffset].attr = CONSOLE_DEFAULT_TEXTCOLOR;
            printOffset++;
        }
        
        // 출력 위치가 화면을 벗어나면 스크롤 처리
        if(printOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH)) {
            // 가장 윗줄을 제외한 나머지를 한 줄 위로 복사
            kMemCpy(CONSOLE_VIDEO_MEM_ADDR, 
                CONSOLE_VIDEO_MEM_ADDR + CONSOLE_WIDTH * sizeof(VGATEXT),
                (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(VGATEXT));

            // 가장 마지막 라인은 공백으로 채움
            for(j = (CONSOLE_HEIGHT - 1) * (CONSOLE_WIDTH); j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); ++j) {
                pScreen[j].ch = ' ';
                pScreen[j].attr = CONSOLE_DEFAULT_TEXTCOLOR;
            }

            // 출력할 위치를 가장 아래쪽 라인의 처음으로 설정
            printOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
        }
    }

    return printOffset;
}

void kClearScreen(void) {
    VGATEXT *pScreen = (VGATEXT *)CONSOLE_VIDEO_MEM_ADDR;
    int i;

    for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; ++i) {
        pScreen[i].ch = ' ';
        pScreen[i].attr = CONSOLE_DEFAULT_TEXTCOLOR;
    }

    // 커서를 화면 상단으로 이동
    kSetCursor(0, 0);
}

BYTE kGetCh(void) {
    KEYDATA keyData;

    // Wait until key pressed
    while(TRUE) {
        // Wait until key pushed into queue
        while(kGetKeyFromKeyQueue(&keyData) == FALSE) {
            kSchedule();
        }

        // 키가 눌렸다는 데이터가 수신되면 ASCII 코드 반환
        if(keyData.flags & KEY_FLAGS_DOWN)
            return keyData.asciiCode;
    }
}

// Print String at Position(X, Y)
void kPrintStringXY(int x, int y, const char *pStr) {
    VGATEXT *pScreen = (VGATEXT *)CONSOLE_VIDEO_MEM_ADDR;
    int i;

    // Calculate position to print string
    pScreen += y * CONSOLE_WIDTH + x;

    for(i = 0; pStr[i]; ++i) {
        pScreen[i].ch = pStr[i];
        pScreen[i].attr = CONSOLE_DEFAULT_TEXTCOLOR;
    }
}