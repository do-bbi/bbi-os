#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "AssemblyUtil.h"
#include "Sync.h"
#include "BuddyMemory.h"
#include "HDD.h"

SHELLCOMMANDENTRY gCommandTable[] = {
    {"help",            "Show Help",                    kHelp},
    {"clear",           "Clear Screen",                 kClear},
    {"totalram",        "Show Total sizeof RAM",        kShowTotalSizeofRAM},
    {"strtonum",        "Convert Str To Dec/Hex",       kStrToNumTest},
    {"shutdown",        "Shutdown & Reboot OS",         kShutdown},
    {"settimer",        "Set PIT Controller(< 56ms)",   kSetTimer},
    {"wait",            "Wait #t ms Using PIT",         kWaitUsingPIT},
    {"rdtsc",           "Read Time Stamp Counter",      kReadTimeStampCounter},
    {"cpuspeed",        "Measyre Processor Speed",      kMeasureProcessorSpeed},
    {"date",            "Show Date & Time",             kShowDateAndTime},
    {"createtask",      "Create Test Task",             kCreateTestTask},
    {"changepriority",  "Change Task Priority, ex) "
                        "changepriority $ID $PR(0~4)",  kChangeTaskPriority},
    {"tasklist",        "Show Task List",               kShowTaskList},
    {"killtask",        "End Task, ex) killtask $ID "
                        "or -1(All)",                   kKillTask},
    {"cpuload",         "Show Processor Load",          kCPULoad},
    {"testmutex",       "Test Mutex Function",          kTestMutex},
    {"testthread",      "Test Thread & Process Func",   kTestThread},
    {"showmatrix",      "Show Matrix Screen",           kShowMatrix},
    {"testpie",         "Test PIE Calculation",         kTestPIE},
    {"buddymeminfo",    "Show Buddy Memory Info",       kShowBuddyMemInfo},
    {"testseqalloc",    "Test Sequential Alloc & Free", kTestSeqAlloc},
    {"testrandalloc",   "Test Random Alloc & Free",     kTestRandAlloc},
    {"showhddinfo",     "Show HDD Info",                kShowHDDInfo},
    {"readsector",      "Read HDD Sector, ex) "
                        "readsector 0(LBA) 10(count)",  kReadSector},
    {"writesector",     "Write HDD Sector, ex) "
                        "writesector 0(LBA) 10(count)", kWriteSector}
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
static void kHelp(const char *pParamBuf) {
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

        // 목록이 많을 경우 나눠 출력
        if( (i != 0) && ((i % 20) == 0) ) {
            kPrintf("Press any key to continue... ('q' is exit) : ");
            if(kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
            kPrintf("\n");
        }
    }
}

static void kClear(const char *pParamBuf) {
    // Use 1st line for debugging
    kClearScreen();
    kSetCursor(0, 1);
}

static void kShowTotalSizeofRAM(const char *pParamBuf) {
    kPrintf("Total sizeof RAM = %d MB\n", kGetTotalRAMSize());
}

static void kStrToNumTest(const char *pParamBuf) {
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

static void kShutdown(const char *pParamBuf) {
    kPrintf("System Shutdown Start...\n");

    kPrintf("Press any key to reboot");
    kGetCh();
    kReboot();
}

// PIT 컨트롤러의 카운터를 0으로 설정
static void kSetTimer(const char *pParamBuf) {
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

    kInitializePIT(MSTOCOUNT(value), periodic);
    kPrintf("Time = %d ms, Periodic = %s Change Complete\n", value, periodic ? "TRUE" : "FALSE");
}

// PIT 컨트롤러를 직접 사용하여 #t ms 동안 대기
static void kWaitUsingPIT(const char *pParamBuf) {
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
static void kReadTimeStampCounter(const char *pParamBuf) {
    QWORD tsc;

    tsc = kReadTSC();

    kPrintf("Value of Time Stamp Counter = %q\n", tsc);
}

// Measure Speed of Processor
static void kMeasureProcessorSpeed(const char *pParamBuf) {
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

static void kShowDateAndTime(const char *pParamBuf) {
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
static void kTestTask(void) {
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

// Task 1 - Print Texts while Rotating the Screen Border
static void kTestTask1(void) {
    BYTE data;
    int i = 0, count, cursorX = 0, cursorY = 0, margin;
    VGATEXT *pScreen = (VGATEXT *)CONSOLE_VIDEO_MEM_ADDR;

    TCB *pRunningTask;

    // Get Running Task ID to Use Screen Offset
    pRunningTask = kGetRunningTask();
    margin = (pRunningTask->link.id & 0xFFFFFFFF) % 10;

    // Print Texts while Rotating the Screen Border
    count = 20000;
    while(count--) {
        switch (i)
        {
        case 0:
            cursorX++;
            if((CONSOLE_WIDTH - margin) <= cursorX)
                i = 1;
            break;
        case 1:
            cursorY++;
            if((CONSOLE_HEIGHT - margin) <= cursorY)
                i = 2;
            break;
        case 2:
            cursorX--;
            if(cursorX < margin)
                i = 3;
            break;
        case 3:
            cursorY--;
            if(cursorY < margin)
                i = 0;
            break;
        
        default:
            break;
        }

        // Set Character & Font Color
        pScreen[cursorY * CONSOLE_WIDTH + cursorX].ch = data;
        pScreen[cursorY * CONSOLE_WIDTH + cursorX].attr = data & 0x0F;
        data++;

        // Context Switching
        // kSchedule();
    }

    kExitTask();
}

// Task 2 - Print a Rotating Pinwheel at a Specific Location with referring to Task ID
static void kTestTask2(void) {
    int i = 0, count, offset;
    VGATEXT * const pScreen = (VGATEXT *)CONSOLE_VIDEO_MEM_ADDR;

    TCB *pRunningTask;
    static char pinwheel[4] = {'-', '\\', '|', '/'};

    // Get Running Task ID to Use Screen Offset
    pRunningTask = kGetRunningTask();
    offset = (pRunningTask->link.id & 0xFFFFFFFF) << 1;
    offset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));
    // offset = 1 + (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    // Print Texts while Rotating the Screen Border
    while(TRUE) {
        pScreen[offset].ch = pinwheel[i % 4];
        pScreen[offset].attr = (offset % 15) + 1;
        i++;

        // kSchedule();
    }
}

static void kCreateTestTask(const char *pParamBuf) {
    PARAMLIST list;
    char types[30];
    char counts[30];
    int i = 0;

    // Get Params
    kInitializeParam(&list, pParamBuf);
    kGetNextParameter(&list, types);
    kGetNextParameter(&list, counts);

    switch (kAtoI(types, 10))
    {
    // Create Task 1
    case 1:
        for(i = 0; i < kAtoI(counts, 10); ++i) {
            if(kCreateTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL)
                break;
        }

        kPrintf("Task1 %d Created\n", i);
        break;
    
    // Create Task 2
    case 2:
    default:
        for(i = 0; i < kAtoI(counts, 10); ++i) {
            if(kCreateTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2) == NULL)
                break;
        }
        
        kPrintf("Task2 %d Created\n", i);
        break;
    }
}

// Change Task Priority
static void kChangeTaskPriority(const char *pParamBuf) {
    PARAMLIST list;
    char ids[30];
    char priorities[3];
    
    QWORD id;
    BYTE priority;
    BOOL result;

    kInitializeParam(&list, pParamBuf);
    kGetNextParameter(&list, ids);
    kGetNextParameter(&list, priorities);

    if(kMemCmp(ids, "0x", 2) == 0) 
        id = kAtoI(ids + 2, 16);
    else
        id = kAtoI(ids, 10);

    priority = kAtoI(priorities, 10);

    result = kChangePriority(id, priority);
    kPrintf("Change Task Priority ID [0x%q] Priority[%d] %s\n", id, priority, result ? "Success" : "Failed");
}

// Show Task List
static void kShowTaskList(const char *pParamBuf) {
    int i;
    TCB *pTCB;
    int count = 0;

    kPrintf("=========== Task Total Count [%03d] ===========\n", kGetTaskCount());
    for(i = 0; i < TASK_MAX_COUNT; ++i) {
        pTCB = kGetTCBInTCBPool(i);
        if((pTCB->link.id >> 32)) {
            if(count && (count % 10) == 0) {
                kPrintf("Press any key to continue... ('q' is exit) : ");
                if(kGetCh() == 'q') {
                    kPrintf("\n");
                    break;
                }
                kPrintf("\n");
            }

            kPrintf("[%d] Task ID[0x%Q]-Priority[%d], Flags[0x%Q], Thread[%d]\n", 
                    ++count, pTCB->link.id, GETPRIORITY(pTCB->flags), pTCB->flags,
                    kGetListCount(&pTCB->childThreads));
            kPrintf("Parent PID[0x%Q]-Memory Addr[0x%Q], Size[0x%Q]\n", 
                    pTCB->parentPid, pTCB->pMemoryAddr, pTCB->memorySize);
        }
    }
}

// Kill Task
static void kKillTask(const char *pParamBuf) {
    PARAMLIST list;
    char ids[30];
    QWORD id;
    TCB *pTCB;
    int i;

    kInitializeParam(&list, pParamBuf);
    kGetNextParameter(&list, ids);

    if(kMemCmp(ids, "0x", 2) == 0)
        id = kAtoI(ids + 2, 16);
    else
        id = kAtoI(ids, 10);
    
    if(id != -1) {
        pTCB = kGetTCBInTCBPool(GETTCBOFFSET(id));
        id = pTCB->link.id;

        if((id >> 32) && (pTCB->flags & TASK_FLAGS_SYSTEM) == 0x00) {
            kPrintf("Kill Task [0x%q]\n", id);
            if(kEndTask(id))
                kPrintf("Success\n");
            else
                kPrintf("Fail\n");
        }
        else
            kPrintf("Task doesn't exist or task is system task\n");
    }
    else {
        for(i = 0; i < TASK_MAX_COUNT; ++i) {
            pTCB = kGetTCBInTCBPool(i);
            id = pTCB->link.id;
            if((id >> 32) && (pTCB->flags & TASK_FLAGS_SYSTEM) == 0x00) {
                kPrintf("Kill Task ID [0x%q]", id);
                if(kEndTask(id))
                    kPrintf("Success\n");
                else
                    kPrintf("Fail\n");
            }
        }
    }
}

// Show CPU Usage Rate
static void kCPULoad(const char *pParamBuf) {
    kPrintf("Current Processor Load = %d%%\n", kGetProcessorLoad());
}

static MUTEX gMutex;
static volatile QWORD gAdder = 1;
static volatile QWORD gRandNum = 0;

QWORD kRand(void) {
    gRandNum = (gRandNum * 412153 + 5571031) >> 16;
    return gRandNum;
}

static void kPrintNumberTask(void) {
    int i, j;
    QWORD waitCnt;

    // 50ms 정도 대기해서 콘솔이 출력하는 메시지가 안 겹치도록 양보
    waitCnt = kGetTickCount() + 50;
    while(kGetTickCount() < waitCnt)
        kSchedule();

    for(i = 0 ; i < 5; ++i) {
        kLock(&gMutex);

        kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->link.id, gAdder);
        gAdder++;

        kUnlock(&gMutex);

        // Stall Processor
        for(j = 0; j < 3000; ++j);
    }

    // 1000ms 정도 대기해서 콘솔이 출력하는 메시지가 안 겹치도록 양보
    waitCnt = kGetTickCount() + 1000;
    while(kGetTickCount() < waitCnt)
        kSchedule();

    kExitTask();
}

static void kTestMutex(const char *pParamBuf) {
    int i;
    gAdder = 1;

    kInitializeMutex(&gMutex);

    for(i = 0; i < 3; ++i)
        kCreateTask(TASK_FLAGS_IDLE | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumberTask);
    
    kPrintf("Wait Util %d Task End...\n", i);
    kGetCh();
}

void kCreateThreadTask(const char *pParamBuf) {
    int i = 0;

    for(i = 0; i < 3; ++i)
        kCreateTask(TASK_FLAGS_IDLE | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2);

    while(TRUE)
        kSleep(1);
}

void kTestThread(const char *pParamBuf) {
    TCB *pProcess;

    pProcess = kCreateTask(TASK_FLAGS_IDLE | TASK_FLAGS_PROCESS, 
                            (void *)0xEEEEEEEE, 0x1000, (QWORD)kCreateThreadTask);
    
    if(pProcess)
        kPrintf("Process Create Success [0x%Q]\n", pProcess->link.id);
    else
        kPrintf("Process Create Failed\n");
}

// void kCreateTestTask(const char *pParamBuf) {
//     KEYDATA data;
//     int i = 0;

//     // Setup Task
//     kSetUpTask(&gTasks[1], 1, 0, (QWORD)kTestTask, &gStacks, sizeof(gStacks));

//     // Run until q key is input
//     while (TRUE) {
//         // Print Message & Wait For Keystroke
//         kPrintf("[%d] This message is from kTestTask, "
//                 "Press any key to switch kConsoleShell\n", i++);
        
//         if(kGetCh() == 'q')
//             break;

//         // If Key Input is not 'q', Switch Task
//         kSwitchContext(&(gTasks[0].context), &(gTasks[1].context));
//     }
// }

// 철자를 흘러내리게 하는 스레드
static void kDropCharactorThread(void) {
    volatile int i, x;
    char text[2] = {0, };

    x = kRand() % CONSOLE_WIDTH;


    while(TRUE) {
        // Wait mement
        kSleep(kRand() % 20);
        if((kRand() % 20) < 15) {
            text[0] = ' ';
            for(i = 0; i < CONSOLE_HEIGHT - 1; ++i) {
                kPrintStringXY(x, i, text);
                kSleep(50);
            }
        }
        else {
            for(i = 0; i < CONSOLE_HEIGHT - 1; ++i) {
                text[0] = i + kRand();
                kPrintStringXY(x, i, text);
                kSleep(50);
            }
        }
    }
}

// 스레드를 생성하여 매트릭스 화면처럼 보여주는 프로세스
static void kMatrixProcess(void) {
    int i;

    for(i = 0; i < 300; ++i) {
        if(kCreateTask(TASK_FLAGS_THREAD | TASK_PRIORITY_LOW, 0, 0, (QWORD)kDropCharactorThread) == NULL)
            break;
        kSleep(kRand() % 5 + 5);
    }

    kPrintf("%d Thread is created\n", i);

    // 키 입력시 프로세스 종료
    kGetCh();
}

static void kShowMatrix(const char *pParamBuf) {
    TCB *pProcess;

    pProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_PRIORITY_LOW, (void *)0xE00000, 0xE00000, (QWORD)kMatrixProcess);

    if(pProcess) {
        kPrintf("Create Process Success - [0x%Q]\n", pProcess->link.id);

        // 태스크 종료까지 대기
        while(pProcess->link.id >> 32)
            kSleep(100);
    }
    else
        kPrintf("Create Process Failed\n");
}

// Test FPU
static void kFPUTestTask(void) {
    double val1, val2;
    TCB *pRunningTask;

    QWORD randVal, cnt = 0;

    int i, offset;
    char datas[4] = {'-', '\\', '|', '/'};
    VGATEXT *pScreen = (VGATEXT *)CONSOLE_VIDEO_MEM_ADDR;

    pRunningTask = kGetRunningTask();

    // Task id를 얻어 화면 offset으로 사용
    offset = (pRunningTask->link.id & 0xFFFFFFFF) << 1;
    offset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(TRUE) {
        val1 = 1.0;
        val2 = 1.0;

        for(i = 0; i < 10; ++i) {
            randVal = kRand();
            val1 *= (double)randVal;
            val2 *= (double)randVal;

            kSleep(1);

            randVal = kRand();
            val1 /= (double)randVal;
            val2 /= (double)randVal;
        }

        if(val1 != val2) {
            kPrintf("FPU seems wrong, [%f] != [%f]\n", val1, val2);
            break;
        }

        pScreen[offset].ch = datas[cnt++ % 4];
        pScreen[offset].attr = (offset % 15) + 1;
    }
}

static void kTestPIE(const char *pParmBuf) {
    double ret;
    int i;

    volatile int A = 355;
    volatile int B = 113;

    kPrintf("PIE Calculation Test\n");
    kPrintf("Result: 355 / 113 = ");

    ret = (volatile double)A / (volatile double)B;

    // kPrintf("%d.%d%d\n", (QWORD)ret, ((QWORD)(ret * 10) % 10), ((QWORD)(ret * 100) % 10));
    kPrintf("%f\n", ret);

    for(i = 0; i < 100; ++i)
        kCreateTask(TASK_FLAGS_IDLE | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask);
}

static void kShowBuddyMemInfo(const char *pParmBuf) {
    QWORD startAddr, totalSize, metaSize, usedSize;

    kGetBuddyMemoryInfo(&startAddr, &totalSize, &metaSize, &usedSize);

    kPrintf("============== Buddy Memory Info ==============\n");
    kPrintf("Start Addr: [0x%Q]\n", startAddr);
    kPrintf("Total Size: [0x%Q]byte [%d]MB\n", totalSize, totalSize / 1024 / 1024);
    kPrintf("Meta Size:  [0x%Q]byte [%d]KB\n", metaSize, metaSize / 1024);
    kPrintf("Used Size:  [0x%Q]byte [%d]KB\n", usedSize, usedSize / 1024);
}

static void kTestSeqAlloc(const char *pParmBuf) {
    BUDDY_MEMORY *pMemory;
    long i, j, k;
    QWORD *pBuffer;

    kPrintf("============== Buddy Memory Test ==============\n");
    pMemory = kGetBuddyMemoryManager();
    kPrintf("pMemory=[0x%Q]\n", pMemory);

    for(i = 0; i < pMemory->maxLevelCnt; ++i) {
        kPrintf("Block List[%d] Test Start\n", i);
        kPrintf("Alloc & Compare: \n");

        // 모든 블록을 할당 받아 값을 채운 후 검사
        for(j = 0; j < (pMemory->minBlockCnt >> i); ++j) {
            pBuffer = kAllocateMemory(MIN_BUDDY_MEMORY_SIZE << i);
            if(pBuffer == NULL) {
                kPrintf("\nAllocation Fail\n");
                return;
            }

            for(k = 0; k < (MIN_BUDDY_MEMORY_SIZE << i) / 8; ++k)
                pBuffer[k] = k;
            
            for(k = 0; k < (MIN_BUDDY_MEMORY_SIZE << i) / 8; ++k) {
                if(pBuffer[k] != k) {
                    kPrintf("\nCompare Failed\n");
                    return;
                }
            }

            // 진행 과정을 .으로 표시
            // kPrintf(".");
        }

        kPrintf("\nFree: \n");
        // 할당받은 블록을 모두 반환
        for(j = 0; j < (pMemory->minBlockCnt >> i); ++j) {
            if(kFreeMemory((void *)(pMemory->startAddr + (MIN_BUDDY_MEMORY_SIZE << i) * j)) == FALSE) {
                kPrintf("\nFree Fail\n");
                return;
            }
            // 진행 과정을 .으로 표시
            // kPrintf(".");
        }
        kPrintf("\n");
    }
    kPrintf("Test Complete\n");
}

static void kRandAllocTask(void) {
    TCB *pTask;
    QWORD memSize;
    char buffer[200];
    BYTE *pAllocBuf;
    int i, j, y;

    pTask = kGetRunningTask();
    y = pTask->link.id % 15 + 2;

    for(j = 0; j < 10; ++j) {
        // 1 KB ~ 32 MB 사이에 할당
        do {
            memSize = ((kRand() % (32 * 1024)) + 1) * 1024;
            pAllocBuf = kAllocateMemory(memSize);

            // 만일 버퍼를 할당받지 못하면 잠시 대기 후 재시도
            if(pAllocBuf == 0)
                kSleep(1);
        } while(pAllocBuf == 0);

        kSPrintf(buffer, "|Addr: [0x%Q] Size: [0x%Q] Alloc Success", pAllocBuf, memSize);

        // 자신의 ID를 Y좌표로 사용해 출력
        kPrintStringXY(0, y, buffer);
        kSleep(200);

        kSPrintf(buffer, "|Addr: [0x%Q] Size: [0x%Q] Data Write...", pAllocBuf, memSize);
        kPrintStringXY(0, y, buffer);
        for(i = 0; i < memSize / 2; ++i) {
            pAllocBuf[i] = kRand() & 0xFF;
            pAllocBuf[i + memSize / 2] = pAllocBuf[i];
        }
        kSleep(200);

        // 정상적으로 Write 되었는지 확인
        kSPrintf(buffer, "|Addr: [0x%Q] Size: [0x%Q] Data Verify..", pAllocBuf, memSize);
        kPrintStringXY(0, y, buffer);
        for(i = 0; i < memSize / 2; ++i) {
            if(pAllocBuf[i] != pAllocBuf[i + memSize / 2]) {
                kSetCursor(50, y);
                kPrintf("TID[0x%Q] Verify Fail\n", pTask->link.id);
                kExitTask();
            }
        }
        kFreeMemory(pAllocBuf);
        kSleep(200);
    }

    kExitTask();
}

static void kTestRandAlloc(const char *pParmBuf) {
    int i;

    kClear(NULL);
    for(i = 0; i < 100; ++i)
        kCreateTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kRandAllocTask);
}

