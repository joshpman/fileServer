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
    listen(listener, 1);
    printf("The socket should be passive now: %d\n", listener);
    length = sizeof(s2);
    conn = accept(listener, (struct sockaddr*) &s2, (socklen_t*) &length);
    printf("Accepted a client, Talking through fd %d\n", conn);
    printsin(&s2, "RSTREAM:, accepted connection from");
    while(read(conn, &ch,1)==1) putchar(ch);
    putchar('\n'); 
}
