[ORG 0x00]
[BITS 16]

SECTION .text

;;;
;;; 코드 영역
;;; 
START:
	mov ax, 0x1000						; Address of entrypoint of protected mode
	mov ds, ax							; Set DS segment register
	mov es, ax							; Set ES segment register

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; A20 게이트란?
    ; 초창기 XT PC에서 0x10FFEF까지 주소 영역 접근이 가능했는데, H/W의 한계로 
    ; 실제로는 0xFFEF 까지만 사용되었다. 즉, 0x10FFEF 접근 시 H/W 한계로 인해 
    ; 0xFFEF로 자연스럽게 접근이 되었고, 이를 이용한 많은 프로그램들이 작성됐다
    ; 이후 H/W의 발전으로, 0x10FFEF 접근이 가능해졌느네, 이 전에 0xFFEF로 
    ; 접근되는 것으로 가정해서 작성된 프로그램 들에 문제가 발생하기 시작했다.
    ; 결국 호환성 유지를 위해 A20 게이트가 탄생해서 주소 접근 시에 20번째 Bit를
    ; 무시하여서 이전의 주소 접근 방식대로 프로그램이 작동되도록 하였다.
    ; A20 게이트 비활성화: bit(20) = 항상 0 / 활성화: bit(20) = 0 or 1
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; BIOS를 이용하여 A20 게이트를 활성화, 실패 시 시스템 컨트롤 포트로 전환
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; BIOS 서비스를 사용해 A20 게이트 활성화
    mov ax, 0x2401      ; A20 게이트 활성화 서비스 설정
    int 0x15            ; BIOS 인터럽트 서비스 호출

    jc .A20GATEERROR    ; A20 게이트 활성화가 성공했는지 확인
    jmp .A20GATESUCCESS

.A20GATEERROR:
    ; 에러 발생 시, 시스템 컨트롤 포트로 전환 시도
    in al, 0x92         ; 시스템 컨트롤 포트(0x92)에서 1 Byte를 읽어 AL 레지스터에 저장
    or al, 0x02         ; 읽은 값에 A20 Gate Bit(1)을 1로 설정해 활성화
    and al, 0xFE        ; 시스템 리셋 방지를 위해 0xFE와 AND 연산하여 Bit(0)을 0으로 설정
    out 0x92, al        ; 시스템 컨트롤 포트(0x92)에 변경된 값으로 1 Byte 만큼 씀

.A20GATESUCCESS:
    cli                                 ; Don't allow interrupt
    lgdt [GDTR]                         ; Load GDT table by setting GDTR data structs to processor

    ;;;
    ;;; Enter protect mode
    ;;; Disable Paging, Cache, Internal FPU, Align Check
    ;;; Enable ProtectedMode
    ;;;

	mov eax, 0x4000003B					; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
	mov cr0, eax						; Enter protect mode by set CR0
	
    ; Replace Kernel Code Segment whose base is 0x00
    ; [CS Segment Selector]=0x08, EIP=(PROTECTEDMODE - $$ + 0x10000)
    jmp dword 0x08: (PROTECTEDMODE - $$ + 0x10000)

[BITS 32]
PROTECTEDMODE:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Create Stack from 0x0 ~ 0x0000FFFF as 64KBs size
    mov ss, ax                          ; 
	mov	esp, 0xFFFE	    				; 
	mov ebp, 0xFFFE						; 

    push (SWITCHSUCCESSMESSAGE - $$ + 0x10000)
    push 2
    push 0
    call PRINTMESSAGE
    add esp, 12

    ; jmp $
    ; [CS Segment Selector]=0x08, EIP=0x10200 
    jmp dword 0x08: 0x10200 ; C 언어 커널이 존재하는 0x10200 어드레스로 이동하여 C 언어 커널 수행

; Function to print message 
; Param: Coord x, Coord y, String s
PRINTMESSAGE:
	push ebp							; Push base pointer to stack
	mov ebp, esp						; Set BP reg to value of SP reg for accessing to parameters by using base pointer

	push esi							; Push (ESI ~ EDX) regs value to stack
	push edi							;
	push eax							;
	push ecx							;
	push edx							;

	; Calculate line address by using X, Y coord
	; Calculate address of line by using Y coord
	mov eax, dword[ebp + 12]			; 
	mov esi, 160						; 
	mul esi		    					; 
	mov edi, eax						; 
	
    ; Calculate address of line by using X coord
    mov eax, dword[ebp + 8]	    		; 
	mov esi, 2  						; 
	mul esi		    					; 
	add edi, eax						; 

	; Print message that mode is changed to protected
    mov esi, dword[ebp + 16]            ; Parameter 3

; Loop to print message
.MESSAGELOOP:
	mov cl, byte[esi]					;

	cmp cl, 0							; Compare 1 byte character and 0x0
	je .MESSAGEEND						; If value of character is 0x0, then goto .MESSAGEEND to finish

	mov byte [edi + 0xB8000], cl		; If not, mov value of CL reg to value of EDI reg
	
	add esi, 1							; Add 1 to ESI reg to point next character
	add edi, 2							; Add 2 to EDI reg to point next video memory addr for printing
										; Because video memory is composed of pair of character(1 byte) and attributes(1 byte)
										; to set character only, move by 2 bytes
	
	jmp .MESSAGELOOP					; call MESSAGELOOP recursively to print next character
	
.MESSAGEEND:
	pop edx								; Restore DX ~ ES Reg with value in stack temporarily
	pop ecx								; 
	pop eax								;
	pop edi								;
	pop esi								; Stack is FILO, pop reversly

	pop ebp								; Restore Base Point Reg
	ret

;;;
;;; Data Section
;;;

; 8 Bytes Aligned
align 8, db 0

; Add 0x0000 to align GDTR at 8 Bytes
dw 0x0000

GDTR:
    dw GDTEND - GDT - 1
    dd (GDT - $$ + 0x10000)

GDT:
    NULLDESCRIPTOR:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0x00
        db 0x00
        db 0x00

    CODEDESCRIPTOR:
        dw 0xFFFF   ; Limit[15:0]
        dw 0x0000   ; Base [15:0]
        db 0x00     ; Base [23:16]
        db 0x9A     ; P=1, DPL=0, Code Segment, Execute/Read
        db 0xCF     ; G=1, D=1, L=0, AVL=0, Limit[19:16]
        db 0x00     ; Base[31:24]

    DATADESCRIPTOR:
        dw 0xFFFF   ; Limit[15:0]
        dw 0x0000   ; Base [15:0]
        db 0x00     ; Base [23:16]
        db 0x92     ; P=1, DPL=0, Data Segment, Read/Write
        db 0xCF     ; G=1, D=1, L=0, AVL=0, Limit[19:16]
        db 0x00     ; Base[31:24]

GDTEND:

SWITCHSUCCESSMESSAGE:
    db 'Switch To Protected Mode Success!', 0

	
times 512 - ($ - $$) db 0x00			; $					Address of current line
										; $$				Source address of current section(.text)
										; $ - $$			Offset from current section(.text)
										; 512 - ($ - $$)	Distance from current offset to 512
										; db 0x00			Declare 1 byte and set 0
										; times				Repeat
										; Write 0x00 from current address to 512
