#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
// #include "AssemblyUtil.h"
// #include "Utility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"

#define VIDEO_MEM_ADDR  (0xB8000)

#define PRINT_BLANK_POS  (45)

void kPrintString(int iX, int iY, const char* pcString);

// C 언어 커널
void Main(void) {
    char temp[2] = {0, };
    BYTE tmp, flags;
    int i = 0;
    KEYDATA data;
    int x, y;

    /**************************************************************************/
    int posY = 10;
    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;

    kInitializeConsole(0, posY);
    kPrintf("IA-32e C Language Kernel Start\n"), posY++;
    
    /**************************************************************************/
    kPrintf("GDT Initialize And Switch For IA-32e Mode...[    ]");
    kInitializeGDTableAndTSS();
    kLoadGDTR(GDTR_BASE_ADDR);

    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;
    
    /**************************************************************************/
    kPrintf("TSS Segment Load............................[    ]");
    kLoadTR(GDT_TS_SEGMENT);

    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;
    
    /**************************************************************************/
    kPrintf("IDT Initialize..............................[    ]");
    kInitializeIDTables();
    kLoadIDTR(IDTR_BASE_ADDR);

    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;

    /**************************************************************************/
    kPrintf("Check Total size of RAM.....................[    ]");
    kCheckTotalSizeofRAM();
    
    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;

    kPrintf("Total sizeof RAM = %d MB\n", kGetTotalRAMSize()), posY++;

    /**************************************************************************/
    kPrintf("TCB Pool And Scheduler Initialize...........[    ]");
    kInitializeScheduler();
    
    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;

    // Set Interrupt to Occur Once per 1ms
    kInitializePIT(MSTOCOUNT(1), 1);
    
    /**************************************************************************/
    kPrintf("Keyboard Activate And Queue Initialize......[    ]");

    if(kInitializeKeyboard()) {
        kSetCursor(PRINT_BLANK_POS, posY);
        kPrintf("PASS\n"), posY++;
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kSetCursor(PRINT_BLANK_POS, posY);
        kPrintf("FAIL\n"), posY++;
        while(TRUE);
    }
    
    /**************************************************************************/
    kPrintf("PIC Activate & Initialize Interrupt.........[    ]");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    
    kSetCursor(PRINT_BLANK_POS, posY);
    kPrintf("PASS\n"), posY++;

    kCreateTask(TASK_PRIORITY_LOWEST | TASK_FLAGS_IDLE, (QWORD)kIdleTask);
    kStartConsoleShell();

    // while(TRUE) {
    //     // 출력 버퍼(Port 0x60)가 차 있으면 Scan code를 읽을 수 있음
    //     if(kGetKeyFromKeyQueue(&data)) {
            
    //         // If key down, print ascii to screen
    //         if(data.flags & KEY_FLAGS_DOWN) {
    //             // Save ASCII data
    //             temp[0] = data.asciiCode;
    //             kPrintString(i++, posY, temp);

    //             // 0이 입력되면 변수를 0으로 나누어 Divide Error Exception(Vector 0) 발생시켜 임시 핸들러 실행
    //             if(temp[0] == '0')
    //                 tmp /= 0;
    //         }
    //     }
    // }
}

void kPrintString(int iX, int iY, const char *pcString) {
    VGATEXT *pstScreen = (VGATEXT *)VIDEO_MEM_ADDR;    // Video Memory Addr
    int i;

    pstScreen += (iY * 80) + iX;
    for(i = 0; pcString[i]; ++i)
        pstScreen[i].ch = pcString[i];
}