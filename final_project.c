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

int num_threads = 2;
pthread_spinlock_t spinlock1, spinlock2;

struct readThread_args{
    int fd;
    int* buffer1;
    int* buffer2;
    size_t blockSize;
};

struct xorThread_args{
    int* buffer1;
    int* buffer2;
    size_t blockSize;
    size_t blockCount;
    unsigned int xor_result;
};

struct multithreaded_read_argstruct {
    int tid;
    int* buffer;
    size_t buffer_size;
    unsigned int xor_result;
};

void *parallel_xor(void* args){
    struct multithreaded_read_argstruct* threadArgs = (struct multithreaded_read_argstruct*) args;
    int tid = threadArgs->tid;
    unsigned int *buffer = threadArgs->buffer;
    size_t buffer_size = threadArgs->buffer_size;
    threadArgs->xor_result = 0;
    for(size_t i=0; i<buffer_size; i++){
        threadArgs->xor_result ^= buffer[i];
    }
    pthread_exit(NULL);
}

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

unsigned int myReadFast(int fd) {
    unsigned int* buffer;
    unsigned int result = 0;
    size_t BLOCK_SIZE = 1048576;                                    // draft: blockSize 1048576 bytes (1 MiB)
    buffer = (unsigned int*) malloc(BLOCK_SIZE);
    while (1) {
        ssize_t byteRead = read(fd, buffer, BLOCK_SIZE);
        if (byteRead == 0) {
            break;
        }
        else if (byteRead < 0) {
            printf("Error: read error encountered in myReadFast, blockSize = %lu!\n", BLOCK_SIZE);
            break;
        }
        for (int i = 0; i < byteRead / 4; i++) {
            result ^= buffer[i];
        }
    }
    free(buffer);
    return result;
}

void *run_readThread(void* args){
    struct readThread_args * readThreadArgs = (struct readThread_args*) args;
    ssize_t byteRead;
    while (1) {
        pthread_spin_lock(&spinlock1);
        byteRead = read(readThreadArgs->fd, readThreadArgs->buffer1, readThreadArgs->blockSize);
        if(byteRead == 0) {
            pthread_spin_unlock(&spinlock1);
            break;
        }
        else if (byteRead < 0) {
            printf("Read error encountered in optimized read!\n");
            pthread_spin_unlock(&spinlock1);
            break;
        }
        pthread_spin_unlock(&spinlock1);

        pthread_spin_lock(&spinlock2);
        byteRead = read(readThreadArgs->fd, readThreadArgs->buffer2, readThreadArgs->blockSize);
        if(byteRead == 0) {
            pthread_spin_unlock(&spinlock2);
            break;
        }
        else if (byteRead < 0) {
            printf("Read error encountered in optimized read!\n");
            pthread_spin_unlock(&spinlock2);
            break;
        }
        pthread_spin_unlock(&spinlock2);
    }
    pthread_exit(NULL);
}

void *run_xorThread(void* args){
    struct xorThread_args * xorThreadArgs = (struct xorThread_args*) args;
    size_t totalIterations = (xorThreadArgs->blockCount/2);

    for(size_t count=0; count< totalIterations; count++){
        pthread_spin_lock(&spinlock1);
        for (int i = 0; i < xorThreadArgs->blockSize / 4; i++) {
            xorThreadArgs->xor_result ^= xorThreadArgs->buffer1[i];
        }
        pthread_spin_unlock(&spinlock1);

        pthread_spin_lock(&spinlock2);
        for (int i = 0; i < xorThreadArgs->blockSize / 4; i++) {
            xorThreadArgs->xor_result ^= xorThreadArgs->buffer2[i];
        }
        pthread_spin_unlock(&spinlock2);
    }

    if(xorThreadArgs->blockCount%2){
        pthread_spin_lock(&spinlock1);
        for (int i = 0; i < xorThreadArgs->blockSize / 4; i++) {
            xorThreadArgs->xor_result ^= xorThreadArgs->buffer1[i];
        }
        pthread_spin_unlock(&spinlock1);
    }

    pthread_exit(NULL);
}

