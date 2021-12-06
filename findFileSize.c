#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>

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

double measureReadTime(char* filename, size_t blockSize, size_t blockCount){
    clock_t start, end;
    int fd = open(filename, O_RDONLY);
    start = clock();
    unsigned int xorAnswer = myRead(fd, blockSize, blockCount);
    end = clock();
    double timeNeeded = ((double)(end-start) / (double)CLOCKS_PER_SEC);
    close(fd);
    printf("XOR Answer is %08x\n", xorAnswer);
    return timeNeeded;
}

unsigned long long findFileSize(size_t blockSize){                          // return reasonable fileSize (in bytes) to test with given blockSize
    size_t blockCount = 1;
    clock_t start, end;
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
    printf("Block Size: %lu, Block Count: %lu, Time Used to Read: %f sec\n", blockSize, blockCount, timeNeeded);
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