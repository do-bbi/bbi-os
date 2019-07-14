[ORG 0x00]								; Set start adress of code to 0x00
[BITS 16]								; Set bit mode to 16-bit

SECTION .text							; Set section to .text section
										; Value of segment register will be translated by multiplying 16(= << 4), See 93 page
jmp 0x1000:START						; Copy "0x1000" to CS segment reg to move address of START label

SECTORCOUNT:
	dw 0x0000							; Save current sector number

TOTALSECTORCOUNT	equ 1024			; Total sector number of virtual OS
										; Maximum 1152 sectors (=0x90000 bytes)

; Global function
START:
	mov ax, cs							; Set AX reg to value of CS reg
	mov ds, ax							; Set DS reg to value of AX reg
	mov ax, 0xB800						; Set AX reg to 0xB800(Address of video memory)
	mov es, ax							; Set ES reg to value of AX reg

; Create codes for each sectors
%assign i	0							; Declare variable i and set 0
%rep	TOTALSECTORCOUNT				; Repeat until TOTALSECTORCOUNT
	%assign i	i + 1					; i++

	mov ax, 2							; Set AX reg to number of bytes for 1 character
	
	mul word[SECTORCOUNT]				; Multiply AX reg and count of sector 
	mov si, ax							; Set SI reg to multiplied value
	
	; Print Sector Number from address of video memory
	mov byte[es:si+(160 * 2)], '0' + (i % 10)
	add word[SECTORCOUNT], 1			; Increase SECTORCOUNT by 1

	%if i == TOTALSECTORCOUNT			; If i is TOTALSECTORCOUNT
		jmp $							; Do infinite loop
	%else								; If not
		jmp (0x1000 + i * 0x20):0x0000	; Move next sector
	%endif

	times (512 - ($ - $$) % 512) db 0x00; $						Address of current line
										; $$					Source address of current section(.text)
										; $ - $$				Offset from current section(.text)
										; 512 - ($ - $$) % 512	Distance from current offset to 510
										; db 0x00				Declare 1 byte and set 0
										; times					Repeat
										; Write 0x00 from current address to 512
%endrep