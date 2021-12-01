#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

void myWrite(int fd, int blockSize, int blockCount, int randomized) {
    char* buffer;
    buffer = (char*) malloc(blockSize);
    for (int i = 0; i < blockCount; i++) {
        if(randomized){
            for (int i = 0; i < blockSize; i++) {
                buffer[i] = (rand() % 26) + 'a';
            }
        }
        if(write(fd, buffer, blockSize)<0){
            printf("Write error encountered!\n");
            break;
        }
    }
    free(buffer);
    return;
}

unsigned int myRead(int fd, int blockSize) {
    int* buffer;
    unsigned int result = 0;
    buffer = (int*) malloc(blockSize);
    while (1) {
        int byteRead = read(fd, buffer, blockSize);
        if(byteRead==0){
            break;
        }
        else if(byteRead<0){
            printf("Read error encountered!\n");
            break;
        }
        for (int i = 0; i < blockSize / 4; i++) {
            //printf("buffer[i] is %d\n", buffer[i]);
            result ^= buffer[i];
        }
    }
    free(buffer);
    return result;
}

double measureReadTime(char* filename, int blockSize){
    clock_t start, end;
    int fd = open(filename, O_RDONLY);
    start=clock();
    myRead(fd,blockSize);
    end=clock();
    double timeNeeded = ((double) (end-start)) / CLOCKS_PER_SEC;
    close(fd);
    return timeNeeded;
}

double findFileSize(int blockSize){
    int blockCount = 1;
    clock_t start, end;
    double timeNeeded = 0;
    char* filename = "tempfile";
    int fd;

    while(timeNeeded<5){
        blockCount*=2;
        if(blockCount<=0){
            printf("overflow during findFileSize: %d\n",blockCount);
            break;
        }
        fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        myWrite(fd,blockSize*blockCount,1,0);
        close(fd);

        timeNeeded = measureReadTime(filename, blockSize);
        //printf("block size: %d, block count: %d, time: %fs\n", blockSize, blockCount, timeNeeded);
    }

    remove(filename);
    return blockSize*blockCount;
}

double getPerformance(int blockSize){
    //return the MiB/s of the read operation by the specified block size
    double desiredFileSize = findFileSize(blockSize);
    int blockCount = (int) desiredFileSize/blockSize;
    char* filename = "tempfile_performance";
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
    myWrite(fd,blockSize*blockCount,1,0);
    close(fd);
    double timeNeeded = measureReadTime(filename,blockSize);
    double MiBPerSec = desiredFileSize/(1024*1024*timeNeeded);
    remove(filename);
    return MiBPerSec;
}

int main(int argc, char *argv[]) {
    int fd, blockSize, blockCount, testBlockSize;
    char mode = 'x';
    unsigned int xorAnswer = 0;

    if(argc==5){
        sscanf (argv[2],"-%c", &mode);
        sscanf (argv[3],"%d", &blockSize);
        sscanf (argv[4],"%d", &blockCount);
    }

    srand(time(0));

    if (mode == 'w' || mode == 'W') {
        fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        myWrite(fd, blockSize, blockCount,1);
        close(fd);
    }
    else if (mode == 'r' || mode == 'R'){
        fd = open(argv[1], O_RDONLY);
        xorAnswer = myRead(fd, blockSize);
        printf("XOR Answer is %d\n", xorAnswer);
        close(fd);
    }
    else if (argc==2){
        if(sscanf (argv[1],"%d", &testBlockSize)==0){
            printf("Invalid arg provided.\n");
        }
        double desiredFileSize = findFileSize(testBlockSize)/(1024*1024);
        printf("block size: %d, desired filesize (in MiB): %f\n", testBlockSize, desiredFileSize);
    }
    else if(argc==1){
        printf("blocksize: %d, performance: %f MiB/s\n", 1, getPerformance(1));
        printf("blocksize: %d, performance: %f MiB/s\n", 10, getPerformance(10));
        printf("blocksize: %d, performance: %f MiB/s\n", 100, getPerformance(100));
    }
    else{
        printf("Invalid arg provided.\n");
    }
    exit(0);
}