#include "Types.h"
#include "AssemblyUtil.h"
#include "Keyboard.h"

// 출력 버퍼(Port 0x60) 수신 데이터 확인
BOOL kIsOutputBufferFull(void) {
    // 상태 레지스터(Port 0x64)에서 읽은 값의 출력 버퍼 상태 비트(BIT 0)가
    // 1로 설정 되어 있는 경우, 출력 버퍼에 키보드가 전송한 데이터가 존재
    return (kInPortByte(0x64) & 0x1) ? TRUE : FALSE;
}

// 입력 버퍼(Port 0x64) 프로세서가 쓴 데이터 확인
BOOL kIsInputBufferFull(void) {
    // 상태 레지스터(Port 0x64)에서 읽은 값의 입력 버퍼 상태 비트(BIT 1)가
    // 1로 설정 되어 있는 경우, 출력 버퍼에 키보드가 전송한 데이터가 존재
    return (kInPortByte(0x64) & 0x2) ? TRUE : FALSE;
}

BOOL kActivateKeyboard(void) {
    int i, j;

    // 컨트롤 레지스터(Port 0x64)에 키보드 활성화 커맨드(0xAE)를 전달하여 키보드 디바이스 활성화
    kOutPortByte(0x64, 0xAE);

    // 입력 버퍼(Port 0x60)가 빌 때까지 기다렸다가 키보드에 활성화 커맨드 전송
    // 0xFFFF 번 루프를 수행할 시간이면 충분히 커맨드 전송이 끝남
    // 0xFFFF 번 루프를 수행 이 후에도 입력 버퍼(Port 0x60)이 비어있지 않으면 무시하고 전송

    for(i = 0; i < 0xFFFF; ++i) {
        if(!kIsInputBufferFull())
            break;
    }

    // 입력 버퍼(0x60)로 키보드 활성화 커맨드(0xF4)를 전송
    kOutPortByte(0x60, 0xF4);

    // ACK 수신 대기
    // ACK 수신 전에 키보드 출력 버퍼(Port 0x60)에 Key 값이 저장되어 있을 수 있으므로
    // 키보드에서 전달 된 데이터를 최대 100개 까지 수신하여 ACK 확인
    for(j = 0; j < 100; ++j) {
        // 0xFFFF 번 루프를 수행할 시간이면 충분히 커맨드 전송이 끝남
        // 0xFFFF 번 루프를 수행 이 후에도 입력 버퍼(Port 0x60)이 쓰여있지 않으면 무시하고 읽음
        for(i = 0; i < 0xFFFF; ++i) {
            if(kIsOutputBufferFull())
                break;
        }

        // 출력 버퍼(Port 0x60)에서 읽은 데이터가 ACK(0xFA)이면 성공
        if(kInPortByte(0x60) == 0xFA)
            return TRUE;
    }

    return FALSE;
}

// 출력 버퍼(Port 0x60)에서 키를 읽음
BYTE kGetKeyboardScanCode(void) {
    // 출력 버퍼(Port 0x60)에 데이터가 있을 때 까지 대기
    while(!kIsOutputBufferFull());

    return kInPortByte(0x60);
}

// 키보드 상태 LED ON/OFF
BOOL kChangeKeyboardLED(BOOL isCapsLockOn, BOOL isNumLockOn, BOOL isScrollLockOn) {
    int i, j;

    for(i = 0; i < 0xFFFF; ++i) {
        if(!kIsInputBufferFull())
            break;
    }

    // 출력 버퍼(Port 0x60)로 LED 상태 변경 명령(0xED) 전송
    kOutPortByte(0x60, 0xED);
    for(i = 0; i < 0xFFFF; ++i) {
        if(!kIsInputBufferFull())
            break;
    }
    
    // ACK 수신 대기
    for(j = 0; j < 100; ++j) {
        // 0xFFFF 번 루프를 수행할 시간이면 충분히 커맨드 전송이 끝남
        // 0xFFFF 번 루프를 수행 이 후에도 입력 버퍼(Port 0x60)이 쓰여있지 않으면 무시하고 읽음
        for(i = 0; i < 0xFFFF; ++i) {
            if(kIsOutputBufferFull())
                break;
        }

        // 출력 버퍼(Port 0x60)에서 읽은 데이터가 ACK(0xFA)이면 성공
        if(kInPortByte(0x60) == 0xFA)
            break;
    }

    if(j >= 100)
        return FALSE;

    // 변경 할 LED 상태를 키보드로 전송하고, 처리가 끝날 때 까지 대기
    kOutPortByte(0x60, (isCapsLockOn << 2) | (isNumLockOn << 1) | (isScrollLockOn << 0));
    for(i = 0; i < 0xFFFF; ++i) {
        if(!kIsInputBufferFull())
            break;
    }

    // ACK 수신 대기
    for(j = 0; j < 100; ++j) {
        // 0xFFFF 번 루프를 수행할 시간이면 충분히 커맨드 전송이 끝남
        // 0xFFFF 번 루프를 수행 이 후에도 입력 버퍼(Port 0x60)이 쓰여있지 않으면 무시하고 읽음
        for(i = 0; i < 0xFFFF; ++i) {
            if(kIsOutputBufferFull())
                break;
        }

        // 출력 버퍼(Port 0x60)에서 읽은 데이터가 ACK(0xFA)이면 성공
        if(kInPortByte(0x60) == 0xFA)
            break;
    }

    returrn (j < 100);
}

