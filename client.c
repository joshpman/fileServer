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
char msg[] = {"the quick brown fox jumps over the lazy dog"};
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
    left = sizeof(msg); put = 0;
    while(left>0){
        if((num-=write(sock, msg + put, left))<0){
            perror("inet_wstream: write");
            exit(1);
        }else{
            left -= num;
            put += num;
        }
    }
}