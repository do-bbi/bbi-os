[ORG 0x00]
[BITS 16]

SECTION .text
										; Value of segment register will be translated by multiplying 16(= << 4), See 93 page
jmp 0x07C0:START						; Copy "0x07C0" to CS segment reg
										; Copy "Address of START Label" to IP reg

TOTAL_SECTOR_COUNT:
	dw 0x07;							; Count of BBI OS Protected Mode Kernel(Except Bootloader)
										; Maximum number of sectors is (0xA0000 - 0x10000 = 0x90000) / 512 = 1152 sectors
KERNEL32_SECTOR_COUNT:
	dw 0x05;							
	
										; Count of BBI OS IA-32e Mode Kernel Sectors
										; Maximum number of sectors is (0x600000 - 0x200000 = 0x400000) / 512 = 8192 sectors

; Global function
START:
	mov ax, 0x07C0						; Address of bootloader
	mov ds, ax							; Set address of bootloader to DS segment register
	mov ax, 0xB800						; Address of video memory
	mov es, ax							; Set address of video memory to ES segment register

	; Create Stack whose size is 64KB at 0x0000:0000 ~ 0x0000:FFFF

	mov ax, 0x0000						; Set AX reg to 0x0000
	mov ss, ax							; Set SS reg to 0x0000
	mov sp, 0xFFFE						; Set SP reg to 0xFFFE
	mov bp, 0xFFFE						; Set BP reg to 0xFFFE

	; Clear Screen and Set attr to GREEN
	mov	si, 0							; Initialize SI reg standing for (String) Source index

; .FUNCTION is local Function
.SCREENCLEARLOOP:						
	mov byte [es:si], 0					; Write 0x00 to clear video memory
	mov byte [es:si+1], 0x0A			; Write 0x0A to set Attr to printing green char, black background

	add si, 2							; Move next char
	cmp si, 80 * 25 * 2					; Compare si and (80 * 25 * 2) to check it finished

	jl	.SCREENCLEARLOOP				; If not, print next

	; Print start message at the top of screen
	push MESSAGE1						; Push address of MESSAGE1
	push 0								; Push Y Coord of message
	push 0								; Push X Coord of message
	call PRINTMESSAGE					; Call PRINTMESSAGE Function
	add	sp, 6							; Cleanup stack parameters

	; Print message about loading os image
	push IMAGELOADINGMESSAGE			; Push address of IMAGE LOADING MESSAGE
	push 1								; Push Y Coord of message
	push 0								; Push X Coord of message
	call PRINTMESSAGE					; Call PRINTMESSAGE Function
	add sp, 6							; Cleanup stack parameters 

; Call BIOS reset function
RESETDISK:
	; Set 0 to Service No. which is Floopy Disk

	mov ax, 0
	mov dl, 0
	int	0x13

	; If error occurs, goto error handling
	jc	HANDLEDISKERROR

	; Read Sector From Disk
	mov	si, 0x1000						; Set address where disk data will be saved
	mov es, si							; Set ES reg to value of si
	mov bx, 0x0000						; Set BX reg to 0x0000, to make Dst addr 0x1000:0000

	mov di, word[TOTAL_SECTOR_COUNT]	; Set DI reg to number of sectors for coping OS image

READDATA:
	cmp di, 0							; Check reading OS image is finished
	je READEND							; If number of sector to read is 0, then goto READEND
	sub di, 0x1							; Substract DI reg by 1

	; Call BIOS read function
	mov ah, 0x02						; BIOS Service No. 2
	mov al, 0x1							; Number of sector to read is 1
	mov ch, byte[TRACKNUMBER]			; Set track No. to read
	mov cl, byte[SECTORNUMBER]			; Set sector No. to read
	mov dh, byte[HEADNUMBER]			; Set head No. to read
	mov dl, 0x00						; Set drive No. to read

	int 0x13							; Execute interupt service
	jc	HANDLEDISKERROR					; If error, do HANDLEDISKERROR

	add si, 0x0020						; Add 512 to SI reg to read next sector
	mov es, si							; Set ES reg to value of SI reg to read next sector

	mov al, byte[SECTORNUMBER]			; Set AL reg to sector number
	add al, 0x01						; Increase sector number by 1
	mov byte[SECTORNUMBER], al			; Write increased sector number to AL reg
	cmp al, 36 + 1						; Compare sector number with 36 + 1 to check it's last
	jl READDATA							; If not, call READDATA recursively

	; After reading last sector, toggle head number(0 <-> 1) and set 1 to sector number
	xor byte[HEADNUMBER], 0x01			; toggle HEADNUMBER (0 <-> 1)
	mov byte[SECTORNUMBER], 0x01		; set 1 to SECTORNUMBER

	; If HEADNUMBER is changed from 1 to 0, increase track number by 1
	cmp byte[HEADNUMBER], 0x00			; Compare HEADNUMBER with 0x00
	jne READDATA						; If not same, goto READDATA

	; Increase TRACKNUMBER by 1 and goto READDATA
	add byte[TRACKNUMBER], 0x01			; Increase TRACKNUMBER by 1
	jmp READDATA						; Goto READDATA

