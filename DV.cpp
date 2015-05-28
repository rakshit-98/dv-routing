#include "DV.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>	// for fprintf
#include <string.h>	// for memcpy
#include <stdlib.h>
#include <stdio.h>

using namespace std;

DV::DV(const char *filename, const char *self)
{
	string sfilename(filename);
	fstream topology(sfilename);

    string line; // current line of file
    string field; // current token (to be put into entry's field)
    m_self = string(self); // name of this router

    while (getline(topology, line)) // parse file line by line
    {
        stringstream linestream(line);
        dv_entry entry;

        // source router
        getline(linestream, field, ',');
        if (field != m_self) // we only take entries for this router's neighbors
        	continue;
        entry.source = field;

		// destination router
        getline(linestream, field, ',');
        entry.destination = field;

        // destination port number
        getline(linestream, field, ',');
        entry.portno = atoi(field.c_str());

        // link cost
        getline(linestream, field, ',');
        entry.distance = atoi(field.c_str());

        m_entries.push_back(entry);
    }

    // print this router's initial distance vector entries
    for (int i = 0; i < m_entries.size(); i++)
    {
    	cerr << m_entries[i].source << " " << m_entries[i].destination << " " << m_entries[i].portno << " " << m_entries[i].distance << " " << endl;
    }
}

// convert m_entries to a buffer (to send as an advertisement to other routers)
const char *DV::getBuffer()
{
	return (char*) 0;
}

// update this router's distance vector based on received advertisement
// return false if this router's distance vector was not changed
bool DV::update(const char *advertisement)
{
	return false;
}

// return port number of next hop router on the least-cost path to the destination
int DV::portNo(const char *destination)
{
	return 0;
}

// return port number of each of this router's neighbors
std::vector<int> DV::neighbors()
{
	std::vector<int> neighbors;
	return neighbors;
}






