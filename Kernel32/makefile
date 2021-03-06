MAKE = make
NASM = nasm
NASM32 = nasm -f elf32 
GCC32 = x86_64-pc-linux-gcc -c -m32 -ffreestanding
LD32 = x86_64-pc-linux-ld -melf_i386 -T ../../elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = tmp
SRCDIR = src
INCDIR = ../include

all: clean prepare Kernel32.bin

prepare:
	mkdir -p $(OBJDIR)

$(OBJDIR)/EntryPoint.bin: $(SRCDIR)/EntryPoint.s
	$(NASM) -o $@ $<

dep:
	@echo === Make Dependency File ===
	$(MAKE) -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	$(MAKE) -C $(OBJDIR) -f ../makefile Kernel32.elf

$(OBJDIR)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJDIR)/Kernel32.elf $@

Kernel32.bin: $(OBJDIR)/EntryPoint.bin $(OBJDIR)/Kernel32.elf.bin
	cat $^ > $@

clean:
	rm -f *.bin
	rm -f $(OBJDIR)/*.*

TARGET = Main
SRC = $(wildcard ../$(SRCDIR)/*.c)
ASM = $(wildcard ../$(SRCDIR)/*.asm)
OBJ = $(subst $(TARGET).o, , $(notdir $(patsubst %.c, %.o, $(SRC))))
ASMOBJ = $(notdir $(patsubst %.asm, %.o, $(ASM)))

%.o : ../$(SRCDIR)/%.c
	$(GCC32) -c -I$(INCDIR) $<

%.o : ../$(SRCDIR)/%.asm
	$(NASM32) -o $@ $<

InternalDependency:
	$(GCC32) -MM $(SRC) -I$(INCDIR) > Dependency.dep

Kernel32.elf: $(TARGET).o $(OBJ) $(ASMOBJ)
	$(LD32) -o $@ $^

ifeq (Dependency dep, $(wildcard Dependency dep))
include Dependency.dep
endif
