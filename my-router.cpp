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
// check if neighbors are alive
// timestamp

struct header
{
	bool advertisement;
	char source;
	char dest;
	int length;
};

void *createPacket(bool advertisement, char source, char dest, int payloadLength, void *payload);
header getHeader(void *packet);
void *getPayload(void *packet, int length);

int main(int argc, char **argv)
{
	// check for errors

	if (argc < 3)
	{
		perror("Not enough arguments.\nUsage: ./my_router <initialization file> <router name>\n");
		return 0;
	}

	DV *dv = new DV(argv[1], argv[2]);

	int myPort = dv->portNoOf(argv[2][0]); // my port

	sockaddr_in myaddr; // our address
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	myaddr.sin_port = htons(myPort);

	socklen_t addrlen = sizeof(sockaddr_in); // length of addresses

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
	else if (pid == 0) // send to each neighbor periodically
	{
		for (;;)
		{
			vector<node> neighbors = dv->neighbors();
			for (int i = 0; i < neighbors.size(); i++)
			{
				cerr << "sent packet to: " << neighbors[i].name << endl;
				dv->printAll();
				void *sendPacket = createPacket(true, dv->getName(), neighbors[i].name, dv->getSize(), (void*)dv->getEntries());
				sendto(socketfd, sendPacket, sizeof(header) + dv->getSize(), 0, (struct sockaddr *)&neighbors[i].addr, addrlen);
				free(sendPacket);
			}
			cerr << endl;
			sleep(1);
		}
	}
	else // listen for advertisements
	{
		void *rcvbuf = malloc(BUFSIZE);
		sockaddr_in remaddr;
		for (;;)
		{
			memset(rcvbuf, 0, BUFSIZE);
			printf("waiting on port %d\n", myPort);
			int recvlen = recvfrom(socketfd, rcvbuf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
			
			header h = getHeader(rcvbuf);
			void *payload = getPayload(rcvbuf, h.length);
			if (h.advertisement)
			{
				cerr << "received advertisement packet!" << endl;
				cerr << "source: " << h.source << endl;
				cerr << "dest: " << h.dest << endl;
				cerr << "advertisement flag: " << h.advertisement << endl;
				cerr << "length: " << h.length << endl;
				dv_entry entries[NROUTERS];
				memcpy((void*)entries, payload, h.length);
				cerr << "dst nexthop cost" << endl;
				for (int dest = 0; dest < NROUTERS; dest++)
				{
					cerr << dv->nameOf(dest) << "   ";
					if (entries[dest].nexthop == -1)
						cerr << "   ";
					cerr << entries[dest].nexthop << "   ";
					if (entries[dest].cost != -1)
						cerr << " ";
					cerr << entries[dest].cost;
					cerr << endl;
				}
				cerr << endl;
				dv->update(payload, h.source);
			}
			else
			{
				cout << "Received data packet\n" << (char*)payload << endl;
			}
		}
		free(rcvbuf);
	}
}

// create a packet with header and payload
// advertisement flag is true if dv packet, false if data packet
void *createPacket(bool advertisement, char source, char dest, int payloadLength, void *payload)
{
	// create empty packet
	void *packet = malloc(sizeof(header)+payloadLength);

	// create header
	header h;
	h.advertisement = advertisement;
	h.source = source;
	h.dest = dest;
	h.length = payloadLength;

	// fill in packet
	memcpy(packet, (void*)&h, sizeof(header));
	memcpy((void*)((char*)packet+sizeof(header)), payload, payloadLength);

	return packet;
}

// extract the header from the packet
header getHeader(void *packet)
{
	header h;
	memcpy((void*)&h, packet, sizeof(header));
	return h;
}

// extract the payload from the packet
void *getPayload(void *packet, int length)
{
	void *payload = malloc(length);
	memcpy(payload, (void*)((char*)packet+sizeof(header)), length);
	return payload;
}

