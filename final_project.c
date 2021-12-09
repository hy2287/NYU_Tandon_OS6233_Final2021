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

int main(int argc, char *argv[]) {
    int fd;
    char mode = 'x';
    size_t blockSize, blockCount;

    if (argc == 5) {
        sscanf(argv[2], "-%c", &mode);
        sscanf(argv[3], "%lu", &blockSize);
        sscanf(argv[4], "%lu", &blockCount);
        if (mode == 'r' || mode == 'R') {
            fd = open(argv[1], O_RDONLY);
            unsigned int result = myRead(fd, blockSize, blockCount);
            printf("XOR result of file %s is %08x\n", argv[1], result);
            close(fd);
        }
        else if (mode == 'w' || mode == 'W') {
            fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
            myWrite(fd, blockSize, blockCount);
            close(fd);
        }
        else {
            printf("Invalid arg provided.\n");
        }
    }
    else {
        printf("Invalid arg provided.\n");
    }
    exit(0);
}