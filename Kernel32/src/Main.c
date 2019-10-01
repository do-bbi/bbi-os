#include "Types.h"

void kPrintString(int iX, int iY, const char* pcString);

BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);

// [Main Address] = 0x10200
// EntryPoint.s will execute Main firstly.
void Main(void) {
    DWORD i;
    
    kPrintString(0, 3, "C language kernel start!....................[PASS]");

    kPrintString(0, 4, "Minimum memory size check...................[    ]");
    if(!kIsMemoryEnough()) {
        kPrintString(45, 4, "FAIL");
        kPrintString(0, 5, "Not enough memory, MINT64 OS requires over 64MBs memory");
        while(TRUE);
    }
    kPrintString(45, 4, "PASS");

    // 1. PC의 장착된 물리 메모리가 64MB 이상인지 확인
    // - 커널이 올라가는 메모리 주소는 0x10000, 만약 1MB 이하의 어드레스 중에 비디오 메모리가 위치한 
    //   0xA0000 이하를 커널로 사용하면 보호 모드 커널과 IA-32e 모드 커널의 최대 크기는 고작 576KB 정도
    //   커널 이미지에는 초기화 되지 않는 영역(.bss)은 포함되지 않기 때문에, 커널 이미지는 이보다 더 작아야 함
    //   커널의 기능이 소형이라면 상관 없지만, 멀티 태스킹과 파일 시스템 기능이 추가 되면 576KB 만으로는 부족
    //   MINT64 OS는 이 문제를 해결하기 위해 IA-32e 모드 커널은 2MB ~ 6MB 주소 공간을 사용하여 총 4MB을 사용함
    // 2. IA-32e 커널 이미지가 올라갈 주소 영역 2MB ~ 6MB 공간의 값들을 0으로 초기화
    // - 커널 이미지에는 초기화되지 않는 영역(.bss)가 포함되지 않기 때문에, 해당 주소 영역에 있던 값을 그대로 사용
    //   하기 때문에, 커널 이미지를 올리기 전에 0으로 초기화 함

    // IA-32e 모드 커널 영역 초기화
    kPrintString(0, 5, "IA-32e Kernel area initialize...............[    ]");
    if(!kInitializeKernel64Area()) {
        kPrintString(45, 5, "FAIL");
        kPrintString(0, 6, "Kernel area initialization failed");
        while(TRUE);
    }
    kPrintString(45, 5, "PASS");

    while(TRUE);
}

void kPrintString(int iX, int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xB8000;    // Video Memory Addr
    int i;

    pstScreen += (iY * 80) + iX;
    for(i = 0; pcString[i]; ++i)
        pstScreen[i].bCharacter = pcString[i];
}

// IA-32e 모드용 커널 영역 0으로 초기화
BOOL kInitializeKernel64Area(void) {
    DWORD *pCurrentAddr;

    // 초기화를 시작할 주소 0x100000(1MB)
    pCurrentAddr = (DWORD *)0x100000;

    // 마지막 주소인 0x600000(6MB)까지 반복문을 돌며 4Bytes 씩 0으로 채움
    while((DWORD)pCurrentAddr < 0x600000) {
        *pCurrentAddr = 0x00;

        // 0으로 저장한 후 다시 읽었을 때 0이 나오지 않으면 해당 주소 영역에 문제가 있는 것이므로 중단
        if(*pCurrentAddr)
            return FALSE;
            
        pCurrentAddr++;
    }

    return TRUE;
}

// MINT64 OS를 실행하기에 충분한 메모리를 갖고 있는지 확인
BOOL kIsMemoryEnough(void) {
    DWORD *pCurrentAddr;

    // 0x100000(1MB) 부터 검사 시작
    pCurrentAddr = (DWORD *)0x100000;

    // 0x4000000(64MBs)까지 반복문을 돌며 확인
    while((DWORD)pCurrentAddr < 0x4000000) {
        *pCurrentAddr = 0x12345678;

        // 0x12345678을 쓰고, 다시 해당주소의 값을 읽어들였을 때 0x12345678이 나오지 않으면
        // 해당 메모리 영역이 없거나 문제가 생긴 것이므로 커널 초기화를 종료
        if(*pCurrentAddr != 0x12345678)
            return FALSE;

        // 1MB 씩 이동하며 확인
        pCurrentAddr += (0x100000 >> 2);
    }

    return TRUE;
}