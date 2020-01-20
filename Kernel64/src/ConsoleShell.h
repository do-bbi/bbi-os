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
void kHelp(const char *pParamBuf);
void kClear(const char *pParamBuf);
void kShutdown(const char *pParamBuf);
void kStrToNumTest(const char *pParamBuf);
void kShowTotalSizeofRAM(const char *pParamBuf);

#endif  // __CONSOLESHELL_H__