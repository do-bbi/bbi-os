MAKE=make

all: BootLoader.bin Kernel32.bin Kernel64.bin Disk.img

BootLoader.bin:
	@echo
	@echo ==================== Build BootLoader ====================
	@echo

	$(MAKE) -C BootLoader

	@echo
	@echo ====================  Build Complete  ====================
	@echo $(MAKE)

Kernel32.bin:
	@echo
	@echo ====================  Build Kernel32 $(MAKE) ====================
	@echo

	$(MAKE) -C Kernel32

	@echo
	@echo ====================  Build Complete  ====================
	@echo

Kernel64.bin:
	@echo
	@echo ====================  Build Kernel64 $(MAKE) ====================
	@echo

	$(MAKE) -C Kernel64

	@echo
	@echo ====================  Build Complete  ====================
	@echo

Disk.img: BootLoader/BootLoader.bin Kernel32/Kernel32.bin Kernel64/Kernel64.bin
	@echo
	@echo ==================== Build DiskImg ====================
	@echo

	# cat $^ > Disk.img
	./ImageMaker $^

	@echo
	@echo ==================== Build Complete ====================
	@echo

clean:
	$(MAKE) -C BootLoader clean
	$(MAKE) -C Kernel32 clean
	$(MAKE) -C Kernel64 clean
	rm -f Disk.img