; Print Load OS Image is finished
READEND:
	push LOADINGCOMPLETEMESSAGE			; Push address of LOADING COMPLETE MESSAGE to stack
	push 1								; Push Y Coord(1) to stack
	push 20								; Push X Coord(20) to stack
	call PRINTMESSAGE					; Call PRINTMESSAGE function
	add sp, 6							; cleanup stack

	; Execute virtual OS Image loaded
	jmp 0x1000:0000

; Function to handle disk error
HANDLEDISKERROR:
	push DISKERRORMESSAGE				; Push address of DISK ERROR MESSAGE to stack
	push 1								; Push Y Coord(1) to stack
	push 20								; Push X Coord(20) to stack
	call PRINTMESSAGE					; Call PRINTMESSAGE function

	jmp $								; Do infinite loop

; Function to print message 
; Param: Coord x, Coord y, String s
PRINTMESSAGE:
	push bp								; Push base pointer to stack
	mov bp, sp							; Set BP reg to value of SP reg for accessing to parameters by using base pointer
	
	push es								; Push (ES ~ DX) regs value to stack
	push si								; 
	push di								;
	push ax								;
	push cx								;
	push dx								; After executing function, restore regs' value with temporarily stack values

	mov	ax, 0xB800						; Set 0xB800 to AX reg
	mov es, ax							; Set ES reg to 0xB800(Address of Video Memory)

	; Calculate line address by using X, Y coord
	; Calculate address of line by using Y coord
	mov ax, word[bp + 6]				; Set AX reg to 2nd parameter(=Y Coord)
	mov si, 160							; Set SI reg to number of bytes in 1 line
	mul si								; Multiply AX reg value and SI reg value to calculate address of Y coord
	mov di, ax							; Set DI reg to calculated address of Y coord

	; Calculate finally address of line by using X coord and multipling 2
	mov ax, word[bp + 4]				; Set AX reg to 1st parameter(=X Coord)
	mov si, 2							; Set SI reg to 2, number of bytes in 1 character
	mul si								; Multiply AX reg value and SI reg value to calculate address of X coord
	add di, ax							; Set DI reg to value of AX reg(=Final address of video memory)

	mov si, word[bp + 8]				; Set si reg to 3rd parameter(=Address of print message)

; Loop to print message
.MESSAGELOOP:
	mov cl, byte[si]					; Get 1 byte from address pointed by SI reg and set it to CL reg

	cmp cl, 0							; Compare 1 byte character and 0x0
	je .MESSAGEEND						; If value of character is 0x0, then goto .MESSAGEEND to finish

	mov byte [es:di], cl				; If not, mov value of CL reg to (value of ES) + DI
	
	add si, 1							; Add 1 to SI reg to point next character
	add di, 2							; Add 2 to DI reg to point next video memory addr for printing
										; Because video memory is composed of pair of character(1 byte) and attributes(1 byte)
										; to set character only, move by 2 bytes
	
	jmp .MESSAGELOOP					; call MESSAGELOOP recursively to print next character
	
.MESSAGEEND:
	pop dx								; Restore DX ~ ES Reg with value in stack temporarily
	pop cx								; 
	pop ax								;
	pop di								;
	pop si								;
	pop es								; Stack is FILO, pop reversly

	pop bp								; Restore Base Point Reg
	ret

MESSAGE1:
	db 'BBI OS Boot Loader Start~!!', 0	; Set last value as 0 to make .MESSAGELOOP knows the string is finished

DISKERRORMESSAGE:
	db 'DISK Error!', 0

IMAGELOADINGMESSAGE:
	db 'OS Image Loading...', 0

LOADINGCOMPLETEMESSAGE:
	db 'Complete!', 0

; Variables related with DISKREAD
SECTORNUMBER:
	db 0x02								; Sector number of OS image
HEADNUMBER:
	db 0x00								; Head number of OS image
TRACKNUMBER:
	db 0x00								; Track number of OS image
	
times 510 - ($ - $$) db 0x00			; $					Address of current line
										; $$				Source address of current section(.text)
										; $ - $$			Offset from current section(.text)
										; 510 - ($ - $$)	Distance from current offset to 510
										; db 0x00			Declare 1 byte and set 0
										; times				Repeat
										; Write 0x00 from current address to 510

db 0x55									; Declare 1 byte and set 0x55 at address 511
db 0xAA									; Declare 1 byte and set 0xAA at address 512
										; which means, it is boot sector
