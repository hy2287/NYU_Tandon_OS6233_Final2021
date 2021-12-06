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

struct jThreadArgs {
    unsigned int* begin;
    unsigned int* end;
    unsigned int* fileEndAddress;
};

void* myReadmt2(void* threadArgPtr) {
    struct jThreadArgs* args = threadArgPtr;
    unsigned int* begin = args->begin;
    if (begin >= args->fileEndAddress) {
        unsigned int XORresult = 0;
        //printf("thread exit early, thread begin is %p, fileEndAddress is %p\n", begin, args->fileEndAddress);
        pthread_exit((void*)(intptr_t)XORresult);
    }
    unsigned int* end = args->end;
    unsigned int XORresult = 0;
    while (begin < end) {
        XORresult ^= *begin;
        begin++;
    }
    pthread_exit((void*)(intptr_t)XORresult);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pthread_t* j_threads;
        struct stat fileStat;
        j_threads = (pthread_t *) malloc(sizeof(pthread_t) * NUM_OF_THREADS);
        struct jThreadArgs* jThArgs = malloc(sizeof(struct jThreadArgs) * NUM_OF_THREADS);

        int fd = open(argv[1], O_RDONLY);
        fstat(fd, &fileStat);
        unsigned int* fileInMem = mmap(NULL, fileStat.st_size, PROT_READ, MAP_SHARED, fd, 0);
        size_t intCount = fileStat.st_size / 4;
        size_t intCountPerThread = intCount / 64 + 1;
        //printf("fileInMem address is %p, fileEndAddress is %p\n", fileInMem, fileInMem + intCount);
        //printf("intCount is %lu, and intCountPerTHread is %lu\n", intCount, intCountPerThread);
        unsigned int finalResult = 0;
        unsigned int* results = (unsigned int*) malloc(sizeof(unsigned int) * NUM_OF_THREADS);

        for (int i = 0; i < NUM_OF_THREADS; i++) {
            jThArgs[i].begin = fileInMem + (i * intCountPerThread);
            jThArgs[i].end = fileInMem + (i + 1) * intCountPerThread;
            jThArgs[i].fileEndAddress = fileInMem + intCount;
            if (i == NUM_OF_THREADS -1) {
                jThArgs[i].end = fileInMem + intCount;
            }
            pthread_create(&j_threads[i], NULL, myReadmt2, (void*)&jThArgs[i]);
            //printf("thread %d created, begin is %p, and end is %p\n", i, jThArgs[i].begin, jThArgs[i].end);
        }
        for (int i = 0; i < NUM_OF_THREADS; i++) {
            pthread_join(j_threads[i], (void**)&results[i]);
            finalResult ^= results[i];
            //printf("thread %d joined, XOR result is %08x\n", i, results[i]);
        }

        free(j_threads);
        free(jThArgs);
        free(results);
        if (munmap(fileInMem, fileStat.st_size) < 0)
            printf("munmap failed.\n");
        
        printf("mmap XOR result is %08x\n", finalResult);
        close(fd);
    }
    exit(0);
}
