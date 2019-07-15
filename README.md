# bbi-os
64bit Multicore OS using Intel x86_64 

## Welcome to bbi-os
bbi-os is derived OS from [MINT64 OS](http://www.mint64os.pe.kr/) and written by C & x86_64 assembly languages 

## Contributing to bbi-os
Contributions to bbi-os are welcomed and encouraged! Please contact us if you want to contribute

## Getting Started
To build from source, you will need about 
- [binutils](https://ftp.gnu.org/gnu/binutils/)
- [gcc](https://ftp.gnu.org/gnu/gcc/)
- [nasm](https://www.nasm.us/)
- [qemu@0.10.4](https://download-mirror.savannah.gnu.org/releases/qemu/)
  ※ Disk I/O mechanism of qemu@>0.10.4 is diffrenct from qemu@0.10.4, please use qemu@0.10.4

## System Requirements
Windows, Linux, MacOS are the current supported host development operating systems.

## Cross-compiling gcc for MacOS
Cross-compiling gcc for macOS is very difficult. <br>
You can download cross compiled gcc from [here](http://crossgcc.rts-software.org/doku.php?id=compiling_for_linux)

## Build Tool
We will use Make as build tool, please install make for build <br>
If your host development OS is windows, install mingw64(I recommend using [chocolatey](https://chocolatey.org/))
- choco install mingw
