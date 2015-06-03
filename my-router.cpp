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
	int type;
	char source;
	char dest;
	int length;
};

enum type
{
	TYPE_DATA, TYPE_ADVERTISEMENT, TYPE_WAKEUP, TYPE_RESETLOWER, TYPE_RESETHIGHER
};

void *createPacket(bool advertisement, char source, char dest, int payloadLength, void *payload);
header getHeader(void *packet);
void *getPayload(void *packet, int length);
void multicast(DV &dv, int socketfd);
void selfcast(DV &dv, int socketfd, int type, char source);
void multicastResetLower(DV &dv, int socketfd, char dead);
void multicastResetHigher(DV &dv, int socketfd, char dead);

int main(int argc, char **argv)
{
	// check for errors

	if (argc < 3)
	{
		perror("Not enough arguments.\nUsage: ./my_router <initialization file> <router name>\n");
		return 0;
	}

	DV dv(argv[1], argv[2]);
	dv.printAll();

	vector<node> neighbors = dv.neighbors();

	int myPort = dv.portNoOf(argv[2][0]); // my port

	dv.initMyaddr(myPort);
	sockaddr_in myaddr = dv.myaddr();

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
			// periodically wake up parent process
			selfcast(dv, socketfd, TYPE_WAKEUP, dv.getName());
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
			int recvlen = recvfrom(socketfd, rcvbuf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
			
			header h = getHeader(rcvbuf);
			void *payload = getPayload(rcvbuf, h.length);
			switch(h.type)
			{
				case TYPE_DATA:
					cout << "Received data packet\n" << (char*)payload << endl;
					break;
				case TYPE_ADVERTISEMENT:
					dv_entry entries[NROUTERS];
					memcpy((void*)entries, payload, h.length);
					for (int i = 0; i < neighbors.size(); i++)
					{
						if (neighbors[i].name == h.source)
							dv.startTimer(neighbors[i]);
					}
					if (dv.update(payload, h.source))
					{
						dv.printAll();
						//multicast(dv, socketfd);
					}
					break;
				case TYPE_WAKEUP:
					// perform periodic tasks
					
					for (int i = 0; i < neighbors.size(); i++)
					{
						node curNeighbor = neighbors[i];
						if ((dv.getEntries()[dv.indexOf(curNeighbor.name)].cost != -1) && dv.timerExpired(neighbors[i]))
						{
							selfcast(dv, socketfd, TYPE_RESETLOWER, neighbors[i].name);
							selfcast(dv, socketfd, TYPE_RESETHIGHER, neighbors[i].name);
						}
					}
					multicast(dv, socketfd);
					break;
				case TYPE_RESETLOWER:
					cerr << "resetting LOWER because " << h.source << " died" << endl;
					dv.reset(h.source);
					dv.printAll();
					multicastResetLower(dv, socketfd, h.source);
					break;
				case TYPE_RESETHIGHER:
					cerr << "resetting HIGHER because " << h.source << " died" << endl;
					dv.reset(h.source);
					dv.printAll();
					multicastResetHigher(dv, socketfd, h.source);
					break;
			}
			//sleep(5);
		}
		free(rcvbuf);
	}
}

// create a packet with header and payload
void *createPacket(int type, char source, char dest, int payloadLength, void *payload)
{
	// create empty packet
	void *packet = malloc(sizeof(header)+payloadLength);

	// create header
	header h;
	h.type = type;
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

// multicast advertisement to all neighbors
void multicast(DV &dv, int socketfd)
{
	vector<node> neighbors = dv.neighbors();
	for (int i = 0; i < neighbors.size(); i++)
	{
		void *sendPacket = createPacket(TYPE_ADVERTISEMENT, dv.getName(), neighbors[i].name, dv.getSize(), (void*)dv.getEntries());
		sendto(socketfd, sendPacket, sizeof(header) + dv.getSize(), 0, (struct sockaddr *)&neighbors[i].addr, sizeof(sockaddr_in));
		free(sendPacket);
	}
}

// periodically wake yourself up to multicast advertisement
void selfcast(DV &dv, int socketfd, int type, char source)
{
	void *sendPacket = createPacket(type, source, (char)0, (char)0, (void*)0);
	sockaddr_in destAddr = dv.myaddr();
	sendto(socketfd, sendPacket, sizeof(header), 0, (struct sockaddr *)&destAddr, sizeof(sockaddr_in));
	free(sendPacket);
}

void multicastResetLower(DV &dv, int socketfd, char dead)
{
	vector<node> neighbors = dv.neighbors();
	for (int i = 0; i < neighbors.size(); i++)
	{
		if (neighbors[i].portno < dv.port())
		{
			void *sendPacket = createPacket(TYPE_RESETLOWER, dead, (char)0, (char)0, (void*)0);
			sendto(socketfd, sendPacket, sizeof(header), 0, (struct sockaddr *)&neighbors[i].addr, sizeof(sockaddr_in));
			free(sendPacket);
		}
	}
}

void multicastResetHigher(DV &dv, int socketfd, char dead)
{
	vector<node> neighbors = dv.neighbors();
	for (int i = 0; i < neighbors.size(); i++)
	{
		if (neighbors[i].portno > dv.port())
		{
			void *sendPacket = createPacket(TYPE_RESETHIGHER, dead, (char)0, (char)0, (void*)0);
			sendto(socketfd, sendPacket, sizeof(header), 0, (struct sockaddr *)&neighbors[i].addr, sizeof(sockaddr_in));
			free(sendPacket);
		}
	}
}
