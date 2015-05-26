#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>	/* for fprintf */
#include <string.h>	/* for memcpy */
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1153
#define BUFSIZE 2048

int
main(int argc, char **argv)
{
    struct sockaddr_in myaddr;      /* our address */
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
    int recvlen;                    /* # bytes received */
    int fd;                         /* our socket */
    char buf[BUFSIZE];     /* receive buffer */
    memset((char *)buf, 0, sizeof(buf));
    
    /* create a UDP socket */
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return 0;
    }
    
    /* bind the socket to any valid IP address and a specific port */
    
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    myaddr.sin_port = htons(PORT);
    
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    
    /* now loop, receiving data and printing what we received */
    for (;;) {
        printf("waiting on port %d\n", PORT);
        recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        printf("received %d bytes\n", recvlen);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf("received message: \"%s\"\n", buf);
        }
        sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen);
    }
    /* never exits */
}