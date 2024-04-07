#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
char listCmd[] = {"list"};
char serverConfirm[] = {"confirmed"};
char killParent[] = {"q!"};
char killSession[] = {"q"};
char getCmd[] = {"get"};
char uploadCmd[] = {"put"};
char found[] = {"Found\n"};
//Gets size of file from server, parses it, and returns the value
int recieveSize(int socketFD){
    char buf[64];
    ssize_t bytesIn = read(socketFD, buf, 64);
    int endDigits = strtol(&buf[0], 0, 0);
    char digitCounter[endDigits+1];
    for(int i = 5; i<bytesIn; i++){
        digitCounter[i-5] = buf[i];
    }
    int totalDigits = strtol(digitCounter, 0, 0);
    return totalDigits; 
}
//Go into this function to wait for server to echo back file size
void awaitEcho(int serverFD, int fileSize){
    char buf[64];
    while(1){
        memset(buf, 0, 64);
        read(serverFD, buf, 64);
        printf("Server echo'd %s\n", buf);
        int intVal = strtol(buf, 0, 0);
        if(intVal == fileSize){
            break;
        }
    }
}
//Returns digits in numbers-- looks messy but it's faster than using num/10 recursively
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
//Readings in file from Server
void readInFile(int socket, char* filename){
    char buf[1024];
    char finder[7];
    int filesize;
    memset(finder, 0, 7);
    read(socket, finder, 7);
    //Server will echo "Found" if the file exists, this just verifies that the file exists
    if(strcmp(finder, found)==0){
        //Getting filesize from server and echoing it back to begin file transfer
        filesize = recieveSize(socket);
        char echo[getDigits(filesize)];
        sprintf(echo, "%d", filesize);
        write(socket, echo, strlen(echo));
        int outputFD = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
        while(1){
            memset(buf, 0, 1024);
            ssize_t bytesIn = read(socket, buf, 1024);
            filesize-=bytesIn;
            write(outputFD, buf, bytesIn);
            if(filesize==0){
                close(outputFD);
                write(1, "Download Complete!\n", 20);
                break;
            }
        }
    }else{
        write(2, "Invalid file name!\n", 20);
    }
}
//Sends file from local machine to server from current directory-- Can also pass in an absolute path
void sendFile(int socket, int fileFD, int fileSize){
    //Get file size, send to parent, then await confirm before sending
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
            break;
        }else{
            write(socket, buf, bytesIn);
        }
    }
}
int main(int argc, char *argv[]){
    //Needs to be format: ./client ip port otherwise the program will seg fault
    if(argc<3){
        write(2, "Not enough arguments\n", 22);
        exit(-1);
    }
    //Setting up socket to connect to server with with IPv4 Address Format and Byte Stream connection
    char *remhost; unsigned short remport;
    int sock;
    struct sockaddr_in remote;
    struct hostent *h;
    remhost = argv[1]; remport = atoi(argv[2]);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset((char*) &remote, 0, sizeof(remote));
    remote.sin_family= (short) AF_INET;
    h = gethostbyname(remhost);
    memcpy((char*) &remote.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    remote.sin_port= htons(remport);
    if((connect(sock, (struct sockaddr*) &remote, sizeof(remote)))==-1){
        write(2, "Socket connection failed\n", 26);
        exit(1);
    }
    write(1, "Socket is now connected\n", 25);
    fflush(stdout);
    fd_set readIn;

    while(1){
        char buf[256];
        memset(buf, 0, 256);
        FD_ZERO(&readIn);
        FD_SET(0, &readIn);
        FD_SET(sock, &readIn);
        int nfds = sock + 1; 
        struct timeval timeout;
        timeout.tv_sec = 60;
        int selectVal = select(nfds, &readIn, 0, 0, &timeout);
        if(selectVal == 0){
            write(2, "Session timeout\n", 17);
            close(sock);
            exit(1);
        }
        if(FD_ISSET(sock, &readIn)){
            //Verifying that server does not feed EOF to client
            ssize_t bytesIn = read(sock, &buf, 256);
            if(bytesIn==0){
                write(2, "Host disconnected, exiting\n",28);
                close(sock);
                exit(1);
            }
            write(1, &buf, bytesIn);
        }
        if(FD_ISSET(0, &readIn)){
            ssize_t bytesIn = read(0, &buf, 256);
            if(bytesIn>1){
                //List Command- Reads off files from servers ./files directory in ls -la format
                if((strstr(buf, listCmd)!=0)&& bytesIn==5){
                    write(1, "Client wrote list\n", 19);
                    write(sock, &buf, bytesIn);
                    memset(buf, 0, 256);
                //Get Command- Downloads specified file from servers ./files directory
                }else if((strstr(buf, getCmd)!=0)&& bytesIn>3){
                    write(1, "Client wrote get\n", 18);
                    buf[bytesIn-1] = '\0';
                    char filename[bytesIn-3];
                    strcpy(filename, &buf[4]);
                    write(sock, &buf, bytesIn);
                    readInFile(sock, filename);
                //Put Command- Uploads specified file to servers ./files directory
                }else if((strstr(buf, uploadCmd)!=0)&& bytesIn>3){
                    write(1, "Client wrote put\n", 18);
                    char filename[bytesIn-3];
                    buf[bytesIn-1] = '\0';
                    strcpy(filename, &buf[4]);
                    int fd = open(filename, O_RDONLY);
                    int fileSize = lseek(fd, 0, SEEK_END);
                    lseek(fd, 0, SEEK_SET);
                    if(fd<0){
                        write(2, "Failed to open fd\n", 19);
                    }else{
                        write(sock, &buf, bytesIn);
                        sleep(1);
                        sendFile(sock, fd, fileSize);
                    }
                //q! command- ends server listening processs
                }else if((strstr(buf, killParent)!=0)&& bytesIn>2){
                    write(1, "Closing TCP Server and this client process\n",44);
                    write(sock, &buf, bytesIn);
                    close(sock);
                    exit(0);
                //q command- ends current client process
                }else if((strstr(buf, killSession)!=0)&& bytesIn>1){
                    write(1, "Closing session\n", 17);
                    write(sock, &buf, bytesIn);
                    close(sock);
                    exit(0);
                //Unknown command on client side just gets echo'd to server
                }else{
                    write(sock, &buf, bytesIn);
                    memset(buf, 0, 256);
                }
                
            }
        }  
    }
    close(sock);
    exit(0);
}