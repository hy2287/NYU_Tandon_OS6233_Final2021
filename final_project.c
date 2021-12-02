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

void createRandomFile(char* filename, size_t blockSize, size_t blockCount){
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
    char* buffer = (char*) malloc(blockSize);
    int rnd = open("/dev/urandom", O_RDONLY);
    for(;blockCount>0;blockCount--){
        if(read(rnd, buffer, blockSize)<0){
            printf("Error reading random stream in createRandomFile\n");
            break;
        }
        if (write(fd, buffer, blockSize) < 0) {
            printf("Write error encountered in createRandomFile\n");
            break;
        }
    }
    close(rnd);
    close(fd);
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

double measureOptimizedReadTime(char* filename, size_t blockSize){
    clock_t start, end;
    start = clock();
    unsigned int xorAnswer = optimizedRead2(filename, blockSize);
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
            double timeNeeded = measureReadTime(argv[1], blockSize);
            printf("Read time: %f\n", timeNeeded);
        }
        else if (mode == 'w' || mode == 'W') {              // write mode
            fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
            myWrite(fd, blockSize, blockCount, 1);          // write random alphabet letters into file
            close(fd);
        }
        else if (mode == 'c' || mode == 'C') {              // more efficiently write randomized data into file
            createRandomFile(argv[1],blockSize,blockCount);
        }
        else if (mode == 'o'){
            //optimized read
            double timeNeeded = measureOptimizedReadTime(argv[1], blockSize);
            printf("Optimized read time: %f\n", timeNeeded);
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
    else if (mode == 'o'){
        //optimized read
        xorAnswer = optimizedRead(argv[1], blockSize);
        printf("Optimized read: XOR Answer is %d\n", xorAnswer);
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