#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>

int num_threads = 4;
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

unsigned int optimizedRead(char* filename, size_t blockSize) {

    int fd = open(filename, O_RDONLY | __O_LARGEFILE);

    struct stat fileStat;
    fstat(fd, &fileStat);
    mmap(NULL, fileStat.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if(posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL)<0){
        printf("fadvice error");
        return 0;
    }

    int* buffer;
    unsigned int result = 0;
    buffer = (int*) malloc(blockSize);
    while (1) {
        ssize_t byteRead = read(fd, buffer, blockSize);
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

double measureReadTime(char* filename, size_t blockSize){
    clock_t start, end;
    int fd = open(filename, O_RDONLY);
    start = clock();
    unsigned int xorAnswer = myRead(fd, blockSize);
    end = clock();
    double timeNeeded = ((double)(end-start) / (double)CLOCKS_PER_SEC);
    close(fd);
    printf("XOR Answer is %u\n", xorAnswer);
    return timeNeeded;
}

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
    remove(filename);

    return blockCount;
}

double getPerformance(char* filename, size_t blockSize, int readVersion){
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
}

int main(int argc, char *argv[]) {
    int fd;
    char mode = 'x';
    size_t blockSize, blockCount;                           // deleted testBlockSize and use blockSize instead
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
            double MiBPerSec = getPerformance(argv[1], blockSize, 0);
            printf("BlockSize: %lu, Read speed (MiB/sec): %f\n", blockSize, MiBPerSec);
        }
        else if (mode == 'w' || mode == 'W') {              // write mode
            fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
            myWrite(fd, blockSize, blockCount, 1);          // write random alphabet letters into file
            close(fd);
        }
        else if (mode == 'o'){
            //optimized read
            double MiBPerSec = getPerformance(argv[1], blockSize, 1);
            printf("BlockSize: %lu, Optimized read speed (MiB/sec): %f\n", blockSize, MiBPerSec);
        }
    }
    else {
        printf("Invalid arg provided.\n");
    }

    exit(0);
}