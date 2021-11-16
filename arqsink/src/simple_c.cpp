#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simple_c.h"

simple_c::simple_c()
{
    p = 0;
    q = 0;
    c = 0;
    l = 0;
}
bool simple_c::error()
{
    int tmp=randomNumber(10000);
    bool r = (tmp>=5000-p)&& (tmp<=5000+p);
    if (r)
        l++;
    c++;
    if (c==TM_SIMPLE_C)
    {
        printf ("(%2.2f)",(float)l/c);
        c=0;
        l=0;
    }
    return r;
}
int simple_c::randomNumber(int hi)
{
    // [0,1)
    float scale = rand()/float(RAND_MAX);
    // [0,hi]
    return int((float)scale*hi);
}
void simple_c::init_c(double plr)
{
    srand( (unsigned)time( NULL ) );
    //based on G model
    q = pow(1.0-plr,1.0/1341.0);
    p = 5000.0*(1.0-pow(q,150));
    return;
}
