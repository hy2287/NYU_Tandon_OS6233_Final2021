#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include "readWrite.h"

double measureReadTime(char* filename, size_t blockSize, size_t blockCount){
    int fd = open(filename, O_RDONLY);
    time_t start = time(NULL);
    myRead(fd, blockSize, blockCount);
    double timeNeeded = (double) (time(NULL) - start);
    close(fd);
    return timeNeeded;
}

unsigned long long findFileSize(size_t blockSize){                          // return reasonable fileSize (in bytes) to test with given blockSize
    size_t blockCount = 1;
    double timeNeeded = 0;
    char* filename = "tempFile";
    int fd;

    while (timeNeeded < 5) {        // increasing blockCount (fileSize) until it takes more than 5 sec to read
        blockCount *= 2;
        if (blockCount <= 0) {
            printf("Overflow during findFileSize, blockCount: %lu\n",blockCount);
            break;
        }
        fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
        myWrite(fd, blockSize, blockCount);
        close(fd);
        timeNeeded = measureReadTime(filename, blockSize, blockCount);
    }
    printf("Block Size: %lu, Block Count: %lu, Time Used to Read: %.2f sec\n", blockSize, blockCount, timeNeeded);
    remove(filename);

    return blockCount;
}

int main(int argc, char *argv[]) {
    int fd;
    size_t blockSize, blockCount;                           

    if (argc == 2) {                                        
        if (sscanf(argv[1], "%lu", &blockSize) <= 0) {      
            printf("Invalid arg provided.\n");
        }
        unsigned long long fileSize = findFileSize(blockSize) * blockSize;
        unsigned long long fileSizeMiB = fileSize / 1048576;
        printf("Input BlockSize: %lu, Reasonable FileSize: %llu bytes (%llu MiB)\n", blockSize, fileSize, fileSizeMiB);
    }
    else {
        printf("Invalid arg provided.\n");
    }

    exit(0);
}