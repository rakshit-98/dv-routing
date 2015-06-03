#include "DV.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <time.h>
#include <unistd.h>

using namespace std;

DV::DV(const char *filename, const char *self)
{
	fstream topology(filename);

	string line; // current line of file
	string field; // current token (to be put into entry's field)
	char selfName = self[0]; // name of self
	m_self = indexOf(self[0]);

	// initialize m_entries
	for (int dest = 0; dest < NROUTERS; dest++)
	{
		m_entries[dest].setNexthopName('0');
		m_entries[dest].setNexthopPort(-1);
		m_entries[dest].setCost(-1);
		m_entries[dest].setValid();
	}

	while (getline(topology, line)) // parse file line by line
	{
		stringstream linestream(line);
		dv_entry entry;

		entry.setValid();

		// source router
		getline(linestream, field, ',');
		char name = field[0];

		// destination router
		getline(linestream, field, ',');
		int dest = indexOf(field[0]);
		node n;
		n.name = field[0];
		entry.setNexthopName(field[0]);
		cerr << "nexthopname: " << field[0] << endl;

		// destination port number
		getline(linestream, field, ',');
		int port = atoi(field.c_str());
		entry.setNexthopPort(port);
		n.portno = port;
		cerr << "nexthopport: " << port << endl;

		memset((char *)&n.addr, 0, sizeof(n.addr));
		n.addr.sin_family = AF_INET;
		n.addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		n.addr.sin_port = htons(port);

		// link cost
		getline(linestream, field, ',');
		entry.setCost(atoi(field.c_str()));
		cerr << "nexthopcost: " << entry.cost() << endl;

		if (selfName == 'H')
		{
			int i;
			for (i = 0; i < m_neighbors.size(); i++)
			{
				if (m_neighbors[i].name == n.name)
					break;
			}
			if (i == m_neighbors.size())
				m_neighbors.push_back(n);
		}
		else if (name == selfName)
		{
			startTimer(n);
			m_neighbors.push_back(n); // store neighbor
			m_entries[dest] = entry;
		}

		m_portnos[n.name] = n.portno;
	}

	// special port number for sending data packet
	m_portnos['H'] = 11111;

	memcpy((void*)m_entries_backup, (void*)m_entries, sizeof(m_entries));
}

void DV::reset(char dead)
{
	for (int i = 0; i < m_neighbors.size(); i++)
	{
		if (m_neighbors[i].name == dead)
		{
			m_entries_backup[indexOf(dead)].setInvalid();
		}
	}
	memcpy((void*)m_entries, (void*)m_entries_backup, sizeof(m_entries));
}

// update this router's distance vector based on received advertisement from source
// return false if this router's distance vector was not changed
bool DV::update(const void *advertisementBuf, char source)
{
	bool updatedDV = false;

	int intermediate = indexOf(source);
	if (m_entries_backup[intermediate].invalid())
	{
		m_entries_backup[intermediate].setValid();
		m_entries[intermediate].setValid();

		updatedDV = true;
	}

	// load advertised distance vector
	dv_entry advertisement[NROUTERS];
	memcpy((void*)advertisement, advertisementBuf, sizeof(advertisement));
 
	// recalculate self's distance vector
	for (int dest = 0; dest < NROUTERS; dest++)
	{
		if (dest == m_self)
			continue;
		bool updatedEntry = false;
		m_entries[dest].setCost(min(m_entries[dest].cost(), m_entries[intermediate].cost(), advertisement[dest].cost(), updatedEntry));
		if (updatedEntry)
		{
			updatedDV = true;
			m_entries[dest].setNexthopPort(portNoOf(source));
			m_entries[dest].setNexthopName(source);
			cerr << "update: " << m_entries[dest].nexthopPort() << " " << m_entries[dest].nexthopName() << endl;
		}
	}

	return updatedDV;
}

// TODO FIX THE PRINTING
// print the DV
// format: source, destination, port number of nexthop router, cost to destination
void DV::printAll() const {
	cout << "--------------" << endl;
	cout << "DV of ROUTER " << nameOf(m_self) << endl;
	cout << "--------------" << endl;
	time_t rawtime;
	time(&rawtime);
	cout << ctime(&rawtime);
	cout << "dst nexthop cost" << endl;
	for (int dest = 0; dest < NROUTERS; dest++)
	{
		cout << "  " << nameOf(dest) << "   ";
		if (m_entries[dest].nexthopPort() == -1)
			cout << "   ";
		cout << m_entries[dest].nexthopPort() << "   ";
		if (m_entries[dest].cost() != -1)
			cout << " ";
		cout << m_entries[dest].cost();
		cout << endl;
	}
	cout << endl;
};

//-----------------
// HELPER FUNCTIONS
//-----------------

// return minimum cost and set updated flag
int DV::min(int originalCost, int selfToIntermediateCost, int intermediateToDestCost, bool &updated) const {
	int new_cost = selfToIntermediateCost + intermediateToDestCost;

	if (selfToIntermediateCost == -1 || intermediateToDestCost == -1)
	{
		return originalCost;
	}
	else if (originalCost == -1)
	{
		updated = true;
		return new_cost;
	}
	else if (new_cost < originalCost)
	{
		updated = true;
		return new_cost;
	}
	else
	{
		return originalCost;
	}
}

// return index of router
int DV::indexOf(char router) const
{
	return router - 'A';
}

// return name of indexed router
char DV::nameOf(int index) const
{
	return (char)index + 'A';
}

// return port number of router
int DV::portNoOf(char router)
{
	return m_portnos[router];
}

void DV::initMyaddr(int portno)
{
	memset((char *)&m_myaddr, 0, sizeof(m_myaddr));
	m_myaddr.sin_family = AF_INET;
	m_myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	m_myaddr.sin_port = htons(portno);
}

void DV::startTimer(node &n)
{
	clock_gettime(CLOCK_MONOTONIC, &n.startTime);
}

bool DV::timerExpired(node &n) const
{
	timespec tend={0,0};
	clock_gettime(CLOCK_MONOTONIC, &tend);

	if (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)n.startTime.tv_sec + 1.0e-9*n.startTime.tv_nsec) > 5)
		return true;
	else
		return false;
}
