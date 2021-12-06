

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>

#define FAST_BLOCK_SIZE 65536
#define NUM_OF_THREADS 64

struct jThreadArgs {
    int fd;
    size_t blockCount;
    off_t offset;
};

void* myReadmt(void* threadArgPtr) {
    struct jThreadArgs* args = threadArgPtr;
    unsigned int* buffer;
    unsigned int XORresult = 0;
    buffer = (unsigned int*) malloc(FAST_BLOCK_SIZE);
    size_t blockCount = args->blockCount;
    while (blockCount > 0) {
        ssize_t bytesRead = pread(args->fd, buffer, FAST_BLOCK_SIZE, args->offset);
        args->offset += bytesRead;
        if (bytesRead == 0)
            break;
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
    if (argc == 2) {                                        // calls by fast script
        pthread_t* j_threads;
        struct stat fileStat;

        j_threads = (pthread_t *) malloc(sizeof(pthread_t) * NUM_OF_THREADS);
        struct jThreadArgs* jThArgs = malloc(sizeof(struct jThreadArgs) * NUM_OF_THREADS);
        if (!j_threads) {
            printf("j_threads malloc failed\n");
            exit(1);
        }
        int fd = open(argv[1], O_RDONLY);
        fstat(fd, &fileStat);
        unsigned int finalResult = 0;
        unsigned int* results = (unsigned int*) malloc(sizeof(unsigned int) * NUM_OF_THREADS);

        size_t blockCountPerThread = (fileStat.st_size / FAST_BLOCK_SIZE / NUM_OF_THREADS) + 1;
        off_t offset = blockCountPerThread * FAST_BLOCK_SIZE;

        for (int i = 0; i < NUM_OF_THREADS; i++) {
            jThArgs[i].fd = fd;
            jThArgs[i].blockCount = blockCountPerThread;
            jThArgs[i].offset = i * offset;
            pthread_create(&j_threads[i], NULL, myReadmt, (void*)&jThArgs[i]);
        }
        for (int i = 0; i < NUM_OF_THREADS; i++) {
            pthread_join(j_threads[i], (void**)&results[i]);
            finalResult ^= results[i];
        }
        free(j_threads);
        free(jThArgs);
        free(results);
        close(fd);

        printf("XOR result is %08x\n", finalResult);
    }
}
