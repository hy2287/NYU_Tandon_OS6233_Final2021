#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BLOCKSIZE 1024

char buf[BLOCKSIZE];

void copy(int fdR, int fdW) {
    int n;

    while ((n = read(fdR, buf, BLOCKSIZE)) > 0)
        write(fdW, buf, n);
    if (n < 0) {
        printf("copy: read error\n");
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    int fdR, fdW, i;

    fdR = open(argv[1], 0);
    fdW = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);

    copy(fdR, fdW);
    close(fdR);
    close(fdW);

  /*if (argc <= 1) {
    copy(0, 1);
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fdR = open(argv[i], 0)) < 0) {
      printf("cat: cannot open %s\n", argv[i]);
      exit(0);
    }
    fdW = open(argv[++i], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
    copy(fdR, fdW);
    close(fdR);
    close(fdW);
  }*/

    exit(0);
}