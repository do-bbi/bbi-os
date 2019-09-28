MAKE=make

all: BootLoader.bin Kernel32.bin Disk.img

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

Disk.img: BootLoader/BootLoader.bin Kernel32/Kernel32.bin
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
	rm -f Disk.img
