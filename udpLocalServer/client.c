#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/select.h>
int main(){
    struct sockaddr_un dest, mine;
    struct{
        char head;
        unsigned char body[64];
        char tail;
    } msgbuf;
    int socketFD = socket(AF_UNIX, SOCK_DGRAM, 0);
    dest.sun_family=AF_UNIX;
    strcpy(dest.sun_path, "udgram");
    mine.sun_family=AF_UNIX;
    strcpy(mine.sun_path, "mynewaddr");
    bind(socketFD, (struct sockaddr *) &mine, sizeof(mine));
    msgbuf.head= '<';
    msgbuf.tail = '>';
    while(1){
        memset(&msgbuf.body, 0, sizeof(msgbuf.body));
        fd_set readFD;
        FD_ZERO(&readFD);
        FD_SET(1, &readFD);
        struct timeval timeout;
        timeout.tv_sec = 5;
        int selectRet = select(2, &readFD, 0, 0, &timeout);
        if(selectRet==0){
            printf("Timeout expired\n");
            break;
        }
        if(FD_ISSET(1, &readFD)){
            read(1, &msgbuf.body, 64);
            sendto(socketFD, &msgbuf, sizeof(msgbuf), 0, (struct sockaddr *) &dest, sizeof(dest));
        }
    }
}