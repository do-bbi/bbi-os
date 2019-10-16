#include "Types.h"

#define VIDEO_MEM_ADDR  (0xB8000)

#define PRINT_BLANK_POS  (45)

void kPrintString(int iX, int iY, const char* pcString);

// C 언어 커널
void Main(void) {
    int posY = 11;
    
    kPrintString(0, posY, "Switch to IA-32e Mode Success");
    posY++;
    kPrintString(0, posY, "IA-32e C Language Kernel Start");
}

void kPrintString(int iX, int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)VIDEO_MEM_ADDR;    // Video Memory Addr
    int i;

    pstScreen += (iY * 80) + iX;
    for(i = 0; pcString[i]; ++i)
        pstScreen[i].bCharacter = pcString[i];
}