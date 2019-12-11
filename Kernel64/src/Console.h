#ifndef __CONSOLE__H__
#define __CONSOLE__H__

#include "Type.h"

// Video Memory Property
#define CONSOLE_BACKGROUND_BLACK          (0x00)
#define CONSOLE_BACKGROUND_BLUE           (0x10)
#define CONSOLE_BACKGROUND_GREEN          (0x20)
#define CONSOLE_BACKGROUND_CYAN           (0x30)
#define CONSOLE_BACKGROUND_RED            (0x40)
#define CONSOLE_BACKGROUND_MAGENTA        (0x50)
#define CONSOLE_BACKGROUND_BROWN          (0x60)
#define CONSOLE_BACKGROUND_WHITE          (0x70)
#define CONSOLE_BACKGROUND_BLINK          (0x80)
#define CONSOLE_FOREGROUND_DARKBLACK      (0x00)
#define CONSOLE_FOREGROUND_DARKBLUE       (0x01)
#define CONSOLE_FOREGROUND_DARKGREEN      (0x02)
#define CONSOLE_FOREGROUND_DARKCYAN       (0x03)
#define CONSOLE_FOREGROUND_DARKRED        (0x04)
#define CONSOLE_FOREGROUND_DARKMAGENTA    (0x05)
#define CONSOLE_FOREGROUND_DARKBROWN      (0x06)
#define CONSOLE_FOREGROUND_DARKWHITE      (0x07)
#define CONSOLE_FOREGROUND_BRIGHTBLACK    (0x08)
#define CONSOLE_FOREGROUND_BRIGHTBLUE     (0x09)
#define CONSOLE_FOREGROUND_BRIGHTGREEN    (0x0A)
#define CONSOLE_FOREGROUND_BRIGHTCYAN     (0x0B)
#define CONSOLE_FOREGROUND_BRIGHTRED      (0x0C)
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA  (0x0D)
#define CONSOLE_FOREGROUND_BRIGHTBROWN    (0x0E)
#define CONSOLE_FOREGROUND_BRIGHTWHITE    (0x0F)

#define CONSOLE_DEFAULT_TEXTCOLOR         (CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTGREEN)

#define CONSOLE_WIDTH           (80)
#define CONSOLE_HEIGHT          (25)
#define CONSOLE_VIDEO_MEM_ADDR  (0xB8000)

// Video Controller I/O Port Address & Register
#define VGA_PORT_IDX            (0x3D4)
#define VGA_PORT_DATA           (0x3D4)
#define VGA_INDEX_UPPERCURSOR   (0x0E)
#define VGA_INDEX_LOWERCURSOR   (0x0F)

#pragma pack(push, 1)

typedef struct kConsoleManagerStruct {
  int currentPrintOffset;
} CONSOLEMANAGER;

#pragma pack(pop);

void kInitializeConsole(int x, int y);
void kSetCursor(int x, int y);
void kGetCursor(int *pX, int *pY);
void kPrintf(const char *pFormatStr, ...);
int kConsolePrintString(const char *pBuf);
void kClearScreen(void);
BYTE kGetCh(void);
void kPrintStringXY(int x, int y, const char *pStr);

#endif  // __CONSOLE__H__