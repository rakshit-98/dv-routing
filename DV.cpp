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
    cerr << "initial distance vector entries:" << endl;
    for (int i = 0; i < m_entries.size(); i++)
    {
    	cerr << m_entries[i].source << " ";
        cerr << m_entries[i].destination << " ";
        cerr << m_entries[i].portno << " ";
        cerr << m_entries[i].distance << " " << endl;
    }
}

// convert m_entries to a buffer (to send as an advertisement to other routers)
const char *DV::getBuffer()
{
    string buffer = "";
    for (vector<dv_entry>::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        buffer += it->source + "," + it->destination + "," + to_string(it->portno) + "," + to_string(it->distance) + "\n";
    }
    cerr << "buffer:" << endl << buffer;
	return buffer.c_str();
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
    cerr << "neighbors:" << endl;
    for (vector<dv_entry>::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if (it->source == m_self)
        {
            neighbors.push_back(it->portno);
            cerr << it->portno << " ";
        }
    }
    cerr << endl;
	return neighbors;
}