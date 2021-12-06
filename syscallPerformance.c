#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "readWrite.h"

#define NUM_SYSCALL 10000000

int main() {
    char* filename = "tempFile";
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
    myWrite(fd, NUM_SYSCALL, 1);

    struct timeval start;
    gettimeofday(&start, NULL);
    double startTime = (double) start.tv_sec + (double) start.tv_usec * 0.000001;

    for(int i=0; i<NUM_SYSCALL; i++){
        lseek(fd, 0, SEEK_CUR);
    }
    struct timeval end;
    gettimeofday(&end, NULL);
    double endTime = (double) end.tv_sec + (double) end.tv_usec * 0.000001;
    
    double timeNeeded = endTime - startTime;
    double sysCallPerSec = NUM_SYSCALL/timeNeeded;

    printf("Time Used to Run %d lseek(): %.2f sec\n", NUM_SYSCALL, timeNeeded);
    printf("Estimated system calls (lseek) per second: %.0f\n", sysCallPerSec);
    close(fd);

    fd = open(filename, O_RDONLY);
    gettimeofday(&start, NULL);
    startTime = (double) start.tv_sec + (double) start.tv_usec * 0.000001;
    myRead(fd, 1, NUM_SYSCALL);
    gettimeofday(&end, NULL);
    endTime = (double) end.tv_sec + (double) end.tv_usec * 0.000001;
    timeNeeded = endTime - startTime;
    double readPerSec = NUM_SYSCALL/timeNeeded;

    printf("Time Used to Run %d read() of 1 byte: %.2f sec\n", NUM_SYSCALL, timeNeeded);
    printf("Estimated system calls (read) per second: %.0f\n", readPerSec);
    
    close(fd);
    remove(filename);
    exit(0);
}

