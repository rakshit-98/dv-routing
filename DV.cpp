#include "DV.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

DV::DV(const char *filename, const char *self)
{
	string sfilename(filename);
	fstream topology(sfilename);

	string line; // current line of file
	string field; // current token (to be put into entry's field)
	char selfName = self[0]; // name of self
	m_self = indexOf(self[0]);

	// initialize m_entries
	for (int dest = 0; dest < NROUTERS; dest++)
	{
		m_entries[dest].nexthop = -1;
		m_entries[dest].cost = -1;
	}

	while (getline(topology, line)) // parse file line by line
	{
		stringstream linestream(line);
		dv_entry entry;

		// source router
		getline(linestream, field, ',');
		char name = field[0];

		// destination router
		getline(linestream, field, ',');
		int dest = indexOf(field[0]);
		node n;
		n.name = field[0];

		// destination port number
		getline(linestream, field, ',');
		int port = atoi(field.c_str());
		entry.nexthop = port;
		n.portno = port;

		memset((char *)&n.addr, 0, sizeof(n.addr));
		n.addr.sin_family = AF_INET;
		n.addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		n.addr.sin_port = htons(port);

		// link cost
		getline(linestream, field, ',');
		entry.cost = atoi(field.c_str());

		if (name == selfName)
		{
			m_neighbors.push_back(n); // store neighbor
			m_entries[dest] = entry;
		}

		m_portnos[n.name] = n.portno;
	}
	cerr << "number of neighbors: " << m_neighbors.size() << endl;
	//printAll();
}

// update this router's distance vector based on received advertisement from source
// return false if this router's distance vector was not changed
bool DV::update(const void *advertisementBuf, char source)
{
	int intermediate = indexOf(source);
	bool updatedDV = false;

	// load advertised distance vector
	dv_entry advertisement[NROUTERS];
	memcpy((void*)advertisement, advertisementBuf, sizeof(advertisement));
 
	// recalculate self's distance vector
	for (int dest = 0; dest < NROUTERS; dest++)
	{
		if (dest == m_self)
			continue;
		bool updatedEntry = false;
		m_entries[dest].cost = min(m_entries[dest].cost, m_entries[intermediate].cost, advertisement[dest].cost, updatedEntry);
		if (updatedEntry)
		{
			updatedDV = true;
			m_entries[dest].nexthop = portNoOf(source);
		}
	}

	//printAll();

	return updatedDV;
}

// return port number of next hop router on the least-cost path to the destination
// return -1 if no port number found
int DV::nextHopPortNo(const char dest)
{
	return m_entries[indexOf(dest)].nexthop;
}

// print the DV
// format: source, destination, port number of nexthop router, cost to destination
void DV::printAll() {
	cerr << getName() << endl;
	cerr << "src dst nexthop cost" << endl;
	for (int dest = 0; dest < NROUTERS; dest++)
	{
		cerr << "  " << nameOf(m_self) << "   ";
		cerr << nameOf(dest) << "   ";
		if (m_entries[dest].nexthop == -1)
			cerr << "   ";
		cerr << m_entries[dest].nexthop << "   ";
		if (m_entries[dest].cost != -1)
			cerr << " ";
		cerr << m_entries[dest].cost;
		cerr << endl;
	}
	cerr << endl;
};

//-----------------
// HELPER FUNCTIONS
//-----------------

// return minimum cost and set updated flag
int DV::min(int originalCost, int selfToIntermediateCost, int intermediateToDestCost, bool &updated) {
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
int DV::indexOf(char router)
{
	return router - 'A';
}

// return name of indexed router
char DV::nameOf(int index)
{
	return (char)index + 'A';
}

// return port number of router
int DV::portNoOf(char router)
{
	return m_portnos[router];
}
