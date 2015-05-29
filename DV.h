#ifndef DV_H
#define DV_H

#include <stdio.h>
#include <vector>
#include <string>

class DV
{
public:
    DV() {}
    DV(const char *filename, const char *self);
    ~DV() {}

   	const char *getBuffer();
   	bool update(const char *advertisement);
   	int portNo(const char *destination);
   	std::vector<int> neighbors();

private:
    struct dv_entry
  	{
  		std::string source;
  		std::string destination;
  		int portno;
  		int distance;
  	};

  	std::string m_self;
    std::vector<dv_entry> m_entries;
};

#endif