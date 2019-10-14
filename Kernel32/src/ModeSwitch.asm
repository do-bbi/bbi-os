; C Code에서 호출할 수 있도록 Export
global kReadCPUID

SECTION .text

; CPUID를 반환하는 함수
; Function Parameter = DWORD eax_val, DWORD *pEAX, *pEBX, *pECX, *pEDX
kReadCPUID:
    push ebp        ; EBP(Base Pointer Register) 값을 스택에 Push
    mov ebp, esp    ; EBP에 ESP(Stack Pointer Register) 값을 Set
    push eax        ; 함수에서 임시로 사용하는 레지스터들(eax, ebx, ecx, edx)
    push ebx        ; 함수 호출이 끝나면, 스택에 Push 된 값들을 Pop해서 복원
    push ecx
    push edx
    push esi

    ; EAX 레지스터 값을 이용해 CPUID 명령어 실행
    ; EAX 값 = 0x00000000: 기본 CPUID 정보 조회
    ; EAX 값 = 0x80000001: 확장 기능 CPUID 정보 조회
    mov eax, dword [ebp + 8]    ; Parameter 1(eax_val)를 EAX 레지스터에 저장
    cpuid                       ; cpuid 명령어 실행

    ; cpuid 명령어 실행 결과를 함수 파라미터로 입력된 주소들에 저장
    ; [ESI] = pEAX, *[ESI] = [EAX]
    mov esi, dword[ebp + 12]    ; 2번째 파라미터 pEAX를 ESI 레지스터에 저장
    mov dword[esi], eax         ; pEAX가 가리키는 주소에 EAX 레지스터의 값을 저장
    ; [ESI] = pEBX, *[ESI] = [EBX]
    mov esi, dword[ebp + 16]    ; 3번째 파라미터 pEBX를 ESI 레지스터에 저장
    mov dword[esi], ebx         ; pEBX가 가리키는 주소에 EBX 레지스터의 값을 저장
    ; [ESI] = pECX, *[ESI] = [ECX]
    mov esi, dword[ebp + 20]    ; 4번째 파라미터 pECX를 ESI 레지스터에 저장
    mov dword[esi], ecx         ; pECX가 가리키는 주소에 ECX 레지스터의 값을 저장
    ; [ESI] = pEDX, *[ESI] = [EDX]
    mov esi, dword[ebp + 24]    ; 5번째 파라미터 pEDX를 ESI 레지스터에 저장
    mov dword[esi], edx         ; pEDX가 가리키는 주소에 EDX 레지스터의 값을 저장

    pop esi                     ; 함수에서 사용이 끝난 ESI 레지스터 ~ EBP 레지스터의 값들을 스택으로 부터 꺼내서 복원
    pop edx                     ; 스택은 LIFO(Last In First Out) 자료구조 이기 때문에, 스택에 넣은 역순으로 복원
    pop ecx
    pop ebx
    pop eax
    pop ebp

    ret                         ; 함수를 호출한 코드의 위치로 복귀 