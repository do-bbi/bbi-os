#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "AssemblyUtil.h"

SHELLCOMMANDENTRY gCommandTable[] = {
    {"help",        "Show Help",                kHelp},
    {"clear",       "Clear Screen",             kClear},
    {"totalram",    "Show Total sizeof RAM",    kShowTotalSizeofRAM},
    {"strtonum",    "Convert Str To Dec/Hex",   kStrToNumTest},
    {"shutdown",    "Shutdown & Reboot OS",     kShutdown},
    {"settimer",    "Set PIT Controller",       kSetTimer},
    {"wait",        "Wait #t ms Using PIT",     kWaitUsingPIT},
    {"rdtsc",       "Read Time Stamp Counter",  kReadTimeStampCounter},
    {"cpuspeed",    "Measyre Processor Speed",  kMeasureProcessorSpeed},
    {"date",        "Show Date & Time",         kShowDateAndTime},
    {"createtask",  "Create Test Task",         kCreateTestTask},
};

void kStartConsoleShell(void) {
    char commandBuf[CONSOLE_SHELL_MAX_COMMANDS_BUFFER_COUNT];
    int commandBufIdx = 0;

    BYTE key;
    int x, y;

    kPrintf(CONSOLE_SHELL_PROMPT_MESSAGE);

    while(TRUE) {
        // Wait Until Key Pressed
        key = kGetCh();

        switch(key) {
            case KEY_BACKSPACE:
                if(0 < commandBufIdx) {
                    // 현재 커서 위치 한 칸 앞에 공백을 출력해 지우고, 커맨드 버퍼에서도 마지막 문자를 삭제
                    kGetCursor(&x, &y);
                    x--;
                    
                    kPrintStringXY(x, y, " ");
                    kSetCursor(x, y);
                    commandBufIdx--;
                }

                break;

            case KEY_ENTER:
                kPrintf("\n");

                if(0 < commandBufIdx) {
                    // 커맨드 버퍼에 있는 명령 실행
                    commandBuf[commandBufIdx] = '\0';
                    kExecuteCommand(commandBuf);
                }

                // 프롬프트 메세지 출력 + 커맨드 버퍼 초기화
                kPrintf("%s", CONSOLE_SHELL_PROMPT_MESSAGE);
                kMemSet(commandBuf, '\0', CONSOLE_SHELL_MAX_COMMANDS_BUFFER_COUNT);
                commandBufIdx = 0;

                break;
            
            case KEY_LSHIFT:
            case KEY_RSHIFT:
            case KEY_CAPSLOCK:
            case KEY_NUMLOCK:
            case KEY_SCROLLLOCK:
                break;

            case KEY_TAB:
                key = ' ';
            default:
                // 버퍼에 공간이 남아 있는 경우
                // @TEST kPrintf("%d %d\n", commandBufIdx, CONSOLE_SHELL_MAX_COMMANDS_BUFFER_COUNT);
                if(commandBufIdx < CONSOLE_SHELL_MAX_COMMANDS_BUFFER_COUNT) {
                    commandBuf[commandBufIdx++] = key;
                    kPrintf("%c", key);
                }

        }
    }
}

