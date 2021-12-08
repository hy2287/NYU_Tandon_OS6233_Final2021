#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>

#define NUM_OF_THREADS 64
#define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB (30 << MAP_HUGE_SHIFT)

struct threadArgs {
    unsigned int* begin;
    unsigned int* end;
    unsigned int* fileEndAddress;
};

void* thrdRead(void* threadArgPtr) {
    struct threadArgs* args = threadArgPtr;
    unsigned int* begin = args->begin;
    unsigned int* end = args->end;
    unsigned int XORresult = 0;
    if (begin >= args->fileEndAddress) {
        pthread_exit((void*)(intptr_t)XORresult);
    }
    while (begin < end) {
        XORresult ^= *begin;
        begin++;
    }
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
        size_t intCount = fileStat.st_size / 4;
        size_t intCountPerThread = (intCount / NUM_OF_THREADS) + 1;
        unsigned int* fileBegAdd = mmap(NULL, fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        unsigned int* fileEndAdd = fileBegAdd + intCount;

        for (int i = 0; i < NUM_OF_THREADS; i++) {
            unsigned int* begin = fileBegAdd + (i * intCountPerThread);
            thrdArgs[i].begin = begin;
            thrdArgs[i].end = begin + intCountPerThread;
            thrdArgs[i].fileEndAddress = fileEndAdd;
            if (i == NUM_OF_THREADS -1) {
                thrdArgs[i].end = fileEndAdd;
            }
            pthread_create(&mThreads[i], NULL, thrdRead, (void*)&thrdArgs[i]);
        }
        for (int i = 0; i < NUM_OF_THREADS; i++) {
            pthread_join(mThreads[i], (void**)&thrdResults[i]);
            finalResult ^= thrdResults[i];
        }

        free(mThreads);
        free(thrdArgs);
        free(thrdResults);
        if (munmap(fileBegAdd, fileStat.st_size) < 0) {
            printf("munmap failed!\n");
        }
        close(fd);
        printf("mmap XOR result is %08x\n", finalResult);
    }
    else {
        printf("Invalid Arguments. Please enter \"./fast2 <filename\" only.\n");
        exit(1);
    }
    exit(0);
}