static void kShowHDDInfo(const char *pParmBuf) {
    HDD_INFO hddInfo;
    char pBuf[100];

    // Read HDD Info
    if(kReadHDDInfo(TRUE, TRUE, &hddInfo) == FALSE) {
        kPrintf("Read HDD Info Fail\n");
        return;
    }

    kPrintf("========== Primary Master HDD info ==========\n");

    // Model Number
    kMemCpy(pBuf, hddInfo.modelNumber, sizeof(hddInfo.modelNumber));
    pBuf[sizeof(hddInfo.modelNumber) - 1] = NULL;
    kPrintf("Model Number:\t %s\n", pBuf);
    
    // Serial Number
    kMemCpy(pBuf, hddInfo.serialNumber, sizeof(hddInfo.serialNumber));
    pBuf[sizeof(hddInfo.serialNumber) - 1] = NULL;
    kPrintf("Serial Number:\t %s\n", pBuf);

    // 헤드, 실린더, 실린더 당 섹터 개수 출력
    kPrintf("#Heads:\t\t %d\n", hddInfo.numberOfHead);
    kPrintf("#Cylinders:\t %d\n", hddInfo.numberOfCylinder);
    kPrintf("#Sectors:\t %d\n", hddInfo.numberOfSectorPerCylinder);

    // 총 섹터 개수 출력
    kPrintf("Total Sector:\t %d Sectors, %d MB\n", hddInfo.totalSectors, hddInfo.totalSectors / 2 / 1024);
}

