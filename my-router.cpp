#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>	// for fprintf
#include <string.h>	// for memcpy
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <time.h>

#include "DV.h"

#define BUFSIZE 2048

using namespace std;

struct header
{
	int type;
	char source;
	char dest;
	int length;
};

enum type
{
	TYPE_DATA, TYPE_ADVERTISEMENT, TYPE_WAKEUP, TYPE_RESET
};

void *createPacket(int type, char source, char dest, int payloadLength, void *payload);
header getHeader(void *packet);
void *getPayload(void *packet, int length);
void multicast(DV &dv, int socketfd);
void selfcast(DV &dv, int socketfd, int type, char source = 0, char dest = 0, int payloadLength = 0, void *payload = 0);

int main(int argc, char **argv)
{
	// check for errors
	if (argc < 3)
	{
		perror("Not enough arguments.\nUsage: ./my_router <initialization file> <router name>\n");
		return 0;
	}

	DV dv(argv[1], argv[2]);

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

	// send a data packet to router A
	if (dv.getName() == 'H')
	{
		
		char data[100];
		memset(data, 0, 100);
		cin.getline(data, 100);
		for (int i = 0; i < neighbors.size(); i++)
		{
			if (neighbors[i].name == 'A')
			{
				void *dataPacket = createPacket(TYPE_DATA, dv.getName(), 'D', strlen(data), (void*)data);
				sendto(socketfd, dataPacket, sizeof(header) + dv.getSize(), 0, (struct sockaddr *)&neighbors[i].addr, sizeof(sockaddr_in));

				// print info
				header h = getHeader(dataPacket);
				cout << "Sent data packet" << endl;
				cout << "Type: data" << endl;
				cout << "Source: " << h.source << endl;
				cout << "Destination: " << h.dest << endl;
				cout << "Length of packet: " << sizeof(header) + h.length << endl;
				cout << "Length of payload: " << h.length << endl;
				cout << "Payload: " << data << endl;

				free(dataPacket);
			}
		}
		exit(0);
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
			selfcast(dv, socketfd, TYPE_WAKEUP);
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
					cout << "Received data packet" << endl;
					time_t rawtime;
					time(&rawtime);
					cout << "Timestamp: " << ctime(&rawtime);
					cout << "ID of source node: " << h.source << endl;
					cout << "ID of destination node: " << h.dest << endl;
					cout << "UDP port in which the packet arrived: " << myPort << endl;
					if (h.dest != dv.getName()) // only forward if this router is not the destination
					{
						if (dv.routeTo(h.dest).nexthopPort() == -1)
						{
							cout << "Error: packet could not be forwarded" << endl;
						}
						else
						{
							cout << "UDP port along which the packet was forwarded: " << dv.routeTo(h.dest).nexthopPort() << endl;
							cout << "ID of node that packet was forwarded to: " << dv.routeTo(h.dest).nexthopName() << endl;
							void *forwardPacket = createPacket(TYPE_DATA, h.source, h.dest, h.length, (void*)payload);
							for (int i = 0; i < neighbors.size(); i++)
							{
								if (neighbors[i].name == dv.routeTo(h.dest).nexthopName())
									sendto(socketfd, forwardPacket, sizeof(header) + dv.getSize(), 0, (struct sockaddr *)&neighbors[i].addr, sizeof(sockaddr_in));
							}
							free(forwardPacket);
						}
						cout << endl;
					}
					else
					{
						char data[100];
						memset(data, 0, 100);
						memcpy((void*)data, payload, h.length);
						cout << "Data payload: " << data << endl << endl;
					}
					break;
				case TYPE_ADVERTISEMENT:
					dv_entry entries[NROUTERS];
					memcpy((void*)entries, payload, h.length);
					for (int i = 0; i < neighbors.size(); i++)
					{
						if (neighbors[i].name == h.source)
							dv.startTimer(neighbors[i]);
					}
					dv.update(payload, h.source);
					break;
				case TYPE_WAKEUP: // perform periodic tasks
					for (int i = 0; i < neighbors.size(); i++)
					{
						node curNeighbor = neighbors[i];
						if ((dv.getEntries()[dv.indexOf(curNeighbor.name)].cost() != -1) && dv.timerExpired(neighbors[i]))
						{
							selfcast(dv, socketfd, TYPE_RESET, dv.getName(), neighbors[i].name, dv.getSize() / sizeof(dv_entry) - 2);
						}
					}
					multicast(dv, socketfd);
					break;
				case TYPE_RESET:
					int hopcount = (int)h.length - 1;
					dv.reset(h.dest);
					if (hopcount > 0)
					{
						void *forwardPacket = createPacket(TYPE_RESET, dv.getName(), h.dest, hopcount, (void*)0);
						for (int i = 0; i < neighbors.size(); i++)
						{
							if (neighbors[i].name != h.source)
								sendto(socketfd, forwardPacket, sizeof(header), 0, (struct sockaddr *)&neighbors[i].addr, sizeof(sockaddr_in));
						}
					}
					break;
			}
		}
		free(rcvbuf);
	}
}

// create a packet with header and payload
void *createPacket(int type, char source, char dest, int payloadLength, void *payload)
{
	int allocatedPayloadLength = payloadLength;
	if ((type != TYPE_DATA) && (type != TYPE_ADVERTISEMENT))
		allocatedPayloadLength = 0;

	// create empty packet
	void *packet = malloc(sizeof(header)+allocatedPayloadLength);

	// create header
	header h;
	h.type = type;
	h.source = source;
	h.dest = dest;
	h.length = payloadLength;

	// fill in packet
	memcpy(packet, (void*)&h, sizeof(header));
	memcpy((void*)((char*)packet+sizeof(header)), payload, allocatedPayloadLength);

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
void selfcast(DV &dv, int socketfd, int type, char source, char dest, int payloadLength, void *payload)
{
	void *sendPacket = createPacket(type, source, dest, payloadLength, payload);
	sockaddr_in destAddr = dv.myaddr();
	sendto(socketfd, sendPacket, sizeof(header), 0, (struct sockaddr *)&destAddr, sizeof(sockaddr_in));
	free(sendPacket);
}

