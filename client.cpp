#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>	/* for fprintf */
#include <string.h>	/* for memcpy */
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>

#define PORT 1153
#define BUFSIZE 2048

//unordered_map<tuple<char, char>, int> addresses; // TODO: add costs
// unordered_map<char, int>

int main(int argc, char **argv) {
	unsigned char buf[BUFSIZE];     /* receive buffer */
    memset((char *)buf, 0, sizeof(buf));
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen = sizeof(remaddr);            /* length of addresses */

	// create the socket
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

	// set up the sockaddr_in struct
	struct sockaddr_in myaddr;
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(10001);

	// bind the socket
	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	struct hostent *hp;
	struct sockaddr_in servaddr;
	char *my_message = "this is a test message";
	char *host = "127.0.0.1";

	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1153);

	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "could not obtain address of %s\n", host);
		return 0;
	}
	memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
	
	if (sendto(fd, my_message, strlen(my_message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("sendto failed");
		return 0;
	}
	int recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
	printf("received message: \"%s\"\n", buf);
}