static void kReadSector(const char *pParmBuf) {
    PARAMLIST list;
    char pLBA[50], pSectorCnt[50];
    DWORD lba;
    int sectorCnt;
    char *pBuf;
    int i, j;
    BYTE data;
    BOOL isExit = FALSE;

    // 파라미터 리스트 초기화, LBA 어드레스, 섹터 수 추출
    kInitializeParam(&list, pParmBuf);
    if(kGetNextParameter(&list, pLBA) == 0 ||
        kGetNextParameter(&list, pSectorCnt) == 0) {
            kPrintf("ex) readsector 0(LBA) 10(count)\n");
            return;
        }
    
    lba = kAtoI(pLBA, 10);
    sectorCnt = kAtoI(pSectorCnt, 10);

    // 섹터 수 만큼 메모리를 할당 받아 READ 수행
    pBuf == kAllocateMemory(sectorCnt * 512);
    if(kReadHDDSector(TRUE, TRUE, lba, sectorCnt, pBuf) == sectorCnt) {
        kPrintf("LBA [%d], [%d] Sector Read Success\n", lba, sectorCnt);
        for(j = 0; j < sectorCnt; ++j) {
            for(i = 0; i < 512; ++i) {
                if(!(j == 0 && i == 0) && (i % 256 == 0)) {
                    kPrintf("\nPress any key to continue... ('q' is exit) : ");
                    if(kGetCh() == 'q') {
                        isExit = TRUE;
                        break;
                    }
                }

                if(i % 16 == 0)
                    kPrintf("\n[LBA: %d, offset: %d]\t| ", lba + j, i);
                
                // 모두 두 자리로 표시, 16보다 작은 경우 0 추가
                data = pBuf[j * 512 + i] & 0xFF;
                if(data < 16)
                    kPrintf("0");
                kPrintf("%x", data);
            }

            if(isExit)
                break;
        }
        kPrintf("\n");
    }
    else
        kPrintf("Read Fail\n");
    
    kFreeMemory(pBuf);
}

