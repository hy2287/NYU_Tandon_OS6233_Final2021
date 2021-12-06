#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include "readWrite.h"

void myWrite(int fd, size_t blockSize, size_t blockCount) {
    char* buffer;
    buffer = (char*) malloc(blockSize);
    for (int i = 0; i < blockCount; i++) {
        if (write(fd, buffer, blockSize) < 0) {
            printf("Error: write error encountered in myWrite, blockSize = %lu, blockCount = %lu!\n", blockSize, blockCount);
            break;
        }
    }
    free(buffer);
    return;
}

unsigned int myRead(int fd, size_t blockSize, size_t blockCount) {
    unsigned int* buffer;
    unsigned int result = 0;
    struct stat fileStat;
    fstat(fd, &fileStat);

    long int totalBlockSize = blockSize * blockCount;
    if (fileStat.st_size < totalBlockSize) {
        printf("Error: file size (%ld bytes) is smaller than blockSize * blockCount (%lu bytes)\n", fileStat.st_size, totalBlockSize);
        exit(1);
    }
    else if (blockSize > 2147479552) {
        printf("Error: read blockSize cannot be greater than 2147479552\n");
        exit(1);
    }

    buffer = (unsigned int*) malloc(blockSize);
    while (blockCount > 0) {
        ssize_t bytesRead = read(fd, buffer, blockSize);
        if (bytesRead == 0)
            break;
        ssize_t numOfInt = bytesRead / 4;
        for (ssize_t i = 0; i < numOfInt; i++) {
            result ^= buffer[i];
        }
        blockCount--;
    }
    free(buffer);
    return result;
}