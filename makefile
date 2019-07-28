all: BootLoader.bin Kernel32.bin Disk.img

BootLoader.bin:
	@echo
	@echo ==================== Build BootLoader ====================
	@echo

	make -C BootLoader

	@echo
	@echo ====================  Build Complete  ====================
	@echo

Kernel32.bin:
	@echo
	@echo ====================  Build Kernel32  ====================
	@echo

	make -C Kernel32

	@echo
	@echo ====================  Build Complete  ====================
	@echo

Disk.img: BootLoader/BootLoader.bin Kernel32/Kernel32.bin
	@echo
	@echo ==================== Build DiskImg ====================
	@echo

	cat $^ > Disk.img

	@echo
	@echo ==================== Build Complete ====================
	@echo

clean:
	make -C BootLoader clean
	make -C Kernel32 clean
	rm -f Disk.img
