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
#include <unistd.h>

#define PORT 1153
#define BUFSIZE 2048

int
main(int argc, char **argv)
{
    // check for errors
    if (argc < 3) {
        perror("not enough arguments");
        return 0;
    }

    struct sockaddr_in myaddr;      /* our address */
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
    int recvlen;                    /* # bytes received */
    int fd;                         /* our socket */
    char rcvbuf[BUFSIZE];     /* receive buffer */
    char sendbuf[BUFSIZE];     /* send buffer */
    memset((char *)rcvbuf, 0, sizeof(rcvbuf));
    memset((char *)sendbuf, 0, sizeof(sendbuf));
    strcpy(sendbuf, argv[1]);
    
    /* create a UDP socket */
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return 0;
    }
    
    /* bind the socket to any valid IP address and a specific port */
    
    int myPort = atoi(argv[1]);
    int remotePort = atoi(argv[2]);

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    myaddr.sin_port = htons(myPort);

    memset((char *)&remaddr, 0, sizeof(remaddr));
    remaddr.sin_family = AF_INET;
    remaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    remaddr.sin_port = htons(remotePort);

    
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    
    int pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 0;
    }
    else if (pid == 0) { // child
        for (;;) {
            // TODO: loop through each neighbor
            sendto(fd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&remaddr, addrlen);
            sleep(3);
        }
    }
    else { // parent
        for (;;) {
            printf("waiting on port %d\n", myPort);
            recvlen = recvfrom(fd, rcvbuf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
            printf("received %d bytes\n", recvlen);
            if (recvlen > 0) {
                rcvbuf[recvlen] = 0;
                printf("received message: \"%s\"\n", rcvbuf);
            }
        }
    }

    /* now loop, receiving data and printing what we received */
    // for (;;) {
    //     printf("waiting on port %d\n", PORT);
    //     recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    //     printf("received %d bytes\n", recvlen);
    //     if (recvlen > 0) {
    //         buf[recvlen] = 0;
    //         printf("received message: \"%s\"\n", buf);
    //     }
    //     sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen);
    // }
    /* never exits */
}