#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
char password[] = {"password"};
char listCmd[] = {"list"};
char *listArgs[] = {"ls", "-la", "./files", NULL};
char getCmd[] = {"get"};
char uploadCmd[] = {"put"};
void readInFile(int fd, int outputFD){
    char buf[1024];
    while(1){
        fd_set readIn;
        FD_ZERO(&readIn);
        FD_SET(fd, &readIn);
        select(fd+1, &readIn, 0,0,0);
        memset(buf, 0, 1024);
        if(FD_ISSET(fd, &readIn)){
            ssize_t bytesIn = read(fd, buf, 1024);
            if(bytesIn==0){
                close(outputFD);
                close(fd);
                exit(0);
            }
            write(outputFD, buf, bytesIn);
        }
    }
}
void listFiles(int writeFD){
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
}
void printsin(s_in, s1, s2)
struct sockaddr_in *s_in; char *s1, *s2;
{
    printf("Program: %s\n%s ", s1, s2);
    printf("(%s, %d)\n", inet_ntoa(s_in->sin_addr) , s_in->sin_port);
}
int main(){
    int enteredPassword = 0;
    int listener, conn, length;
    struct sockaddr_in s1, s2;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    memset(&s1, 0, sizeof(s1));
    s1.sin_family= (short) AF_INET;
    s1.sin_addr.s_addr = htonl(INADDR_ANY);
    s1.sin_port= htons(0);
    length = sizeof(s1);
    bind(listener, (struct sockaddr*) &s1, (socklen_t) length);
    getsockname(listener, (struct sockaddr*) &s1, (socklen_t *) &length);
    printf("RSTREAM: assigned port number %d and ip %s\n", ntohs(s1.sin_port), inet_ntoa(s1.sin_addr));
    char buf[256];
    char passwordBuff[9];
    struct stat st ={0};
    if(stat("./files", &st)==-1){
        mkdir("./files", 0700);
    }
    while(1){
        memset(buf, 0, 256);
        listen(listener, 1);
        length = sizeof(s2);
        conn = accept(listener, (struct sockaddr*) &s2, (socklen_t*) &length);
        if(conn>0){
            printf("Accepted a client, Talking through fd %d\n", conn);
            printsin(&s2, "RSTREAM:, accepted connection from");
            if(fork()==0){
                close(listener);
                write(conn, "Please enter password:\n", 24);
                while(1){
                    memset(buf, 0, 256);
                    int nfds = conn + 1;
                    fd_set watchFD;
                    FD_ZERO(&watchFD);
                    FD_SET(conn, &watchFD);
                    struct timeval timeout;
                    timeout.tv_sec = 60;  
                    while(enteredPassword==0){
                        int nfds = conn + 1;
                        fd_set watchFD;
                        FD_ZERO(&watchFD);
                        FD_SET(conn, &watchFD);
                        select(nfds, &watchFD, 0, 0, 0);
                        memset(passwordBuff, 0, 9);
                        if(FD_ISSET(conn, &watchFD)){
                            ssize_t bytesIn = read(conn, passwordBuff, 8);
                            if(bytesIn> 0 && strcmp(passwordBuff, password)==0){
                                enteredPassword = 1;
                                write(conn, "Correct Password!\n", 19);
                            }else{
                                write(conn, "Incorrect Password!\n", 21);
                            }
                        }
                    }
                    int selectVal = select(nfds, &watchFD, 0, 0, &timeout);
                    if(selectVal==0){
                        write(conn, "Inactive client, timing out\n",29);
                        close(conn);
                        exit(1);
                    }
                    if(FD_ISSET(conn, &watchFD)){
                        ssize_t bytesIn = read(conn, buf, 256);
                        //EOF Detection
                        if(bytesIn==0){
                            write(2, "Peer disconnected, closing socket\n",35);
                            close(conn);
                            exit(0);
                        }
                        //Command check
                        if((strstr(buf, listCmd)!=0)&& bytesIn==5){
                            write(1, "Client wrote list\n", 19);
                            listFiles(conn);
                            write(conn, "\n", 2);
                        }
                         if((strstr(buf, getCmd)!=0)&& bytesIn>4){
                            write(1, "Client wrote get\n", 18);
                        }
                         if((strstr(buf, uploadCmd)!=0)&& bytesIn>4){
                            write(1, "Client wrote put\n", 18);
                            printf("Inputted %ld bytes\n", bytesIn);
                            char output[bytesIn-4 + strlen("./files") + 1];
                            strcpy(output, "./files");
                            strcpy(output, &buf[5]);
                            int outputFD = open(output, O_CREAT | O_RDWR | O_TRUNC, 0644);
                            readInFile(conn, outputFD);
                        }
                    }
                }
                
            }
            close(conn);
        }
    }
}
