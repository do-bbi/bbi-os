[BITS 64]           ; 이하 코드는 64bit 코드로 설정

SECTION .text       ; text 섹션 정의

; C 언어에서 호출 가능하도록 함수 Export
global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt
global kReadRFLAGS, kReadTSC
global kSwitchContext

SECTION .text

; Port로 부터 1 Byte 읽기
; PARAM: WORD portNum
kInPortByte:
    push rdx        ; 함수에서 임시로 사용할 레지스터 값 보관
    
    mov rdx, rdi    ; RDX 레지스터에 PARAM 0(portNum) 저장
    mov rax, 0      ; RAX 레지스터 초기화
    in al, dx       ; DX 레지스터에 저장된 포트 주소에서 1 Byte를 읽어
                    ; AL 레지스터에 저장, AL 레지스터는 함수 반환 값으로 사용
    
    pop rdx         ; 함수에서 사용이 끝난 레지스터 복원
    ret             ; 함수 호출 후, 다음 실행 코드 위치로 복귀

; Port에 1 Byte 쓰기
; PARAM: WORD portNum, BYTE data
kOutPortByte:
    push rdx        ; 함수에서 임시로 사용할 레지스터 값 보관
    push rax        ; 함수에서 임시로 사용할 레지스터 값 보관
    
    mov rdx, rdi    ; RDX 레지스터에 PARAM 0(portNum) 저장
    mov rax, rsi    ; RAX 레지스터에 PARAM 1(data) 저장
    out dx, al      ; DX 레지스터에 저장된 포트 주소로 AL 레지스터 값 1 Byte를 씀
    
    pop rax         ; 함수에서 사용이 끝난 레지스터 복원
    pop rdx         ; 함수에서 사용이 끝난 레지스터 복원
    ret             ; 함수 호출 후, 다음 실행 코드 위치로 복귀

; GDTR 레지스터에 GDT 테이블 설정
; PARAM: QWORD pGDT
kLoadGDTR:
    lgdt [rdi]  ; Param 0(Address of GDTR)를 프로세서에 로드
                ; GDT 테이블 설정
    ret

; TR 레지스터에 TS 세그먼트 디스크립터 설정
; PARAM: WORD offset
kLoadTR:
    ltr di      ; Param 0(offset of TS 세그먼트를 프로세서에 설정
                ; Load TS(Task State) Segment
    ret

; IDTR 레지스터에 IDT 테이블 설정
; PARAM: QWORD pIDTR
kLoadIDTR:
    lidt [rdi]  ; Param 0(Address of IDTR)를 프로세서에 로드
                ; IDT 테이블 설정
    ret

; Activate Interrupts
; PARAM: None
kEnableInterrupt:
    sti         ; Activate Interrupts
    ret

; Deactivate Interrupts
; PARAM: None
kDisableInterrupt:
    cli         ; Deactivate Interrupts
    ret

; Read RFLAGS & Return
kReadRFLAGS:
    pushfq      ; Save RFLAGS Register to Stack
    pop rax     ; Save RFLAGS in stack to RAX Register

    ret         ; Return RAX Value

; Read Time Stamp Counter & Return
kReadTSC:
    push rdx    ; Store RDX Register value to Stack
    rdtsc       ; Read Time Stamp Counter & Save it to RDX:RAX

    shl rdx, 32 ; RDX 레지스터 상위 32bit TSC 값을 32bit Left Shift 하고
    or rax, rdx ; rax 레지스터에 있는 하위 32bit TSC 값을 or 하여 RAX 레지스터에 64bit TSC 값을 저장

    pop rdx     ; Restore previous RDX Register value from stack
    ret         ; Return RAX Value

; Macro to save context and replace selector
%macro KSAVECONTEXT 0       ; No Parameter For Macro
    ; Insert From RBP Register To GS Segment Selector into stack
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    mov ax, ds      ; Since DS Selectors Can't be inserted directly into stack,
    push rax        ; Store them into RAX register before inserting them into stack

    mov ax, es      ; Since ES Selectors Can't be inserted directly into stack,
    push rax        ; Store them into RAX register before inserting them into stack

    push fs
    push gs 
%endmacro


; Macro to Restore Context
%macro KLOADCONTEXT 0   ; No Parameter For Macro
    ; Pop GS Selector Value ~ RBP Register Value From Stack
    pop gs
    pop fs
    pop rax         ; ES Selector Can't be retrieved directly From Stack
    mov es, ax      ; Pop ES Selector Value to RAX Register From Stack And Move into ES Selector
    pop rax         ; DS Selector Can't be retrieved directly From Stack
    mov ds, ax      ; Pop ES Selector Value to RAX Register From Stack And Move into ES Selector
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp        
%endmacro

; Current Context에 현재 콘텍스트를 저장하고 Next Task에서 콘텍스트를 복구
;   PARAM: Current Context, Next Context
kSwitchContext:
    push rbp        ; 스택에 RBP 레지스터를 저장하고 RSP 레지스터를 RBP에 저장
    mov rbp, rsp
    
    ; Current Context가 NULL이면 콘텍스트를 저장할 필요 없음
    pushfq          ; 아래의 cmp의 결과로 RFLAGS 레지스터가 변하지 않도록 스택에 저장
    cmp rdi, 0      ; Current Context가 NULL이면 콘텍스트 복원으로 바로 이동
    je .LoadContext 
    popfq           ; 스택에 저장한 RFLAGS 레지스터를 복원

    ; 현재 태스크의 콘텍스트를 저장
    push rax            ; 콘텍스트 영역의 오프셋으로 사용할 RAX 레지스터를 스택에 저장
    
    ; SS, RSP, RFLAGS, CS, RIP 레지스터 순서대로 삽입
    mov ax, ss                          ; SS 레지스터 저장
    mov qword[ rdi + ( 23 * 8 ) ], rax

    mov rax, rbp                        ; RBP에 저장된 RSP 레지스터 저장
    add rax, 16                         ; RSP 레지스터는 push rbp와 Return Address를
    mov qword[ rdi + ( 22 * 8 ) ], rax  ; 제외한 값으로 저장
    
    pushfq                              ; RFLAGS 레지스터 저장
    pop rax
    mov qword[ rdi + ( 21 * 8 ) ], rax

    mov ax, cs                          ; CS 레지스터 저장
    mov qword[ rdi + ( 20 * 8 ) ], rax
    
    mov rax, qword[ rbp + 8 ]           ; RIP 레지스터를 Return Address로 설정하여 
    mov qword[ rdi + ( 19 * 8 ) ], rax  ; 다음 콘텍스트 복원 시에 이 함수를 호출한 
                                        ; 위치로 이동하게 함
    
    ; 저장한 레지스터를 복구한 후 인터럽트가 발생했을 때처럼 나머지 콘텍스트를 모두 저장
    pop rax
    pop rbp
    
    ; 가장 끝부분에 SS, RSP, RFLAGS, CS, RIP 레지스터를 저장했으므로, 이전 영역에
    ; push 명령어로 콘텍스트를 저장하기 위해 스택을 변경
    add rdi, ( 19 * 8 )
    mov rsp, rdi
    sub rdi, ( 19 * 8 )
    
    ; 나머지 레지스터를 모두 Context 자료구조에 저장
    KSAVECONTEXT


; 다음 태스크의 콘텍스트 복원
.LoadContext:
    mov rsp, rsi
    
    ; Context 자료구조에서 레지스터를 복원
    KLOADCONTEXT
    iretq