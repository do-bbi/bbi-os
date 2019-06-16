[ORG 0x00]			; Start Address is 0x00
[BITS 16]			; Follow Codes are executed with 16 bit mode

SECTION .text		; text Section

mov ax, 0xB800		; copy 0xB800 to AX Reg
mov ds, ax			; copy Value of AX Reg to DS Segment Reg

mov byte [0x00], 'B'
mov byte [0x01], 0x4A
mov byte [0x02], 'B'
mov byte [0x03], 0x4A
mov byte [0x04], 'i'
mov byte [0x05], 0x4A
mov byte [0x06], ' '
mov byte [0x07], 0x4A
mov byte [0x08], 'O'
mov byte [0x09], 0x4A
mov byte [0x0A], 'S'
mov byte [0x0B], 0x4A
mov byte [0x0C], ' '
mov byte [0x0D], 0x4A
mov byte [0x0E], 'B'
mov byte [0x0F], 0x4A
mov byte [0x10], 'o'
mov byte [0x11], 0x4A
mov byte [0x12], 'o'
mov byte [0x13], 0x4A
mov byte [0x14], 't'
mov byte [0x15], 0x4A
mov byte [0x16], 'l'
mov byte [0x17], 0x4A
mov byte [0x18], 'o'
mov byte [0x19], 0x4A
mov byte [0x1A], 'a'
mov byte [0x1B], 0x4A
mov byte [0x1C], 'd'
mov byte [0x1D], 0x4A
mov byte [0x1E], 'e'
mov byte [0x1F], 0x4A
mov byte [0x20], 'r'
mov byte [0x21], 0x4A
mov byte [0x22], ' '
mov byte [0x23], 0x4A
mov byte [0x24], 'S'
mov byte [0x25], 0x4A
mov byte [0x26], 't'
mov byte [0x27], 0x4A
mov byte [0x28], 'a'
mov byte [0x29], 0x4A
mov byte [0x2A], 'r'
mov byte [0x2B], 0x4A
mov byte [0x2C], 't'
mov byte [0x2D], 0x4A
mov byte [0x2E], '!'
mov byte [0x2F], 0x4A
mov byte [0x30], '!'
mov byte [0x31], 0x4A
mov byte [0x32], '!'

jmp $				; Infinite Loop on current location

times 510 - ($ - $$) db 0x00	; $: Address of Current Line
								; $$: Start Address of Current Section(text)
								; $ - $$: Offset From Current Section
								; 510 - ( $ - $$ ): From Current Address To Address 510
								; db 0x00: Declare 1 Byte and Assign Value "0"
								; time: Loop Statement
								; From Current Address To Address 510, Write 0

db 0x55							; Declare 1 Byte and Assign Value "0x55"
db 0xAA							; Declare 1 Byte and Assign Value "0xAA"
								; By Writing 0x55 and 0xAA At Address 511 and 512, Mark this sector as Boot Sector
