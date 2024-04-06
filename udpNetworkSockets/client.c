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
    struct sockaddr_in dest, mine;
    struct{
        uint16_t head;
        unsigned char body[64];
    } msgbuf;
    int socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    memset((char*) &dest,0, sizeof(dest));
    struct hostent *hostptr;
    hostptr = gethostbyname(argv[1]);
    dest.sin_family = (short) AF_INET;
    memcpy(hostptr->h_addr_list[0], (char *) &dest.sin_addr , hostptr->h_length);
    dest.sin_port = htons((short) 0x3333);
    bind(socketFD, (struct sockaddr *) &mine, sizeof(mine));
    unsigned char quit = 'q';
    while(1){
         memset(&msgbuf.body, 0, sizeof(msgbuf.body));
        msgbuf.head = 0;
        fd_set readFD;
        FD_ZERO(&readFD);
        FD_SET(0, &readFD);
        struct timeval timeout;
        timeout.tv_sec = 5;
        int selectRet = select(1, &readFD, 0, 0, &timeout);
        if(selectRet==0){
            write(2, "Timeout expired\n", 17);
            msgbuf.body[0] = 'q';
            msgbuf.head = htons(1);
            sendto(socketFD, &msgbuf, sizeof(msgbuf), 0, (struct sockaddr *) &dest, sizeof(dest));
            close(socketFD);
            break;
        }
        if(FD_ISSET(0, &readFD)){
            int size = read(1, &msgbuf.body, 64);
            if((msgbuf.body[0] == quit) && size==2){
                msgbuf.head = htons(1);
                msgbuf.body[0] = 'q';
                sendto(socketFD, &msgbuf, sizeof(msgbuf), 0, (struct sockaddr *) &dest, sizeof(dest));
                close(socketFD);
                write(1, "Exiting\n",9);
                exit(0);
            }
            msgbuf.head=htons((uint16_t) size-1);
            sendto(socketFD, &msgbuf, sizeof(msgbuf), 0, (struct sockaddr *) &dest, sizeof(dest));
        }
    }
}