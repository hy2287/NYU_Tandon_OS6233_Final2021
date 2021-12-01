#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

void myWrite(int fd, int blockSize, int blockCount) {
    char* buffer;
    buffer = (char*) malloc(blockSize);
    for (int i = 0; i < blockSize; i++) {
        buffer[i] = (rand() % 26) + 'a';
    }
    for (int i = 0; i < blockCount; i++) {
      write(fd, buffer, blockSize);
    }
    free(buffer);
    return;
}

unsigned int myRead(int fd, int blockSize) {
    int* buffer;
    unsigned int result = 0;
    buffer = (int*) malloc(blockSize);
    while (read(fd, buffer, blockSize) > 0) {
        for (int i = 0; i < blockSize / 4; i++) {
            //printf("buffer[i] is %d\n", buffer[i]);
            result ^= buffer[i];
        }
    }
    free(buffer);
    return result;
}

int main(int argc, char *argv[]) {
    int fd, blockSize, blockCount;
    char mode;
    unsigned int xorAnswer = 0;

    sscanf (argv[2],"-%c", &mode);
    sscanf (argv[3],"%d", &blockSize);
    sscanf (argv[4],"%d", &blockCount);

    srand(time(0));

    if (mode == 'w' || mode == 'W') {
        fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        myWrite(fd, blockSize, blockCount);
    }
    else {
        fd = open(argv[1], O_RDONLY);
        xorAnswer = myRead(fd, blockSize);
    }

    close(fd);
    if (mode == 'r' || mode == 'R') {
      printf("XOR Answer is %d", xorAnswer);
    }

    exit(0);
}