#ifndef DV_H
#define DV_H

#include <stdio.h>
#include <vector>
#include <string>

#define NROUTERS 6

struct dv_entry
{
    int nexthop; // port number of next hop router
    int cost; // link cost to destination
};

class DV
{
public:
    DV() {}
    DV(const char *filename, const char *self);
    ~DV() {}
   	
   	const char *getBuffer();
   	bool update(const char *advertisement, char src);
   	int nextHopPortNo(const char dest);
   	std::vector<int> neighbors();
    void printAll();

private:
    // member variables
	int m_self; // index of self
    dv_entry m_entries[NROUTERS]; // each router's distance vectors
    std::vector<int> m_neighbors; // port numbers of self's neighbors

    // helper functions
    int min(int original_cost, int self_to_intermediate_cost, int intermediate_to_dest_cost, bool &updated);
    int indexOf(char router);
    int portNoOf(char router);
    char nameOf(int index);
};

#endif
