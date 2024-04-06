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
char password[] = {"password"};
void printsin(s_in, s1, s2)
struct sockaddr_in *s_in; char *s1, *s2;
{
    printf("Program: %s\n%s ", s1, s2);
    printf("(%s, %d)\n", inet_ntoa(s_in->sin_addr) , s_in->sin_port);
}
int main(){
    int enteredPassword = 0;
    int listener, conn, length; char ch;
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
                        int selectVal = select(nfds, &watchFD, 0, 0, 0);
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
                }
                
            }
            close(conn);
        }
    }
    
    // write(conn, "Please enter the password:\n", 28);
    // char buf[256];
    // while(1){

    // }
    
}