void kExecuteCommand(const char *pCommandBuf) {
    int i, cmdIdx;
    int len, cmdLen;

    int cnt;

    len = kStrLen(pCommandBuf);
    cmdIdx = 0;
    while(cmdIdx < len) {
        if(pCommandBuf[cmdIdx] == ' ')
            break;
        ++cmdIdx;
    }

    cnt = sizeof(gCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for(i = 0; i < cnt; ++i) {
        cmdLen = kStrLen(gCommandTable[i].pCommand);

        // 커맨드의 길이와 내용이 일치하는지 확인
        if(cmdLen == cmdIdx && kMemCmp(gCommandTable[i].pCommand, pCommandBuf, cmdIdx) == 0) {
            gCommandTable[i].pFunc(pCommandBuf + cmdIdx + 1);
            break;
        }
    }

    if(cnt <= i)
        kPrintf("'%s' is not founc\n", pCommandBuf);
}

void kInitializeParam(PARAMLIST *pList, const char *pParam) {
    pList->pBuf = pParam;
    pList->len = kStrLen(pParam);
    pList->cur = 0;
}

int kGetNextParameter(PARAMLIST *pList, char *pParam) {
    int i, len;

    if(pList->len <= pList->cur)
        return 0;

    for(i = pList->cur; i < pList->len; ++i) 
        if(pList->pBuf[i] == ' ')
            break;

    // Copy Parameter
    kMemCpy(pParam, pList->pBuf + pList->cur, i);

    len = i - pList->cur;
    pParam[len] = '\0';

    // Update Parameter Current Position
    pList->cur += len + 1;

    return len;
}

// Functions for COMMAND
void kHelp(const char *pParamBuf) {
    int i, x, y, cnt;
    int cmdLen, maxLen = 0;

    kPrintf("==================================================\n");
    kPrintf("                BBI OS Shell Help                 \n");
    kPrintf("==================================================\n");

    cnt = sizeof(gCommandTable) / sizeof(SHELLCOMMANDENTRY);

    // Find the Longest Length of Commands
    for(i = 0; i < cnt; ++i) {
        cmdLen = kStrLen(gCommandTable[i].pCommand);
        if(maxLen < cmdLen)
            maxLen = cmdLen;
    }

    // Print HELP
    for(i = 0; i < cnt; ++i) {
        kPrintf("%s", gCommandTable[i].pCommand);
        kGetCursor(&x, &y);
        kSetCursor(maxLen, y);
        kPrintf(" - %s\n", gCommandTable[i].pHelp);
    }
}

void kClear(const char *pParamBuf) {
    // Use 1st line for debugging
    kClearScreen();
    kSetCursor(0, 1);
}

void kShowTotalSizeofRAM(const char *pParamBuf) {
    kPrintf("Total sizeof RAM = %d MB\n", kGetTotalRAMSize());
}

void kStrToNumTest(const char *pParamBuf) {
    char param[100];
    int len, cnt = 0;;

    PARAMLIST list;
    long value;

    kInitializeParam(&list, pParamBuf);

    while(TRUE) {
        len = kGetNextParameter(&list, param);
        if(len == 0)
            break;

        kPrintf("Param %d = '%s', Length = %d ", cnt + 1, param, len);

        if(kMemCmp(param, "0x", 2) == 0) {
            value = kAtoI(param + 2, 16);
            kPrintf("Hex value = %d\n", value);
        }
        else {
            value = kAtoI(param, 10);
            kPrintf("Dec value = %d\n", value);
        }

        ++cnt;
    }
}

void kShutdown(const char *pParamBuf) {
    kPrintf("System Shutdown Start...\n");

    kPrintf("Press any key to reboot");
    kGetCh();
    kReboot();
}

// PIT 컨트롤러의 카운터를 0으로 설정
void kSetTimer(const char *pParamBuf) {
    char params[100];
    PARAMLIST list;
    long value;
    BOOL periodic;

    // Init Params
    kInitializeParam(&list, pParamBuf);

    // Get ms
    if(kGetNextParameter(&list, params) == 0) {
        kPrintf("Ex) settimer 10(ms) 1(periodic)\n");
        return;
    }
    value = kAtoI(params, 10);

    // Get periodic
    if(kGetNextParameter(&list, params) == 0) {
        kPrintf("Ex) settimer 10(ms) 1(periodic)\n");
        return;
    }
    periodic = kAtoI(params, 10);

    kInitializePIT(value, periodic);
    kPrintf("Time = %d ms, Periodic = %s Change Complete\n", value, periodic ? "TRUE" : "FALSE");
}

// PIT 컨트롤러를 직접 사용하여 #t ms 동안 대기
void kWaitUsingPIT(const char *pParamBuf) {
    char params[100];
    PARAMLIST list;
    long ms;
    int len, i;

    // Init Params
    kInitializeParam(&list, pParamBuf);

    // Get ms
    if(kGetNextParameter(&list, params) == 0) {
        kPrintf("Ex) wait 100(ms)\n");
        return;
    }
    ms = kAtoI(params, 10);
    kPrintf("Sleep %d ms\n", ms);

    // 인터럽트를 비활성화, PIT 컨트롤러를 이용해 시간 측정
    kDisableInterrupt();
    
    for(i = 0; i < ms / 30; ++i)
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    kWaitUsingDirectPIT(MSTOCOUNT(ms % 30));

    kEnableInterrupt();

    kPrintf("Sleep %d ms compelte\n", ms);

    // Restore Timer
    kInitializePIT(MSTOCOUNT(1), TRUE);
}

// Read Time Stamp Counter
void kReadTimeStampCounter(const char *pParamBuf) {
    QWORD tsc;

    tsc = kReadTSC();

    kPrintf("Value of Time Stamp Counter = %q\n", tsc);
}

// Measure Speed of Processor
void kMeasureProcessorSpeed(const char *pParamBuf) {
    int i;
    QWORD lastTSC, totalTSC = 0;

    kPrintf("Measuring...");

    // 10초 동안 변화한 타임 스탬프 카운터를 이용하여 프로세서 속도를 간접적으로 측정
    kDisableInterrupt();

    for(i = 0; i < 200; ++i) {
        lastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        totalTSC += kReadTSC() - lastTSC;

        kPrintf(".");
    }

    // Restore PIT
    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();

    kPrintf("\nCPU Speed = %d MHz\n", totalTSC / 10 / 1000 / 1000);
}

void kShowDateAndTime(const char *pParamBuf) {
    BYTE ss, mm, hh;
    BYTE dayOfWeek, dayOfMonth, month;
    WORD year;

    kReadRTCTime(&hh, &mm, &ss);
    kReadRTCDate(&year, &month, &dayOfMonth, &dayOfWeek);

    kPrintf("Date: %d/%d/%d %s, ", year, month, dayOfMonth, kConvertDayOfWeekToString(dayOfWeek));
    kPrintf("Time: %d:%d:%d\n", hh, mm, ss);
}

static TCB      gTasks[2] = {0, };
static QWORD    gStacks[1024] = {0, };

// Task switching Task
void kTestTask(void) {
    int i = 0;

    while(TRUE) {
        // Print Message & Wait For Keystroke
        kPrintf("[%d] This message is from kConsoleShell, "
                "Press any key to switch kTestTask\n", i++);
        kGetCh();

        // If Key Input Occurs, Switch Task
        kSwitchContext(&(gTasks[1].context), &(gTasks[0].context));
    }
}

void kCreateTestTask(const char *pParamBuf) {
    KEYDATA data;
    int i = 0;

    // Setup Task
    kSetUpTask(&gTasks[1], 1, 0, (QWORD)kTestTask, &gStacks, sizeof(gStacks));

    // Run until q key is input
    while (TRUE) {
        // Print Message & Wait For Keystroke
        kPrintf("[%d] This message is from kTestTask, "
                "Press any key to switch kConsoleShell\n", i++);
        
        if(kGetCh() == 'q')
            break;

        // If Key Input is not 'q', Switch Task
        kSwitchContext(&(gTasks[0].context), &(gTasks[1].context));
    }
}