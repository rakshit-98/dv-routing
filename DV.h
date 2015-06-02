#ifndef DV_H
#define DV_H

#include <stdio.h>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <map>
#include <time.h>


#define NROUTERS 6

struct dv_entry
{
	int nexthop; // port number of next hop router
	int cost; // link cost to destination
};

struct node
{
	char name;
	int portno;
  clock_t timer;
	sockaddr_in addr;
};

class DV
{
public:
	DV() {}
	DV(const char *filename, const char *self);
	~DV() {}
	
	dv_entry *getEntries() { return m_entries; }
	int getSize() const { return sizeof(m_entries); }
	char getName() const { return nameOf(m_self); }
	bool update(const void *advertisement, char src);
	int nextHopPortNo(const char dest) const;
	std::vector<node> neighbors() const { return m_neighbors; };
	void printAll() const;
	int portNoOf(char router);
	char nameOf(int index) const;
  void initMyaddr(int portno);
  sockaddr_in myaddr() const { return m_myaddr; }
  void startTimer(char neighbor) { m_neighbors[indexOf(neighbor)].timer = clock(); }
  clock_t getTime(char neighbor) const { return (clock() - m_neighbors[indexOf(neighbor)].timer); }

private:
	// member variables
	int m_self; // index of self
	int m_size;
	dv_entry m_entries[NROUTERS]; // each router's distance vectors
	std::vector<node> m_neighbors; // port numbers of self's neighbors
  sockaddr_in m_myaddr;
	std::map<char, int> m_portnos;

	// helper functions
	int min(int original_cost, int self_to_intermediate_cost, int intermediate_to_dest_cost, bool &updated) const;
	int indexOf(char router) const;
};

#endif
