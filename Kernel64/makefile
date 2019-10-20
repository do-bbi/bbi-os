MAKE=make
NASM64 = nasm -f elf64
GCC64 = x86_64-pc-linux-gcc -c -m64 -ffreestanding
LD64 = x86_64-pc-linux-ld -melf_x86_64 -T ../../elf_x86_64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = tmp
SRCDIR = src

all: clean prepare Kernel64.bin

prepare:
	mkdir -p $(OBJDIR)

EntryPoint.o: ../$(SRCDIR)/EntryPoint.s
	$(NASM64) -o $@ $<

dep:
	@echo === Make Dependency File ===
	$(MAKE) -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	$(MAKE) -C $(OBJDIR) -f ../makefile Kernel64.elf

Kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJDIR)/Kernel64.elf $@

clean:
	rm -f *.bin
	rm -f $(OBJDIR)/*.*

TARGET = EntryPoint
INCDIR = ../../Kernel32/src

LDFLAGS = $(foreach dir, $(INCDIR), -I$(dir))

SRC = $(wildcard ../$(SRCDIR)/*.c)
ASM = $(wildcard ../$(SRCDIR)/*.asm)
OBJ = $(subst $(TARGET).o, , $(notdir $(patsubst %.c, %.o, $(SRC))))
ASMOBJ = $(notdir $(patsubst %.asm, %.o, $(ASM)))

%.o : ../$(SRCDIR)/%.c
	$(GCC64) $(LDFLAGS) -c $<

%.o : ../$(SRCDIR)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) $(LDFLAGS) -MM $(SRC) > Dependency.dep

Kernel64.elf: $(TARGET).o $(OBJ) $(ASMOBJ)
	$(LD64) -o $@ $^

ifeq (Dependency dep, $(wildcard Dependency dep))
include Dependency.dep
endif
