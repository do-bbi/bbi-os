#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

// MACRO
// PAUSE 키 처리를 위해 무시해야 할 스캔 코드의 수
#define KEY_SKIP_COUNT_FOR_PAUSE    (2)

// 키 상태 플래그
#define KEY_FLAGS_UP                (0x00)
#define KEY_FLAGS_DOWN              (0x01)
#define KEY_FLAGS_EXTENDED          (0x02)

// 스캔 코드 맵핑 테이블
#define KEY_MAPPING_TABLE_MAX_COUNT (89)

#define KEY_NONE                    (0x00)
#define KEY_ENTER                   ('\n')
#define KEY_TAB                     ('\t')
#define KEY_ESC                     (0x1B)
#define KEY_BACKSPACE               (0x08)
#define KEY_CTRL                    (0x81)
#define KEY_LSHIFT                  (0x82)
#define KEY_RSHIFT                  (0x83)
#define KEY_PRINTSCREEN             (0x84)
#define KEY_LALT                    (0x85)
#define KEY_CAPSLOCK                (0x86)
#define KEY_F1                      (0x87)
#define KEY_F2                      (0x88)
#define KEY_F3                      (0x89)
#define KEY_F4                      (0x8A)
#define KEY_F5                      (0x8B)
#define KEY_F6                      (0x8C)
#define KEY_F7                      (0x8D)
#define KEY_F8                      (0x8E)
#define KEY_F9                      (0x8F)
#define KEY_F10                     (0x90)
#define KEY_NUMLOCK                 (0x91)
#define KEY_SCROLLLOCK              (0x92)
#define KEY_HOME                    (0x93)
#define KEY_UP                      (0x94)
#define KEY_PAGEUP                  (0x95)
#define KEY_LEFT                    (0x96)
#define KEY_CENTER                  (0x97)
#define KEY_RIGHT                   (0x98)
#define KEY_END                     (0x99)
#define KEY_DOWN                    (0x9A)
#define KEY_PAGEDOWN                (0x9B)
#define KEY_INS                     (0x9C)
#define KEY_DEL                     (0x9D)
#define KEY_F11                     (0x9E)
#define KEY_F12                     (0x9F)
#define KEY_PAUSE                   (0xA0)

#define KEY_MAX_QUEUE_COUNT         (100)   // Max count of key-queue

#pragma pack( push, 1 )

// 스캔 코드 테이블을 구성하는 항목
typedef struct kKeyMappingEntryStruct
{
    // Shift 키나 Caps Lock 키와 조합되지 않는 ASCII 코드
    BYTE normalCode;
    
    // Shift 키나 Caps Lock 키와 조합된 ASCII 코드
    BYTE combinedCode;
} KEYMAPPINGENTRY;

#pragma pack( pop )

// 키보드의 상태를 관리하는 자료구조
typedef struct kKeyboardManagerStruct
{
    // 조합 키 정보
    BOOL isShiftDown;
    BOOL isCapsLockOn;
    BOOL isNumLockOn;
    BOOL isScrollLockOn;
    
    // 확장 키를 처리하기 위한 정보
    BOOL isExtendedCodeIn;
    int skipCountForPause;
} KEYBOARDMANAGER;

typedef struct kKeyDataStruct {
    BYTE scanCode;  // 키보드에서 전달된 스캔 코드
    BYTE asciiCode; // 스캔 코드를 변환한 ASCII 코드
    BYTE flags;     // 키 상태를 저장하는 플래그(key down / key up / expansion key)
} KEYDATA;

BOOL kIsOutputBufferFull(void);
BOOL kIsInputBufferFull(void);
BOOL kActivateKeyboard(void);
BYTE kGetKeyboardScanCode(void);
BOOL kChangeKeyboardLED(BOOL isCapsLockOn, BOOL isNumLockOn, BOOL isScrollLockOn);
void kEnableA20Gate(void);
void kReboot(void);
BOOL kIsAlphabetScanCode(BYTE isScanCode);
BOOL kIsNumberOrSymbolScanCode(BYTE isScanCode);
BOOL kIsNumberPadScanCode(BYTE isScanCode);
BOOL kIsUseCombinedCode(BOOL isScanCode);
void UpdateCombinationKeyStatusAndLED(BYTE isScanCode);
BOOL kConvertScanCodeToASCIICode(BYTE isScanCode, BYTE *pASCIICode, BOOL *pFlags);
BOOL kInitializeKeyboard(void);
BOOL kConvertScanCodeAndPutQueue(BYTE scanCode);
BOOL kGetKeyFromKeyQueue(KEYDATA *pData);
BOOL kWaitForACKAndPutOtherScanCode(void);

#endif // __KEYBOARD_H__