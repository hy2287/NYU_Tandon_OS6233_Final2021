

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>

#define FAST_BLOCK_SIZE 512 //1048576
int num_of_threads = 4;

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
    pthread_exit((void*)XORresult);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {                                        // calls by fast script
        pthread_t* j_threads;
        struct stat fileStat;

        j_threads = (pthread_t *) malloc(sizeof(pthread_t) * num_of_threads);
        struct jThreadArgs* jThArgs = malloc(sizeof(struct jThreadArgs) * num_of_threads);
        if (!j_threads) {
            printf("j_threads malloc failed\n");
            exit(1);
        }
        int fd = open(argv[1], O_RDONLY);
        fstat(fd, &fileStat);
        unsigned int finalResult = 0;
        unsigned int* results = (unsigned int*) malloc(sizeof(unsigned int) * num_of_threads);
        for (int i = 0; i < num_of_threads; i++) {
            jThArgs[i].fd = fd;
            jThArgs[i].blockCount = fileStat.st_size / num_of_threads / FAST_BLOCK_SIZE;
            jThArgs[i].offset = lseek(fd, i * fileStat.st_size / num_of_threads, SEEK_SET);
            pthread_create(&j_threads[i], NULL, myReadmt, (void*)&jThArgs[i]);
            printf("thread %d is created, processing %lu blocks, and offset is %ld\n", i, jThArgs[i].blockCount, jThArgs[i].offset);
        }
        for (int i = 0; i < num_of_threads; i++) {
            pthread_join(j_threads[i], (void**)&results[i]);
            finalResult ^= results[i];
            printf("thread %d joined and thread XOR is %08x\n", i, results[i]);
        }
        free(j_threads);
        free(jThArgs);
        free(results);
        close(fd);

        printf("final XOR is %08x\n", finalResult);
    }
}
