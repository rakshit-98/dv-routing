#ifndef DV_H
#define DV_H

#include <stdio.h>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <map>
#include <time.h>
#include <ctime>


#define NROUTERS 6

struct dv_entry
{
public:	
	int nexthopPort() const { return (invalid() ? -1 : m_nexthopPort); }
	char nexthopName() const { return (invalid() ? '0' : m_nexthopName); }
	int cost() const { return (invalid() ? -1 : m_cost); }
	bool invalid() const { return m_invalid; }

	void setNexthopPort(int n) { m_nexthopPort = n; }
	void setNexthopName(char n) { m_nexthopName = n; }
	void setCost(int c) { m_cost = c; }
	void setValid() { m_invalid = false; }
	void setInvalid() { m_invalid = true; }
private:
	bool m_invalid;
	int m_nexthopPort; // port number of next hop router
	char m_nexthopName;
	int m_cost; // link cost to destination
};

struct node
{
	char name;
	int portno;
	timespec startTime;
	sockaddr_in addr;
};

class DV
{
public:
	DV() {}
	DV(const char *filename, const char *self);
	~DV() {}
	
	void reset(char dead);
	dv_entry *getEntries() { return m_entries; }
	int getSize() const { return sizeof(m_entries); }
	char getName() const { return nameOf(m_self); }
	bool update(const void *advertisement, char src);
	dv_entry routeTo(const char dest) const { return m_entries[indexOf(dest)]; };
	std::vector<node> neighbors() const { return m_neighbors; };
	void printAll() const;
	int portNoOf(char router);
	char nameOf(int index) const;
	int indexOf(char router) const;
	void initMyaddr(int portno);
	sockaddr_in myaddr() const { return m_myaddr; }
	void startTimer(node &n);
	bool timerExpired(node &n) const;
	int port() { return portNoOf(getName()); }

private:
	// member variables
	int m_self; // index of self
	int m_size;
	dv_entry m_entries[NROUTERS]; // each router's distance vectors
	dv_entry m_entries_backup[NROUTERS]; // initial distance vectors (for resetting)
	std::vector<node> m_neighbors; // port numbers of self's neighbors
	sockaddr_in m_myaddr;
	std::map<char, int> m_portnos;

	// helper functions
	int min(int original_cost, int self_to_intermediate_cost, int intermediate_to_dest_cost, bool &updated) const;
};

#endif
