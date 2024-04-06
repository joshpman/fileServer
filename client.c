#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>
int main(int argc, char *argv[]){
    if(argc<3){
        write(2, "Not enough arguments\n", 22);
        exit(-1);
    }
    char *remhost; unsigned short remport;
    int sock, left, num, put;
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
        printf("Socket connection failed\n");
        exit(-1);
    }
    printf("Socket is now connected\n");
    fd_set readIn;
    while(1){
        char buf[256];
        memset(buf, 0, 256);
        FD_ZERO(&readIn);
        FD_SET(0, &readIn);
        FD_SET(sock, &readIn);
        int nfds = sock + 1; 
        struct timeval timeout;
        timeout.tv_sec = 25;
        int selectVal = select(nfds, &readIn, 0, 0, &timeout);
        if(selectVal == 0){
            write(2, "Session timeout\n", 17);
            close(sock);
            exit(1);
        }
        if(FD_ISSET(0, &readIn)){
            ssize_t bytesIn = read(0, &buf, 256);
            write(sock, &buf, bytesIn);
            memset(buf, 0, 256);
        }
        if(FD_ISSET(sock, &readIn)){
            ssize_t bytesIn = read(sock, &buf, 256);
            if(bytesIn==0){
                write(2, "Host disconnected, exiting\n",28);
                close(sock);
                exit(1);
            }
            write(1, &buf, bytesIn);
        }
    }
    close(sock);
    exit(0);
}