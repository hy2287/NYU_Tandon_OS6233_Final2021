#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int fd;
    fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);

    srand((unsigned int) time(NULL));
    int* buffer;
    buffer = (int*) malloc(16);
    for (int i = 0; i < 16 / 4; i++) {
        buffer[i] = rand() % 100 + 1;
    }
    write(fd, buffer, 16);
    close(fd);

    exit(0);
}