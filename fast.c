#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>

#define FAST_BLOCK_SIZE 4096
#define NUM_OF_THREADS 64

struct threadArgs {
    int fd;
    size_t blockCount;
    off_t offset;
};

void* thrdRead(void* threadArgPtr) {
    struct threadArgs* args = threadArgPtr;
    unsigned int* buffer;
    unsigned int XORresult = 0;
    buffer = (unsigned int*) malloc(FAST_BLOCK_SIZE);
    if (!buffer) {
        printf("thrdRead buffer malloc failed!\n");
        exit(1);
    }
    size_t blockCount = args->blockCount;
    while (blockCount > 0) {
        ssize_t bytesRead = pread(args->fd, buffer, FAST_BLOCK_SIZE, args->offset);
        if (bytesRead == 0) {
            break;
        }
        args->offset += bytesRead;
        ssize_t numOfInt = bytesRead / 4;
        for (ssize_t i = 0; i < numOfInt; i++) {
            XORresult ^= buffer[i];
        }
        blockCount--;
    }
    free(buffer);
    pthread_exit((void*)(intptr_t)XORresult);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pthread_t* mThreads;
        struct stat fileStat;
        unsigned int finalResult = 0;

        mThreads = (pthread_t *) malloc(sizeof(pthread_t) * NUM_OF_THREADS);
        if (!mThreads) {
            printf("mThreads malloc failed!\n");
            exit(1);
        }
        struct threadArgs* thrdArgs = (struct threadArgs*) malloc(sizeof(struct threadArgs) * NUM_OF_THREADS);
        if (!thrdArgs) {
            printf("thrdArgs malloc failed!\n");
            exit(1);
        }
        unsigned int* thrdResults = (unsigned int*) malloc(sizeof(unsigned int) * NUM_OF_THREADS);
        if (!thrdResults) {
            printf("thrdResults malloc failed!\n");
            exit(1);
        }

        int fd = open(argv[1], O_RDONLY);
        fstat(fd, &fileStat);
        size_t blockCountPerThread = (fileStat.st_size / FAST_BLOCK_SIZE / NUM_OF_THREADS) + 1;
        off_t offset = blockCountPerThread * FAST_BLOCK_SIZE;

        for (int i = 0; i < NUM_OF_THREADS; i++) {
            thrdArgs[i].fd = fd;
            thrdArgs[i].blockCount = blockCountPerThread;
            thrdArgs[i].offset = i * offset;
            pthread_create(&mThreads[i], NULL, thrdRead, (void*)&thrdArgs[i]);
        }
        for (int i = 0; i < NUM_OF_THREADS; i++) {
            pthread_join(mThreads[i], (void**)&thrdResults[i]);
            finalResult ^= thrdResults[i];
        }

        free(mThreads);
        free(thrdArgs);
        free(thrdResults);
        close(fd);
        printf("XOR result is %08x\n", finalResult);
    }
    else {
        printf("Invalid Arguments. Please enter \"./fast <filename>\" only.\n");
        exit(1);
    }
    exit(0);
}