static void kWriteSector(const char *pParmBuf) {
    PARAMLIST list;
    char pLBA[50], pSectorCnt[50];
    DWORD lba;
    int sectorCnt;
    char *pBuf;
    int i, j;
    BYTE data;
    BOOL isExit = FALSE;
    static DWORD writeCnt = 0;

    // 파라미터 리스트 초기화, LBA 어드레스, 섹터 수 추출
    kInitializeParam(&list, pParmBuf);
    if(kGetNextParameter(&list, pLBA) == 0 ||
        kGetNextParameter(&list, pSectorCnt) == 0) {
            kPrintf("ex) writesector 0(LBA) 10(count)\n");
            return;
        }
    
    lba = kAtoI(pLBA, 10);
    sectorCnt = kAtoI(pSectorCnt, 10);

    writeCnt++;

    // 버퍼를 할당 받아 데이터를 채움
    pBuf == kAllocateMemory(sectorCnt * 512);
    for(j = 0; j < sectorCnt; ++j) {
        for(i = 0; i < 512; i += 8) {
            *(DWORD *)&(pBuf[j * 512 + i]) = lba + j;
            *(DWORD *)&(pBuf[j * 512 + i + 4]) = writeCnt;
        }
    }

    if(kWriteHDDSector(TRUE, TRUE, lba, sectorCnt, pBuf) != sectorCnt) {
        kPrintf("Write Fail\n");
        return;
    }
    kPrintf("LBA [%d], [%d] Sector Write Success", lba, sectorCnt);

    for(j = 0; j < sectorCnt; ++j) {
        for(i = 0; i < 512; ++i) {
            if(!(j == 0 && i == 0) && (i % 256 == 0)) {
                kPrintf("\nPress any key to continue... ('q' is exit) : ");
                if(kGetCh() == 'q') {
                    isExit = TRUE;
                    break;
                }
            }

            if(i % 16 == 0)
                kPrintf("\n[LBA: %d, offset: %d]\t| ", lba + j, i);
            
            // 모두 두 자리로 표시, 16보다 작은 경우 0 추가
            data = pBuf[j * 512 + i] & 0xFF;
            if(data < 16)
                kPrintf("0");
            kPrintf("%x", data);
        }

        if(isExit)
            break;
    }
    kPrintf("\n");
    kFreeMemory(pBuf);
}