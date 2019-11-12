#include "Descriptor.h"
#include "Utility.h"

/**
 *  GDT(Global Descriptor Table)
 *  Segment들의 크기, 시작 주소(Base Address), 권한(쓰기/읽기/실행, 커널 모드 등)
 *  GDT에 있는 시작주소를 참조해서 각 세그먼트들의 주소로 접근 가능
 *  GDT에 대한 정보는 CPU내 GDTR 레지스터에 저장 됨
 */
// Initialize GDT(Global Descriptor Table)