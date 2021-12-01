#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

void myWrite(int fd, int blockSize, int blockCount) {
    char* buffer;
    buffer = (char*) malloc(blockSize);
    for (int i = 0; i < blockCount; i++) {
        for (int i = 0; i < blockSize; i++) {
            buffer[i] = (rand() % 26) + 'a';
        }
        write(fd, buffer, blockSize);
    }
    free(buffer);
    return;
}

unsigned int myRead(int fd, int blockSize) {
    int* buffer;
    unsigned int result = 0;
    buffer = (int*) malloc(blockSize);
    while (read(fd, buffer, blockSize) > 0) {
        for (int i = 0; i < blockSize / 4; i++) {
            //printf("buffer[i] is %d\n", buffer[i]);
            result ^= buffer[i];
        }
    }
    free(buffer);
    return result;
}

double findFileSize(int blockSize){
    int blockCount = 1;
    clock_t start, end;
    double timeNeeded = 0;
    char* filename = "tempfile";
    int fd;

    while(timeNeeded<5){
        blockCount*=2;
        fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        myWrite(fd,blockSize*blockCount,1);
        close(fd);

        fd = open(filename, O_RDONLY);
        start=clock();
        myRead(fd,blockSize);
        end=clock();
        timeNeeded = ((double) (end-start)) / CLOCKS_PER_SEC;

        printf("block size: %d, block count: %d, time: %fs\n", blockSize, blockCount, timeNeeded);

        close(fd);
    }

    remove(filename);
    return blockSize*blockCount;
}

int main(int argc, char *argv[]) {
    int fd, blockSize, blockCount;
    char mode = 'x';
    unsigned int xorAnswer = 0;

    sscanf (argv[2],"-%c", &mode);
    sscanf (argv[3],"%d", &blockSize);
    sscanf (argv[4],"%d", &blockCount);

    srand(time(0));

    if (mode == 'w' || mode == 'W') {
        fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        myWrite(fd, blockSize, blockCount);
        close(fd);
    }
    else if (mode == 'r' || mode == 'R'){
        fd = open(argv[1], O_RDONLY);
        xorAnswer = myRead(fd, blockSize);
        printf("XOR Answer is %d", xorAnswer);
        close(fd);
    }
    else {
        findFileSize(20);
    }

    exit(0);
}