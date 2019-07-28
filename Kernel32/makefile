all: Kernel32.bin

Kernel32.bin: Entrypoint.asm
	nasm -o Kernel32.bin $<

clean:
	rm -f Kernel32.bin
