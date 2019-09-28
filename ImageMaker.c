#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <io.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTES_OF_SECTOR 512

int AdjustInSectorSize(int fd, int srcSize);                // 현재 위치 부터 512 Bytes Aligned 위치 까지 0x00으로 채움
void WriteKernelInformation(int fd, int kernelSectorCount); // 부트 로더에 커널에 대한 정보 삽입
int CopyFile(int srcFD, int dstFD);                         // 입력 파일(src FD)의 내용을 출력 파일(dst FD)로 복사하고, 복사 된 크기를 반환

int main(int argc, char *argv[]) {
    int srcFD;
    int dstFD;

    int bootLoaderSize;
    int kernel32SectorCount;
    int srcSize;

    if(argc < 3) {
        fprintf(stderr, "[ERROR] ImageMaker.exe BootLoader.bin Kernel32.bin\n");
        exit(-1);
    }
#ifdef __APPLE__
    if( (dstFD = open("Disk.img", O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1) {
#else
    if( (dstFD = open("Disk.img", O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1) {
#endif
        fprintf(stderr, "[ERROR] Disk.img open failed\n");
        exit(-1);
    }

    /*
        부트 로더 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
    */
    printf("[INFO] Copy BootLoader to image file\n");
#ifdef __APPLE__
    if( (srcFD = open(argv[1], O_RDONLY)) == -1) {
#else
    if( (srcFD = open(argv[1], O_RDONLY | O_BINARY)) == -1) {
#endif
        fprintf(stderr, "[ERROR] %s open failed\n", argv[1]);
        exit(-1);
    }

    srcSize = CopyFile(srcFD, dstFD);
    close(srcFD);

    bootLoaderSize = AdjustInSectorSize(dstFD, srcSize);
    printf("[INFO] %s size = %d Bytes / Sector count = %d Bytes\n", argv[1], srcSize, bootLoaderSize);

    /*
        32bit 커널 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
    */
    printf("[INFO] Copy protected mode kernel to image file\n");
#ifdef __APPLE__
    if((srcFD = open(argv[2], O_RDONLY)) == -1) {
#else
    if((srcFD = open(argv[2], O_RDONLY | O_BINARY)) == -1) {
#endif
        fprintf(stderr, "[ERROR] %s open failed\n", argv[2]);
        exit(-1);
    }

    srcSize = CopyFile(srcFD, dstFD);
    close(srcFD);

    // 파일 크기를 섹터 크기인 512 바이트로 맞추기 위해 나머지 부분을 0x00으로 채움
    kernel32SectorCount = AdjustInSectorSize(srcFD, dstFD);
    close(srcFD);

    kernel32SectorCount = AdjustInSectorSize(srcFD, srcSize);
    printf("[INFO] %s size = %d Bytes / Sector count = %d Bytes\n", argv[2], srcSize, kernel32SectorCount);

    /*
        디스크 이미지에 커널 정보 갱신
    */
   printf("[INFO] Start to write kernel information\n");

   // 부트 섹터의 5번째 바이트 부터 커널에 대한 정보를 넣음
   WriteKernelInformation(dstFD, kernel32SectorCount);
   printf("[INFO] Image file create complete");

   close(dstFD);
   return 0;
}

// 현재 위치 부터 512 Bytes Aligned 위치 까지 0x00으로 채움
int AdjustInSectorSize(int fd, int srcSize) {
    int i;
    char ch;
    
    int adjustSizeToSector;
    int sectorCount;

    adjustSizeToSector = srcSize % BYTES_OF_SECTOR;
    ch = 0x00;

    if(adjustSizeToSector) {
        adjustSizeToSector = BYTES_OF_SECTOR - adjustSizeToSector;

        printf("[INFO] File size [%lu] and fill [%u] bytes\n", srcSize, adjustSizeToSector);
        for(i = 0; i < adjustSizeToSector; ++i) {
            write(fd, &ch, 1);
        }
    }
    else
        printf("[INFO] File size is aligned 512 bytes\n");

    sectorCount = (srcSize + adjustSizeToSector) / BYTES_OF_SECTOR;

    return sectorCount;
}

// 부트 로더에 커널에 대한 정보 삽입
void WriteKernelInformation(int dstFD, int kernelSectorCount) {
    unsigned short data;
    long pos;

    // 파일의 시작에서 5바이트 떨어진 위치가 커널의 총 섹터 수 정보를 의미
    pos = lseek(dstFD, (off_t)5, SEEK_SET);

    if(pos == -1) {
        fprintf(stderr, "lseek failed, return value = %d, errno = %d, %d\n", pos, errno, SEEK_SET);
        exit(-1);
    }

    data = (unsigned short)kernelSectorCount;
    write(dstFD, data, 2);

    printf("[INFO] Total sector count except boot loader [%d]\n", kernelSectorCount);
}

// 입력 파일(src FD)의 내용을 출력 파일(dst FD)로 복사하고, 복사 된 크기를 반환
int CopyFile(int srcFD, int dstFD) {
    int srcFileSize;
    int rCount, wCount;

    char buf[BYTES_OF_SECTOR];

    srcFileSize = 0;
    while(1) {
        rCount = read(srcFD, buf, sizeof(buf));
        wCount = write(dstFD, buf, rCount);

        if(rCount != wCount) {
            fprintf(stderr, "[ERROR] rCount != wCount.. \n");
            exit(-1);
        }

        srcFileSize += rCount;

        if(rCount != sizeof(buf)) 
            break;
    }

    return srcFileSize;
}
