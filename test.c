#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

void myWrite(int fd, size_t blockSize, size_t blockCount, int randomized) {
    char* buffer;
    buffer = (char*) malloc(blockSize);
    for (int i = 0; i < blockCount; i++) {
        if(randomized){
            for (int i = 0; i < blockSize; i++) {
                buffer[i] = (rand() % 26) + 'a';
            }
        }
        if (write(fd, buffer, blockSize) < 0) {
            printf("Write error encountered in myWrite, blockSize = %lu, blockCount = %lu!\n", blockSize, blockCount);
            break;
        }
    }
    free(buffer);
    return;
}

unsigned int myRead(int fd, size_t blockSize) {
    int* buffer;
    unsigned int result = 0;
    buffer = (int*) malloc(blockSize);
    while (1) {
        ssize_t byteRead = read(fd, buffer, blockSize);
        if(byteRead == 0) {
            break;
        }
        else if (byteRead < 0) {
            printf("Read error encountered in myRead, blockSize = %lu!\n", blockSize);
            break;
        }
        for (int i = 0; i < blockSize / 4; i++) {
            // printf("buffer[i] is %d\n", buffer[i]);
            result ^= buffer[i];
        }
    }
    free(buffer);
    return result;
}

double measureReadTime(char* filename, size_t blockSize){
    clock_t start, end;
    int fd = open(filename, O_RDONLY);
    start = clock();
    myRead(fd, blockSize);
    end = clock();
    double timeNeeded = ((double)(end-start) / (double)CLOCKS_PER_SEC);
    close(fd);
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
        myWrite(fd, blockSize, blockCount, 0);
        close(fd);
        timeNeeded = measureReadTime(filename, blockSize);
    }
    printf("Block Size: %lu, Block Count: %lu, Time Used to Read: %f sec\n", blockSize, blockCount, timeNeeded);
    //remove(filename);

    return blockCount;
}

double getPerformance(size_t blockSize){
    //return the MiB/s of the read operation by the specified block size
    unsigned long long desiredFileSize = findFileSize(blockSize);
    size_t blockCount = (size_t) (desiredFileSize / blockSize);
    char* filename = "tempfile_performance";
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
    myWrite(fd, blockSize, blockCount, 0);
    close(fd);
    double timeNeeded = measureReadTime(filename,blockSize);
    double MiBPerSec = (double)(desiredFileSize/(unsigned long long)(1024*1024*timeNeeded));
    remove(filename);
    return MiBPerSec;
}

int main(int argc, char *argv[]) {
    int fd;
    char mode = 'x';
    size_t blockSize, blockCount;                           // deleted testBlockSize and use blockSize instead
    unsigned int xorAnswer = 0;
    srand(time(0));

    if (argc == 2) {                                        // print appropirate file size of given blockSize on stdout
        if (sscanf(argv[1], "%lu", &blockSize) <= 0) {      // store stdin arg into blockSize
            printf("Invalid arg provided.\n");
        }
        unsigned long long fileSize = findFileSize(blockSize) * blockSize;
        unsigned long long fileSizeMiB = fileSize / 1048576;
        printf("Input BlockSize: %lu, Reasonable FileSize: %llu bytes (%llu MiB)\n", blockSize, fileSize, fileSizeMiB);
    }
    else if (argc == 5) {                                   // normal mode - read or write
        sscanf (argv[2], "-%c", &mode);
        sscanf (argv[3], "%lu", &blockSize);
        sscanf (argv[4], "%lu", &blockCount);
        if (mode == 'r' || mode == 'R') {                   // read mode
            fd = open(argv[1], O_RDONLY);
            xorAnswer = myRead(fd, blockSize);
            printf("XOR Answer is %u\n", xorAnswer);
        }
        else if (mode == 'w' || mode == 'W') {              // write mode
            fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
            myWrite(fd, blockSize, blockCount, 1);          // write random alphabet letters into file
            close(fd);
        }
    }
    else {
        printf("Invalid arg provided.\n");
    }

    exit(0);



    /*if(argc==5){
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
        unsigned long long desiredFileSize = (unsigned long long) findFileSize(testBlockSize)*(unsigned long long) testBlockSize;
        unsigned long long desiredFileSizeMiBs = (desiredFileSize)/(1024*1024);
        printf("block size: %d, desired filesize (in MiB): %llu\n", testBlockSize, desiredFileSizeMiBs);
    }
    else if(argc==1){
        size_t blockSizesToTest[11] = {1, 4, 16, 256, 1024, 4096, 10240, 102400, 1024*1024, 4*1024*1024, 4294967295};
        printf("blocksize: %d, performance: %f MiB/s\n", 1, getPerformance(1));
        printf("blocksize: %d, performance: %f MiB/s\n", 10, getPerformance(10));
        printf("blocksize: %d, performance: %f MiB/s\n", 100, getPerformance(100));
    }
    else{
        printf("Invalid arg provided.\n");
    }
    exit(0);*/
}