[ORG 0x00]
[BITS 16]

SECTION .text
					; Value of Segment Register will be translated by multiplying 16(= << 4), See 93 Page
jmp 0x07C0:START	; Copy "0x07C0" To CS Segment Reg
					; Copy "Address of START Label" to IP Reg

START:				; global function
	mov ax, 0x07C0	; Address of Bootloader
	mov ds, ax		; Set Address of Bootloader to DS Segment Register
	mov ax, 0xB800	; Address of Video Memory
	mov es, ax		; Set Address of Video Memory to ES Segment Register

	mov si, 0		; SI Register(String source index Register)

.SCREENCLEARLOOP:					; .FUNCTION is local Function
	mov byte [ es: si ], 0			; Write 0x00 to clear Video Memory
	mov byte [ es: si + 1 ], 0x0A	; Write 0x0A to set Attr to printing green char, black background

	add si, 2						; Move next char
	cmp si, 80 * 25 * 2				; Compare si and (80 * 25 * 2) to check it finished

	jl .SCREENCLEARLOOP				; if not, print next

	mov si, 0						; Init SI Reg
	mov di, 0						; Init DI Reg

.MESSAGELOOP:						; Loop to print message
	mov cl, byte [ si + MESSAGE1 ]	; MESSAGE1 주소에서 SI 레지스터 값 만큼 더한 위치의 문자를 CL 레지스터에 복사
									; 문자열은 1 바이트면 충분하므로, CX 레지스터 하위 1바이트만 사용

	cmp cl, 0				; Compare copied char and 0x0
	je .MESSAGEEND			; if char is 0x0, then goto .MESSAGEEND to finish

	mov byte [ es: di ], cl	; if not, mov value of CL Reg to (value of ES) + DI
	
	add si, 1				; SI 레지스터에 1을 더해 다음 문자열로 이동
	add di, 2				; DI 레지스터에 2를 더해, 비디오 메모리 다음 문자 출력 위치로 이동
							; 비디오 메모리는 (문자, 속성) 쌍으로 구성되므로, 속성 없이 문자만 설정하기 위해서는 2씩 이동
	
	jmp .MESSAGELOOP		; 다음 문자를 출력하기 위해 이동
	
.MESSAGEEND:
	jmp	$					; 현재 위치에서 무한루프 수행

MESSAGE1:
	db 'BBI OS Boot Loader Start~!!', 0	; 마지막 값을 0으로 설정해 .MESSAGELOOP에서 문자열이 종료됐음을 인지할 수 있도록 만듦
	
times 510 - ($ - $$) db 0x00	; $					현재 Line의 Address
								; $$				현재 Section(.text)의 Src Address
								; $ - $$			현재 Section을 기준으로 하는 Offset
								; 510 - ($ - $$)	현재부터 Adress 510 까지
								; db 0x00			1바이트를 선언하고 값은 0
								; time				반복 수행
								; 현 위치에서 510 번지까지 0x00 쓰기

db 0x55							; 511번지에 0x55를 쓰기
db 0xAA							; 512번지에 0xAA를 쓰기
								; 부트 섹터임을 표기
