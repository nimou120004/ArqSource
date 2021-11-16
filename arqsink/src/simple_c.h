#ifndef SIMPLE_C_H_INCLUDED
#define SIMPLE_C_H_INCLUDED

#include "socket_io.h"

#define TM_SIMPLE_C          100

class simple_c
{
    public:
        int p;
        double q;
        int c;//packet counter
        int l;//packet loss

    public:
        simple_c();
        bool error();
        int randomNumber(int hi);
        void init_c(double plr);
};

#endif // SIMPLE_C_H_INCLUDED
