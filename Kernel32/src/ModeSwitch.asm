; 이 코드는 32bit 모드로 동작
[BITS 32]

; C Code에서 호출할 수 있도록 Export
global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text

; CPUID를 반환하는 함수
; Function Parameter(DWORD eax_val, DWORD *pEAX, DWORD *pEBX, DWORD *pECX, DWORD *pEDX)
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

; IA-32e 모드 전환 및 64bit 커널 수행
; Function Parameter(void)
kSwitchAndExecute64bitKernel:
    ; CR4 컨트롤 레지스터의 PAE 비트를 1로 설정
    mov eax, cr4    ; CR4 컨트롤 레지스터의 값을 EAX 레지스터에 저장
    or eax, 0x20    ; PAE[5]=1
    mov cr4, eax    ; PAE[5]=1 이 된 EAX 레지스터 값을 CR4 컨트롤 레지스터에 저장
    
    ; CR3 컨트롤 레지스터로 PML4 Table 주소와 캐시 활성화
    mov eax, 0x100000   ; EAX 레지스터에 PML4 Table의 주소인 0x100000를 저장
    mov cr3, eax        ; PML4 Table의 주소(0x100000)이 담긴 EAX 레지스터 값을 CR3 컨트롤 레지스터에 저장

    ; IA32_EFER(IA32 Extended Feature Enable Register)의 LME 값을 1로 설정하여 IA-32e 모드를 활성화
    mov ecx, 0xC0000080 ; IA32_EFER MSR(Model Specific Register)의 주소를 ECX 레지스터에 저장
    rdmsr               ; IA32_EFER을 위한 특수 명령어 rdmsr을 이용해 MSR 읽기 -> EAX 레지스터

    or eax, 0x0100      ; EAX 레지스터에 저장된 IA32_EFER MSR 값의 하위 32bit 중 LME[8]=1
    wrmsr               ; IA32_EFER을 위한 특수 명령어 wrmsr을 이용해 MSR 쓰기 <- EAX 레지스터

    ; CR0 레지스터의 NW[29](Not Write-through)와 CD[30](Cache Disabled)는 0, PG[31](Paging)는 1로 설정해 캐시와 페이징을 활성화
    mov eax, cr0        ; EAX 레지스터에 CR0 컨트롤 레지스터 값을 저장
    or eax, 0xE0000000  ; NW[29]=1, CD[29]=1, PG[29]=1
    xor eax, 0x60000000 ; NW[29]^1=0, CD[29]^1=0, PG[29]^0=1
    mov cr0, eax        ; CR0 컨트롤 레지스터에 EAX 레지스터의 값을 저장

    jmp 0x08:0x200000   ; CS 세그먼트 셀렉터를 IA-32e 모드용 코드 세그먼트 디스크립터로 교체하고, 0x200000 으로 Jump

    ; Follow code won't run
    jmp $
