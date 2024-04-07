#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <stdlib.h>
char *listArgs[] = {"ls", "-la", "./files", NULL};
int main(int argc, char *argv[]){
    int outputPipe[2];
    pipe(outputPipe);
    int preserveStdout = dup(1);
    printf("Strlen for filename is %ld\n", strlen(argv[1]));
    char *listArgs[] = {"wc", "-c", argv[1], NULL};
    if(fork()==0){
        close(outputPipe[0]);
        dup2(outputPipe[1], 1);
        execvp("wc", listArgs);
    }
    close(outputPipe[1]);
    dup2(preserveStdout, 1);
    wait(NULL);
    char buf[64];
    memset(buf, 0, 64);
    ssize_t bytesIn = read(outputPipe[0], buf, 64);
    //Null terminator the buffer before wc states the filename so we can just read the char count
    buf[bytesIn-strlen(argv[1]) -1] = '\0';
    int file=strtol(buf, 0, 10);
    printf("filesize is %d\n", file);
    int fd = open(argv[1], O_RDONLY);
    int size = lseek(fd, 0,SEEK_END);
    printf("Lseek size is %d\n", size);
    close(outputPipe[0]);

}

/*
int outputFD[2];
    pipe(outputFD);
    int preserveStdout = dup(1);
    close(1);
    if(fork()==0){
        close(outputFD[0]);
        dup2(outputFD[1], 1);
        execvp("ls", listArgs);
    }
    close(outputFD[1]);
    dup2(preserveStdout, 1);
    int nfds = outputFD[0] + 1;
    char buf[256];
    while(1){
        memset(buf, 0, 256);
        fd_set watch;
        FD_ZERO(&watch);
        FD_SET(outputFD[0], &watch);
        select(nfds, &watch, 0,0,0);
        if(FD_ISSET(outputFD[0], &watch)){
            ssize_t bytesIn = read(outputFD[0], buf, 256);
            if(bytesIn==0){
                close(outputFD[0]);
                break;
            }
            write(writeFD, buf, bytesIn);
        }
    }
*/