// A20 게이트 활성화
void kEnableA20Gate(void) {
    BYTE outputPortData;
    int i;

    // 컨트롤 레지스터(Port 0x64)에 키보드 컨트롤러 출력 포트 값을 읽어들이는 명령(0xD0) 전송
    kOutPortByte(0x64, 0xD0);

    // 출력 포트의 데이터를 기다렸다가 읽어들임
    for(i = 0; i < 0xFFFF; ++i) {
        // 출력 버퍼(Port 0x60)에 값이 들어오면 읽을 수 있음
        if(kIsOutputBufferFull())
            break;
    }

    // 출력 포트(Port 0x60)에 수신된 키보드 컨트롤러의 출력 포트 값 읽기
    outputPortData = kInPortByte(0x60);

    // A20 게이트 활성화 Bit 설정
    outputPortData |= (0x1 << 0);

    // 입력 버퍼(Port 0x60)에 데이터가 비면, 출력 포트에 값을 쓰는 커맨드와 출력 포트 데이터 전송
    for(i = 0; i < 0xFFFF; ++i) {
        // 출력 버퍼(Port 0x60)에 값이 들어오면 읽을 수 있음
        if(!kIsInputBufferFull())
            break;
    }

    // 컨트롤 레지스터(Port 0x64)에 키보드 컨트롤러 출력 포트 값을 쓰는 명령(0xD1) 전송
    kOutPortByte(0x64, 0xD1);

    // 입력 버퍼(Port 0x60)에 A20 게이트 활성화 Bit를 1로 설정한 값 전송
    kOutPortByte(0x60, outputPortData);
}

// Keyboard 프로세서 Reset
void kReboot(void) {
    int i;
    
    // 입력 버퍼(Port 0x60)에 데이터가 비면, 출력 포트에 값을 쓰는 커맨드와 출력 포트 데이터 전송
    for(i = 0; i < 0xFFFF; ++i) {
        // 출력 버퍼(Port 0x60)에 값이 들어오면 읽을 수 있음
        if(!kIsInputBufferFull())
            break;
    }

    // 컨트롤 레지스터(Port 0x64)에 키보드 컨트롤러 출력 포트 값을 쓰는 명령(0xD1) 전송
    kOutPortByte(0x64, 0xD1);

    // 입력 버퍼(Port 0x60)에 0x0을 전달해서 Keyboard 프로세서 Reset
    kOutPortByte(0x60, 0x00);
}

/**
 *  Scan code를 ASCII 코드로 변환하는 함수
 */
// 키보드 상태 관리 매니저
static KEYBOARDMANAGER gKeyboardManager = {0, };

