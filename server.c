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
//Signal handler for SIGUSR1 that closes out listener process
void closeOut(){
    close(listener);
    running = 0;
}
//Go into this function to wait for client to echo back file size
void awaitEcho(int clientFD, int fileSize){
    char buf[64];
    while(1){
        memset(buf, 0, 64);
        read(clientFD, buf, 64);
        printf("client echo'd %s\n", buf);
        int intVal = strtol(buf, 0, 0);
        if(intVal == fileSize){
            break;
        }
    }
}
//Returns digits in numbers-- looks messy but it's faster than using num/10 recursively- limited to single digit values for now but that still allows up to 1 billion bytes
int getDigits(int number){
    if (number < 10) return 1;
    if (number < 100) return 2;
    if (number < 1000) return 3;
    if (number < 10000) return 4;
    if (number < 100000) return 5;
    if (number < 1000000) return 6;
    if (number < 10000000) return 7;
    if (number < 100000000) return 8;
    if (number < 1000000000) return 9;
    return 0;
}
//Gets size of file from client, parses it, and returns the value
int recieveSize(int socketFD){
    char buf[64];
    memset(buf, 0, 64);
    ssize_t bytesIn = read(socketFD, buf, 64);
    int endDigits = strtol(&buf[0], 0, 0);
    char digitCounter[endDigits+1];
    memset(digitCounter, 0, endDigits+1);
    for(int i = 5; i<bytesIn; i++){
        digitCounter[i-5] = buf[i];
    }
    int totalDigits = strtol(digitCounter, 0, 0);
    printf("Total Digits is %d\n", totalDigits);
    return totalDigits; 
}
//Sends length encoding to client, awaits response, then writes out requested file
void writeFile(int socket, int fileFD, int fileSize){
    int digits = getDigits(fileSize);
    //Need space for the digit counter-- 1 digit -- then the word file-- then the actual fileSize + 1 for null terminator
    char filesizeEncoding[strlen("file") + digits + 2];
    sprintf(filesizeEncoding, "%dfile%d", digits, fileSize);
    write(socket, filesizeEncoding, strlen(filesizeEncoding));
    awaitEcho(socket, fileSize);
    char buf[32768];
    while(1){
        memset(buf, 0, 1024);
        ssize_t bytesIn = read(fileFD, buf, 32768);
        if(bytesIn==0){
            close(fileFD);
            write(1, "Finished writing file\n", 23);
            break;
        }else{
            write(socket, buf, bytesIn);
        }
    }
}
//Readings in file from client
void readInFile(int fd, int outputFD){
    char buf[32768];
    int fileSize = recieveSize(fd);
    char echo[getDigits(fileSize)+1];
    sprintf(echo, "%d", fileSize);
    write(fd, echo, strlen(echo));
    while(1){
        fd_set readIn;
        FD_ZERO(&readIn);
        FD_SET(fd, &readIn);
        select(fd+1, &readIn, 0,0,0);
        memset(buf, 0, 32768);
        if(FD_ISSET(fd, &readIn)){
            ssize_t bytesIn = read(fd, buf, 32768);
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
//List files-- Just execs ls ./files in subprocess and pipes the results to client(./files in the 'working directory' since theres no directory navigation)
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
//Prints information about client- Only for testing, does not add functionality
void printsin(s_in, s1, s2)
struct sockaddr_in *s_in; char *s1, *s2;
{
    printf("Program: %s\n%s ", s1, s2);
    printf("(%s, %d)\n", inet_ntoa(s_in->sin_addr) , s_in->sin_port);
}
int main(){
    //Setting up signal handler + running value which signal handler uses to end the listener
    int selectVal;
    fd_set watchFD;
    FD_ZERO(&watchFD);
    struct timeval timeout = {0,0};
    int nfds = 0;
    running = 1;
    signal(SIGUSR1, closeOut);
    int enteredPassword = 0;
    int conn, length;
    //Setting up socket as AF_INET and SOCK_STREAM then binding it to a wildcard IP and Port in network byte ordering
    struct sockaddr_in s1, s2;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    memset(&s1, 0, sizeof(s1));
    s1.sin_family= (short) AF_INET;
    s1.sin_addr.s_addr = htonl(INADDR_ANY);
    s1.sin_port= htons(0);
    length = sizeof(s1);
    bind(listener, (struct sockaddr*) &s1, (socklen_t) length);
    getsockname(listener, (struct sockaddr*) &s1, (socklen_t *) &length);
    //Prints server information
    printf("RSTREAM: assigned port number %d and ip %s\n", ntohs(s1.sin_port), inet_ntoa(s1.sin_addr));
    char buf[256];
    char passwordBuff[strlen(password)+1];
    
    //Opening ./files directory if it doesn't already exist
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
                    nfds = conn + 1;
                    FD_ZERO(&watchFD);
                    FD_SET(conn, &watchFD);

                    timeout.tv_sec = 60;
                    //Password checking, can turn off by changing to while(0)  
                    while(enteredPassword==0){
                        nfds = conn + 1;
                        FD_ZERO(&watchFD);
                        FD_SET(conn, &watchFD);
                        select(nfds, &watchFD, 0, 0, 0);
                        memset(passwordBuff, 0, (strlen(password)+1));
                        if(FD_ISSET(conn, &watchFD)){
                            ssize_t bytesIn = read(conn, passwordBuff, strlen(password));
                            if(bytesIn> 0 && strcmp(passwordBuff, password)==0){
                                enteredPassword = 1;
                                write(conn, "Correct Password!\n", 19);
                                //Clearing out socket so that any password related entries arent caught in command handling
                                memset(passwordBuff, 0,(strlen(password)+1));
                                read(conn, passwordBuff, (strlen(password)+1));
                                break;
                            }else{
                                write(conn, "Incorrect Password!\n", 21);
                            }
                        }
                    }
                    //Checking if client is active
                    selectVal = select(nfds, &watchFD, 0, 0, &timeout);
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
                        //Command checking
                        //Client wrote list- list off files in ./files directory in ls -la format
                        if((strstr(buf, listCmd)!=0)&& bytesIn==5){
                            write(1, "Client wrote list\n", 19);
                            listFiles(conn);
                            write(conn, "\n", 2);
                        //Client wrote get- Downloads requested file to their machine in their working directory                            
                        }else if((strstr(buf, getCmd)!=0)&& bytesIn>4){
                            write(1, "Client wrote get\n", 18);
                            char filename[bytesIn-4 + strlen("./files/") + 1];
                            strcpy(filename, "./files/");
                            strcat(filename, &buf[4]);
                            printf("Filename is %s\n", filename);
                            int outputFD = open(filename, O_RDWR, 0644);
                            int fileSize = lseek(outputFD, 0, SEEK_END);
                            lseek(outputFD, 0, SEEK_SET);
                            if(outputFD<0){
                                write(conn, "File DNE\n", 10);
                            }else{
                                write(conn, "Found\n", 7);
                                sleep(1);
                                writeFile(conn, outputFD, fileSize);
                            }
                        //Client wrote put- Uploads file from their machine to servers ./files directory overriding the file if it already exist
                        }else if((strstr(buf, uploadCmd)!=0)&& bytesIn>4){
                            write(1, "Client wrote put\n", 18);
                            char output[bytesIn-4 + strlen("./files/") + 1];
                            strcpy(output, "./files/");
                            strcat(output, &buf[4]);
                            int outputFD = open(output, O_CREAT | O_RDWR | O_TRUNC, 0644);
                            readInFile(conn, outputFD);
                        //Client wrote q!- Kills listener process and child which ends the TCP Server process entirely
                        }else if((strstr(buf, killParent)!=0)&& bytesIn>2){
                            write(1, "Closing parent process and current socket\n", 43);
                            close(conn);
                            kill(getppid(), SIGUSR1);
                            exit(0);
                        //Else just means it is not a command that is recognized so server writes 'Unknown command'
                        }else{
                            write(conn, "Unknown command\n", 17);
                        }
                    }
                }
                
            }
            //Closing accepted client FD in listener so we can accept more clients-- Accept only has a 1 spot queue so this is needed
            close(conn);
        }
    }
    //Program should get here if a client sends the kill signal and the signal handler ends the main logic loop, at this point just exiting is all thats left to do
    exit(0);
}
