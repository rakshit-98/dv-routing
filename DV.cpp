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
        if (field[0] != selfName) // only take entries for this router's neighbors
        	continue;

		// destination router
        getline(linestream, field, ',');
        int dest = indexOf(field[0]);

        // destination port number
        getline(linestream, field, ',');
        int port = atoi(field.c_str());
        entry.nexthop = port;
        m_neighbors.push_back(port); // store neighbor

        // link cost
        getline(linestream, field, ',');
        entry.cost = atoi(field.c_str());

        m_entries[dest] = entry;
    }

    printAll();
}

// TODO: not sure if this works
// convert m_entries to a buffer (to send as an advertisement to other routers)
const char *DV::getBuffer()
{
	return (char*)m_entries;
}

// update this router's distance vector based on received advertisement from source
// return false if this router's distance vector was not changed
bool DV::update(const char *advertisementBuf, char source)
{
    int intermediate = indexOf(source);
    bool updatedDV = false;

    // load advertised distance vector
    dv_entry advertisement[NROUTERS];
    memcpy((void*)advertisement, (void*)advertisementBuf, sizeof(advertisement));
 
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

    printAll();

	return updatedDV;
}

// return port number of next hop router on the least-cost path to the destination
// return -1 if no port number found
int DV::nextHopPortNo(const char dest)
{
	return m_entries[indexOf(dest)].nexthop;
}

// return port number of each of this router's neighbors
std::vector<int> DV::neighbors()
{
    cerr << "neighbors:" << endl;
    for (int i = 0; i < m_neighbors.size(); i++)
    {
        cerr << m_neighbors[i] << endl;
    }
    cerr << endl;
	return m_neighbors;
}

// print the DV
// format: source, destination, port number of nexthop router, cost to destination
void DV::printAll() {
    cerr << "distance vector entries:" << endl;
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

// return port number of router
int DV::portNoOf(char router)
{
    return router - 'A' + 10000;
}

// return name of indexed router
char DV::nameOf(int index)
{
    return (char)index + 'A';
}
