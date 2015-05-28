#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>	// for fprintf
#include <string.h>	// for memcpy
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "DV.h"

#define BUFSIZE 2048

using namespace std;

// TODO:
// implement distance vector algorithm
// support more than 2 nodes
// check if neighbors are alive

int main(int argc, char **argv)
{
    // check for errors

    if (argc < 3)
    {
        perror("Not enough arguments.\nUsage: ./my_router <initialization file> <router name>\n");
        return 0;
    }


    // initialize everything

    DV dv(argv[1], argv[2]);
    exit(0);

    // TODO: change to accomodate multiple nodes?


    int myPort = atoi(argv[1]); // my port; first argument
    int remotePort = atoi(argv[2]); // remote port; second argument

    struct sockaddr_in myaddr; // our address
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    myaddr.sin_port = htons(myPort);

    struct sockaddr_in remaddr; // remote address
    memset((char *)&remaddr, 0, sizeof(remaddr));
    remaddr.sin_family = AF_INET;
    remaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    remaddr.sin_port = htons(remotePort);

    socklen_t addrlen = sizeof(remaddr); // length of addresses

    char rcvbuf[BUFSIZE]; // receive buffer
    memset((char *)rcvbuf, 0, sizeof(rcvbuf));

    char sendbuf[BUFSIZE]; // send buffer
    memset((char *)sendbuf, 0, sizeof(sendbuf));
    strcpy(sendbuf, argv[1]); // initialize send buffer
    
    // create a UDP socket
    
    int socketfd; // our socket
    if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create socket\n");
        return 0;
    }
    
    // bind the socket to localhost and myPort

    if (bind(socketfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        perror("bind failed");
        return 0;
    }
    
    // distance vector routing

    int pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        return 0;
    }
    else if (pid == 0) // parent
    {
        for (;;)
        {
            // TODO: send to each neighbor
            sendto(socketfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&remaddr, addrlen);
            sleep(3);
        }
    }
    else // child
    {
        for (;;)
        {
            printf("waiting on port %d\n", myPort);
            int recvlen = recvfrom(socketfd, rcvbuf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
            printf("received %d bytes\n", recvlen);
            if (recvlen > 0)
            {
                rcvbuf[recvlen] = 0;
                printf("received message: \"%s\"\n", rcvbuf);
            }
        }
    }
}