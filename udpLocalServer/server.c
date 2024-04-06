#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/types.h>
int main(){
    int cc;
    socklen_t fsize;
    struct sockaddr_un s_un, from;
    size_t addrlength;
    struct{
        char head;
        unsigned char body[64];
        char tail;
    } msg;
    int socketFD = socket(AF_UNIX, SOCK_DGRAM, 0);
    s_un.sun_family=AF_UNIX;
    strcpy(s_un.sun_path, "udgram");
    addrlength = sizeof(s_un.sun_family) + sizeof(s_un.sun_path);
    if(bind(socketFD, (struct sockaddr *) &s_un, addrlength)<0){
        perror("bind");
        return -1;
    }
    while(1){
        fsize=sizeof(from);
        cc= recvfrom(socketFD, &msg, sizeof(msg), 0, (struct sockaddr *) &from, &fsize);
        for(int i = 0; i<sizeof(msg.body); i++){
            printf("%c", msg.body[i]);
        }
        fflush(stdout);
    }
}