// Scan code를 ASCII 코드로 변환하는 테이블
static KEYMAPPINGENTRY gKeyMappingTable[KEY_MAPPING_TABLE_MAX_COUNT] = {
    //              NormalCode          CombinedCode
    /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
    /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
    /*  2   */  {   '1'             ,   '!'             },
    /*  3   */  {   '2'             ,   '@'             },
    /*  4   */  {   '3'             ,   '#'             },
    /*  5   */  {   '4'             ,   '$'             },
    /*  6   */  {   '5'             ,   '%'             },
    /*  7   */  {   '6'             ,   '^'             },
    /*  8   */  {   '7'             ,   '&'             },
    /*  9   */  {   '8'             ,   '*'             },
    /*  10  */  {   '9'             ,   '('             },
    /*  11  */  {   '0'             ,   ')'             },
    /*  12  */  {   '-'             ,   '_'             },
    /*  13  */  {   '='             ,   '+'             },
    /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
    /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
    /*  16  */  {   'q'             ,   'Q'             },
    /*  17  */  {   'w'             ,   'W'             },
    /*  18  */  {   'e'             ,   'E'             },
    /*  19  */  {   'r'             ,   'R'             },
    /*  20  */  {   't'             ,   'T'             },
    /*  21  */  {   'y'             ,   'Y'             },
    /*  22  */  {   'u'             ,   'U'             },
    /*  23  */  {   'i'             ,   'I'             },
    /*  24  */  {   'o'             ,   'O'             },
    /*  25  */  {   'p'             ,   'P'             },
    /*  26  */  {   '['             ,   '{'             },
    /*  27  */  {   ']'             ,   '}'             },
    /*  28  */  {   '\n'            ,   '\n'            },
    /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
    /*  30  */  {   'a'             ,   'A'             },
    /*  31  */  {   's'             ,   'S'             },
    /*  32  */  {   'd'             ,   'D'             },
    /*  33  */  {   'f'             ,   'F'             },
    /*  34  */  {   'g'             ,   'G'             },
    /*  35  */  {   'h'             ,   'H'             },
    /*  36  */  {   'j'             ,   'J'             },
    /*  37  */  {   'k'             ,   'K'             },
    /*  38  */  {   'l'             ,   'L'             },
    /*  39  */  {   ';'             ,   ':'             },
    /*  40  */  {   '\''            ,   '\"'            },
    /*  41  */  {   '`'             ,   '~'             },
    /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
    /*  43  */  {   '\\'            ,   '|'             },
    /*  44  */  {   'z'             ,   'Z'             },
    /*  45  */  {   'x'             ,   'X'             },
    /*  46  */  {   'c'             ,   'C'             },
    /*  47  */  {   'v'             ,   'V'             },
    /*  48  */  {   'b'             ,   'B'             },
    /*  49  */  {   'n'             ,   'N'             },
    /*  50  */  {   'm'             ,   'M'             },
    /*  51  */  {   ','             ,   '<'             },
    /*  52  */  {   '.'             ,   '>'             },
    /*  53  */  {   '/'             ,   '?'             },
    /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
    /*  55  */  {   '*'             ,   '*'             },
    /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
    /*  57  */  {   ' '             ,   ' '             },
    /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
    /*  59  */  {   KEY_F1          ,   KEY_F1          },
    /*  60  */  {   KEY_F2          ,   KEY_F2          },
    /*  61  */  {   KEY_F3          ,   KEY_F3          },
    /*  62  */  {   KEY_F4          ,   KEY_F4          },
    /*  63  */  {   KEY_F5          ,   KEY_F5          },
    /*  64  */  {   KEY_F6          ,   KEY_F6          },
    /*  65  */  {   KEY_F7          ,   KEY_F7          },
    /*  66  */  {   KEY_F8          ,   KEY_F8          },
    /*  67  */  {   KEY_F9          ,   KEY_F9          },
    /*  68  */  {   KEY_F10         ,   KEY_F10         },
    /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
    /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },
    /*  71  */  {   KEY_HOME        ,   '7'             },
    /*  72  */  {   KEY_UP          ,   '8'             },
    /*  73  */  {   KEY_PAGEUP      ,   '9'             },
    /*  74  */  {   '-'             ,   '-'             },
    /*  75  */  {   KEY_LEFT        ,   '4'             },
    /*  76  */  {   KEY_CENTER      ,   '5'             },
    /*  77  */  {   KEY_RIGHT       ,   '6'             },
    /*  78  */  {   '+'             ,   '+'             },
    /*  79  */  {   KEY_END         ,   '1'             },
    /*  80  */  {   KEY_DOWN        ,   '2'             },
    /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
    /*  82  */  {   KEY_INS         ,   '0'             },
    /*  83  */  {   KEY_DEL         ,   '.'             },
    /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  87  */  {   KEY_F11         ,   KEY_F11         },
    /*  88  */  {   KEY_F12         ,   KEY_F12         }
};

// 알파벳인지 확인
BOOL kIsAlphabetScanCode(BYTE scanCode) {
    // 변환 테이블을 이용해 확인
    return ('a' <= gKeyMappingTable[scanCode].normalCode &&
        gKeyMappingTable[scanCode].normalCode <= 'z');
}

// 숫자 혹은 기호인지 확인
BOOL kIsNumberOrSymbolScanCode(BYTE scanCode) {
    // 숫자 패드, 확장 키를 제외한 Code 중(2 ~ 53)에서 영문자가 아니면 숫자 or 기호
    return (2 <= scanCode && scanCode <= 53 && kIsAlphabetScanCode(scanCode) == FALSE);
}

// 숫자 패드 범위인지 확인
BOOL kIsNumberPadScanCode(BYTE scanCode) {
    return (71 <= scanCode && scanCode <= 83);
}

// 조합된 키 값을 사용해야 하는지 확인
BOOL kIsUseCombinedCode(BYTE scanCode) {
    BYTE downScanCode;
    BOOL useCombinedKey = FALSE;

    downScanCode = scanCode & 0x7F;

    if(kIsAlphabetScanCode(downScanCode))               // 알파벳이라면 Shift 키와 Caps Lock의 영향을 받음
        useCombinedKey = gKeyboardManager.isShiftDown ^ gKeyboardManager.isCapsLockOn;
    else if(kIsNumberOrSymbolScanCode(downScanCode))    // 숫자 혹은 기호라면 Shift 키의 영향을 받음
        useCombinedKey = gKeyboardManager.isShiftDown;
    else if(kIsNumberPadScanCode(downScanCode)          // 숫자 패드 키라면 Num Lock의 영향을 받음
        && gKeyboardManager.extendedCodeIn == FALSE)    // 0xE0만 제외하면 확장 키 코드와 숫자 패드의 코드가 겹치므로
        useCombinedKey = gKeyboardManager.isNumLockOn;  // 확장 키 코드가 수신되지 않았을 때만 조합 코드 사용

    return useCombinedKey;
}