unsigned int optimizedRead(char* filename, size_t blockSize) {

    blockSize = 4194304;
    
    pthread_t readThread, xorThread;
    struct readThread_args readThreadData;
    struct xorThread_args xorThreadData;

    pthread_spin_init(&spinlock1, 0);
    pthread_spin_init(&spinlock2, 0);

    int* buffer1 = (int*) malloc(blockSize);
    memset(buffer1, 0, blockSize);
    int* buffer2 = (int*) malloc(blockSize);
    memset(buffer2, 0, blockSize);

    int fd = open(filename, O_RDONLY | __O_LARGEFILE);
    struct stat fileStat;
    fstat(fd, &fileStat);
    size_t blockCount = fileStat.st_size/blockSize;

    readThreadData.fd = fd;
    readThreadData.blockSize = blockSize;
    readThreadData.buffer1 = buffer1;
    readThreadData.buffer2 = buffer2;

    xorThreadData.blockCount = blockCount;
    xorThreadData.blockSize = blockSize;
    xorThreadData.buffer1 = buffer1;
    xorThreadData.buffer2 = buffer2;
    xorThreadData.xor_result = 0;

    if(posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_MADV_WILLNEED)<0){
        printf("fadvice error");
        return 0;
    }

    pthread_create(&readThread, NULL, &run_readThread, (void*) &readThreadData);
    pthread_create(&xorThread, NULL, &run_xorThread, (void*) &xorThreadData);

    pthread_join(readThread, NULL);
    pthread_join(xorThread, NULL);

    unsigned int result = xorThreadData.xor_result;
    
    free(buffer1);
    free(buffer2);
    close(fd);
    return result;
}

unsigned int optimizedRead2(char* filename, size_t blockSize) {

    int fd = open(filename, O_RDONLY | __O_LARGEFILE);

    struct stat fileStat;
    fstat(fd, &fileStat);
    int* memoryPtr = (int*) mmap(NULL, fileStat.st_size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
    if(memoryPtr == (int*) -1){
        printf("mmap error");
        return 0;
    }

    if(madvise(memoryPtr, fileStat.st_size, POSIX_MADV_SEQUENTIAL | MADV_WILLNEED)<0){
        printf("fadvice error");
        return 0;
    }

    unsigned int result = 0;
    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t)*num_threads);
    size_t perThreadBufferSize = (fileStat.st_size/(sizeof (unsigned int)))/num_threads;
    struct multithreaded_read_argstruct threadsData[num_threads];
    //(struct multithreaded_read_argstruct**) malloc(numOfThreads * sizeof(struct multithreaded_read_argstruct));
    for(int tid=0; tid<num_threads; tid++){
        threadsData[tid].tid = tid;
        threadsData[tid].buffer = memoryPtr + tid*perThreadBufferSize;
        threadsData[tid].buffer_size = perThreadBufferSize;
        pthread_create(&threads[tid], NULL, &parallel_xor, (void*) &threadsData[tid]);
    }
    for(int tid=0; tid<num_threads; tid++){
        pthread_join(threads[tid], NULL);
        result ^= threadsData[tid].xor_result;
    }
    // for(ssize_t index=0; index<fileStat.st_size/4; index++){
    //     result ^= memoryPtr[index];
    // }
    close(fd);
    return result;
}

/*double measureReadTime(char* filename, size_t blockSize){
    clock_t start, end;
    int fd = open(filename, O_RDONLY);
    start = clock();
    unsigned int xorAnswer = myRead(fd, blockSize);
    end = clock();
    double timeNeeded = ((double)(end-start) / (double)CLOCKS_PER_SEC);
    close(fd);
    printf("XOR Answer is %08x\n", xorAnswer);
    return timeNeeded;
}*/

