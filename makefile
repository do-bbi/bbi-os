all: BootLoader Kernel32 Disk.img

BootLoader:
	@echo
	@echo ==================== Build BootLoader ====================
	@echo

	make -C BootLoader

	@echo
	@echo ====================  Build Complete  ====================
	@echo

Kernel32:
	@echo
	@echo ====================  Build Kernel32  ====================
	@echo

	make -C Kernel32

	@echo
	@echo ====================  Build Complete  ====================
	@echo

Disk.img: BootLoader Kernel32
	@echo
	@echo ==================== Build DiskImg ====================
	@echo

	cat BootLoader/Bootloader.bin Kernel32/VirtualOS.bin > Disk.img

	@echo
	@echo ==================== Build Complete ====================
	@echo

clean:
	make -C BootLoader clean
	make -C Kernel32 clean
	rm -f Disk.img