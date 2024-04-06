#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
void printsin(s_in, s1, s2)
struct sockaddr_in *s_in; char *s1, *s2;
{
    printf("Program: %s\n%s ", s1, s2);
    printf("(%s, %d)\n", inet_ntoa(s_in->sin_addr) , s_in->sin_port);
}
int main(){
    int cc;
    socklen_t fsize;
    struct sockaddr_in s_in, from;
    struct{
        uint16_t head;
        unsigned char body[64];
    } msg;
    int socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    memset((char*) &s_in, 0, sizeof(s_in));
    s_in.sin_family=AF_INET;
    s_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_in.sin_port=htons((unsigned short) 0x3333);
    printsin(&s_in, "RECV_UDP", "Local socket is:");
    if(bind(socketFD, (struct sockaddr *) &s_in, sizeof(s_in))<0){
        perror("bind");
        return -1;
    }
    unsigned char quit = 'q';
    while(1){
        memset(&msg.body, 0, sizeof(msg.body));
        msg.head = 0;
        fsize=sizeof(from);
        cc= recvfrom(socketFD, &msg, sizeof(msg), 0, (struct sockaddr *) &from, &fsize);
        if((msg.body[0] == quit) && ntohs(msg.head)==1){
                write(1, "Recieved exit code\n", 20);
                close(socketFD);
                exit(0);
        }
        write(1, msg.body, ntohs(msg.head));
        write(1, "\n", 2);
        fflush(stdout);
    }
}