double measureOptimizedReadTime(char* filename, size_t blockSize, int readVersion){
    clock_t start, end;
    unsigned int xorAnswer;
    start = clock();
    switch (readVersion)
    {
        case 1:
            xorAnswer = optimizedRead(filename, blockSize);
            break;
        case 2:
            xorAnswer = optimizedRead2(filename, blockSize);
            break;
        default:
            break;
    }
    end = clock();
    double timeNeeded = ((double)(end-start) / (double)CLOCKS_PER_SEC);
    printf("XOR Answer is %u\n", xorAnswer);
    return timeNeeded;
}

/*unsigned long long findFileSize(size_t blockSize){                          // return reasonable fileSize (in bytes) to test with given blockSize
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
    remove(filename);

    return blockCount;
}*/

/*double getPerformance(char* filename, size_t blockSize, int readVersion){
    //return the MiB/s of the read operation by the specified block size
    int fd = open(filename, O_RDONLY);
    struct stat fileStat;
    fstat(fd, &fileStat);
    size_t fileSize = fileStat.st_size/(1024*1024);
    double timeNeeded;
    switch (readVersion)
    {
        case 0:
            timeNeeded = measureReadTime(filename,blockSize);
            break;
        case 1:
            timeNeeded = measureOptimizedReadTime(filename,blockSize,1);
            break;
        case 2:
            timeNeeded = measureOptimizedReadTime(filename,blockSize,2);
            break;
        case 3:
            timeNeeded = measureOptimizedReadTime(filename,blockSize,3);
            break;
        default:
            return 0;
            break;
    }
    printf("Read time: %f\n", timeNeeded);
    double MiBPerSec = (double)(fileSize/timeNeeded);
    return MiBPerSec;
}*/

int main(int argc, char *argv[]) {
    int fd;
    char mode = 'x';
    size_t blockSize, blockCount;                           // deleted testBlockSize and use blockSize instead
    //srand(time(0));

    if (argc == 2) {                                        // calls by fast script
        fd = open(argv[1], O_RDONLY);
        unsigned int result = myReadFast(fd);
        printf("XOR result of file %s is %08x", argv[1], result);
    }
    /*if (argc == 2) {                                        // print appropirate file size of given blockSize on stdout
        if (sscanf(argv[1], "%lu", &blockSize) <= 0) {      // store stdin arg into blockSize
            printf("Invalid arg provided.\n");
        }
        unsigned long long fileSize = findFileSize(blockSize) * blockSize;
        unsigned long long fileSizeMiB = fileSize / 1048576;
        printf("Input BlockSize: %lu, Reasonable FileSize: %llu bytes (%llu MiB)\n", blockSize, fileSize, fileSizeMiB);
    }*/
    else if (argc == 5) {                                   // normal mode - read or write
        sscanf(argv[2], "-%c", &mode);
        sscanf(argv[3], "%lu", &blockSize);
        sscanf(argv[4], "%lu", &blockCount);
        if (mode == 'r' || mode == 'R') {                   // read mode
            //double MiBPerSec = getPerformance(argv[1], blockSize, 0);
            //printf("BlockSize: %lu, Read speed (MiB/sec): %f\n", blockSize, MiBPerSec);
            fd = open(argv[1], O_RDONLY);
            unsigned int result = myRead(fd, blockSize, blockCount);
            close(fd);
            printf("XOR result is %08x\n", result);
        }
        else if (mode == 'w' || mode == 'W') {              // write mode
            fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
            myWrite(fd, blockSize, blockCount);          // write random alphabet letters into file
            close(fd);
        }
        /*else if (mode == 'o'){
            //optimized read
            double MiBPerSec = getPerformance(argv[1], blockSize, 1);
            printf("BlockSize: %lu, Optimized read speed (MiB/sec): %f\n", blockSize, MiBPerSec);
        }*/
    }
    else {
        printf("Invalid arg provided.\n");
    }

    exit(0);
}