#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/types.h>
char password[] = {"password"};
char listCmd[] = {"list"};
char *listArgs[] = {"ls", "-la", "./files", NULL};
char getCmd[] = {"get"};
char uploadCmd[] = {"put"};
char killParent[] = {"q!"};
volatile sig_atomic_t running;
static int listener;
int getFileSize(char *filename){
    int outputPipe[2];
    pipe(outputPipe);
    int preserveStdout = dup(1);
    printf("Strlen for filename is %ld\n", strlen(filename));
    char *listArgs[] = {"wc", "-c", filename, NULL};
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
    buf[bytesIn-strlen(filename) -1] = '\0';
    int file=strtol(buf, 0, 10);
    printf("filesize is %d\n", file);
    close(outputPipe[0]);
    return file;
}
void awaitEcho(int clientFD, int fileSize){
    char buf[64];
    while(1){
        memset(buf, 0, 64);
        ssize_t bytesIn = read(clientFD, buf, 64);
        printf("client echo'd %s\n", buf);
        int intVal = strtol(buf, 0, 0);
        if(intVal == fileSize){
            break;
        }
    }
}
int getDigits(int number){
    if (number < 10) return 1;
    if (number < 100) return 2;
    if (number < 1000) return 3;
    if (number < 10000) return 4;
    if (number < 100000) return 5;
    if (number < 1000000) return 6;
    if (number < 10000000) return 7;
    return 0;
}
int recieveSize(int socketFD){
    char buf[64];
    ssize_t bytesIn = read(socketFD, buf, 64);
    int endDigits = strtol(&buf[0], 0, 0);
    char digitCounter[endDigits+1];
    for(int i = 5; i<bytesIn; i++){
        digitCounter[i-5] = buf[i];
    }
    int totalDigits = strtol(digitCounter, 0, 0);
    printf("Total Digits is %d\n", totalDigits);
    return totalDigits; 
}
void closeOut(){
    close(listener);
    exit(0);
}
void writeFile(int socket, int fileFD, int fileSize){
    int digits = getDigits(fileSize);
    //Need space for the digit counter-- 1 digit -- then the word file-- then the actual fileSize + 1 for null terminator
    char filesizeEncoding[strlen("file") + digits + 2];
    sprintf(filesizeEncoding, "%dfile%d", digits, fileSize);
    write(socket, filesizeEncoding, strlen(filesizeEncoding));
    awaitEcho(socket, fileSize);
    char buf[1024];
    while(1){
        memset(buf, 0, 1024);
        ssize_t bytesIn = read(fileFD, buf, 1024);
        if(bytesIn==0){
            close(fileFD);
            write(1, "Finished writing file\n", 23);
            break;
        }else{
            write(socket, buf, bytesIn);
        }
    }
}
void readInFile(int fd, int outputFD){
    char buf[1024];
    int fileSize = recieveSize(fd);
    char echo[getDigits(fileSize)+1];
    sprintf(echo, "%d", fileSize);
    write(fd, echo, strlen(echo));
    while(1){
        fd_set readIn;
        FD_ZERO(&readIn);
        FD_SET(fd, &readIn);
        select(fd+1, &readIn, 0,0,0);
        memset(buf, 0, 1024);
        if(FD_ISSET(fd, &readIn)){
            ssize_t bytesIn = read(fd, buf, 1024);
            fileSize-=bytesIn;
            write(outputFD, buf, bytesIn);
            if(fileSize==0){
                write(1, "Finished reading in file\n", 26);
                close(outputFD);
                break;
            }
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
    running = 1;
    signal(SIGUSR1, closeOut);
    int enteredPassword = 0;
    int conn, length;
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
    while(running==1){
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
                                //Clearing out socket so that any password related entries arent caught in command handling
                                memset(passwordBuff, 0, 9);
                                read(conn, passwordBuff, 9);
                                break;
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
                        }else if((strstr(buf, getCmd)!=0)&& bytesIn>4){
                            write(1, "Client wrote get\n", 18);
                            char filename[bytesIn-4 + strlen("./files/") + 1];
                            strcpy(filename, "./files/");
                            strcat(filename, &buf[4]);
                            printf("Filename is %s\n", filename);
                            int outputFD = open(filename, O_RDWR, 0644);
                            if(outputFD<0){
                                write(conn, "File DNE\n", 10);
                            }else{
                                write(conn, "Found\n", 7);
                                sleep(1);
                                writeFile(conn, outputFD, getFileSize(filename));
                            }
                        }else if((strstr(buf, uploadCmd)!=0)&& bytesIn>4){
                            write(1, "Client wrote put\n", 18);
                            char output[bytesIn-4 + strlen("./files/") + 1];
                            strcpy(output, "./files/");
                            strcat(output, &buf[4]);
                            int outputFD = open(output, O_CREAT | O_RDWR | O_TRUNC, 0644);
                            readInFile(conn, outputFD);
                        }else if((strstr(buf, killParent)!=0)&& bytesIn>2){
                            write(1, "Closing parent process and current socket\n", 43);
                            close(conn);
                            kill(getppid(), SIGUSR1);
                            exit(0);
                        }else{
                            write(conn, "Unknown command\n", 17);
                        }
                    }
                }
                
            }
            close(conn);
        }
    }
    close(listener);
    exit(0);
}
