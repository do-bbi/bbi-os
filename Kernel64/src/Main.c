#include "Types.h"
#include "Keyboard.h"

#define VIDEO_MEM_ADDR  (0xB8000)

#define PRINT_BLANK_POS  (45)

void kPrintString(int iX, int iY, const char* pcString);

// C 언어 커널
void Main(void) {
    char temp[2] = {0, };
    BYTE tmp, flags;
    int i = 0;

    int posY = 10;
    kPrintString(PRINT_BLANK_POS, posY, "PASS");
    posY++;
    kPrintString(0, posY, "IA-32e C Language Kernel Start");
    posY++;
    kPrintString(0, posY, "Keyboard Activate...........................[    ]");

    if(kActivateKeyboard()) {
        kPrintString(PRINT_BLANK_POS, posY, "PASS");
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kPrintString(PRINT_BLANK_POS, posY, "FAIL");
        while(TRUE);
    }

    posY++;
    while(TRUE) {
        // 출력 버퍼(Port 0x60)가 차 있으면 Scan code를 읽을 수 있음
        if(kIsOutputBufferFull()) {
            // 출력 버퍼(Port 0x60)에서 Scan code를 읽어서 저장
            tmp = kGetKeyboardScanCode();

            // Scan code를 ASCII 코드와 Key Up/Down 정보로 변환
            if(kConvertScanCodeToASCIICode(tmp, temp, &flags) && (flags & KEY_FLAGS_DOWN))
                kPrintString(i++, 13, temp);
        }
    }
}

void kPrintString(int iX, int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)VIDEO_MEM_ADDR;    // Video Memory Addr
    int i;

    pstScreen += (iY * 80) + iX;
    for(i = 0; pcString[i]; ++i)
        pstScreen[i].bCharacter = pcString[i];
}