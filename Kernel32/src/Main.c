#include "Types.h"

void kPrintString(int iX, int iY, const char* pcString);

BOOL kInitializeKernel64Area(void);

// [Main Address] = 0x10200
// EntryPoint.s will execute Main firstly.
void Main(void) {
    DWORD i;
    
    kPrintString(0, 3, "C Language Kernel Start!");

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
    kInitializeKernel64Area();
    kPrintString(0, 4, "IA-32e Kernel Area Initialization Complete!");

    while(1);
}

void kPrintString(int iX, int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xB8000;    // Video Memory Addr
    int i;

    pstScreen += (iY * 80) + iX;
    for(i = 0; pcString[i]; ++i)
        pstScreen[i].bCharacter = pcString[i];
}