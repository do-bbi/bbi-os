#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

#define CONSOLE_SHELL_MAX_COMMANDS_BUFFER_COUNT (300)
#define CONSOLE_SHELL_PROMPT_MESSAGE            ("BBI >")

typedef void (* CommandFunction)(const char *pParams);

#pragma pack(push, 1)

// Struct for saving commands of shell
typedef struct kShellCommandEntryStruct {
  char *pCommand;
  char *pHelp;

  CommandFunction pFunc;
} SHELLCOMMANDENTRY;

typedef struct kParamListStruct {
    const char *pBuf;
    int len;
    int cur;
} PARAMLIST;

#pragma pack(pop)

void kStartConsoleShell(void);
void kExecuteCommand(const char *pCommandBuf);
void kInitializeParam(PARAMLIST *pList, const char *pParam);
int kGetNextParameter(PARAMLIST *pList, char *pParam);

// Functions for COMMAND
static void kHelp(const char *pParamBuf);
static void kClear(const char *pParamBuf);
static void kShutdown(const char *pParamBuf);
static void kStrToNumTest(const char *pParamBuf);
static void kShowTotalSizeofRAM(const char *pParamBuf);
static void kSetTimer(const char *pParamBuf);
static void kWaitUsingPIT(const char *pParamBuf);
static void kReadTimeStampCounter(const char *pParamBuf);
static void kMeasureProcessorSpeed(const char *pParamBuf);
static void kShowDateAndTime(const char *pParamBuf);
static void kCreateTestTask(const char *pParamBuf);
static void kChangeTaskPriority(const char *pParamBuf);
static void kShowTaskList(const char *pParamBuf);
static void kKillTask(const char *pParamBuf);
static void kCPULoad(const char *pParamBuf);
static void kTestMutex(const char *pParmBuf);
static void kTestThread(const char *pParmBuf);
static void kShowMatrix(const char *pParmBuf);
static void kTestPIE(const char *pParmBuf);
static void kShowBuddyMemInfo(const char *pParmBuf);
static void kTestSeqAlloc(const char *pParmBuf);
static void kTestRandAlloc(const char *pParmBuf);
static void kShowHDDInfo(const char *pParmBuf);
static void kReadSector(const char *pParmBuf);
static void kWriteSector(const char *pParmBuf);
static void kMountHDD(const char *pParmBuf);
static void kFormatHDD(const char *pParmBuf);
static void kShowFSInfo(const char *pParmBuf);
static void kCreateFileInRootDir(const char *pParmBuf);
static void kDeleteFileInRootDir(const char *pParmBuf);
static void kShowRootDir(const char *pParmBuf);

#endif  // __CONSOLESHELL_H__