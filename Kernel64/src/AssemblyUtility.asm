[BITS 64]           ; 이하 코드는 64bit 코드로 설정

SECTION .text       ; text 섹션 정의

; C 언어에서 호출 가능하도록 함수 Export
global kInPortByte, kOutPortByte

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
