qemu-system-x86_64 -M pc -m 64 -rtc base=localtime \
  -drive format=raw,file=Disk.img,if=floppy \
  -drive format=raw,file=HDD.img,if=ide \
  -boot c -monitor stdio 