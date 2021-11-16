#ifndef GEC_H
#define GEC_H

#include "socket_io.h" //a hdr for put/get functions related to socket/data

#define MAX_BURST_LENGTH_GEC 1000

class Gec
{
		int p,q;
    	bool state;
	public:
    	short stat[MAX_BURST_LENGTH_GEC];
    	unsigned char bl; //burst length

    public:
        Gec();
    	void initGec(double, double);
    	bool getState(void);
    	bool getCurrentState(void);
    	int randomNumber(int hi);
    	int write_stat_to_file(FILE *file);
};

#endif
