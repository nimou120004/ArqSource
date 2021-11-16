#include "socket_io.h"

int put_uc (char *bfr, int n, unsigned char x)
{
	bfr[n] = (unsigned char) x;
	return EXIT_SUCCESS;
}

int put_ul (char *bfr, int n, unsigned long x)
{
	bfr[n] = (unsigned char) (x >> 24) & 0xFF;
    bfr[n + 1] = (unsigned char) (x >> 16) & 0xFF;
    bfr[n + 2] = (unsigned char) (x >> 8) & 0xFF;
    bfr[n + 3] = (unsigned char) x & 0xFF;
    return EXIT_SUCCESS;
}

int put_us (char *bfr, int n, unsigned short x)
{
	bfr[n] = (unsigned char) (x >> 8) & 0xFF;
    bfr[n + 1] = (unsigned char) x & 0xFF;
    return EXIT_SUCCESS;
}

int put_f (char *bfr, int n, float x)
{
	unsigned char *bytes = (unsigned char *) &x;
	unsigned int i;
	for (i = 0; i < sizeof (x); i++)
		bfr[n + i] = bytes[i];
    return EXIT_SUCCESS;
}

unsigned char get_uc (char *bfr, int n)
{
	return (unsigned char)bfr[n];
}
unsigned short get_us (char *bfr, int n)
{
	return ((unsigned char)bfr[n] << 8) + (unsigned char)bfr[n+1];
}
unsigned long get_ul (char *bfr, int n)
{
	return ((unsigned char)bfr[n] << 24) + ((unsigned char)bfr[n + 1] << 16) + ((unsigned char)bfr[n + 2] << 8) + (unsigned char)bfr[n + 3];
}
float get_f (char *bfr, int n)
{
	float value;
	unsigned char bytes[sizeof value];
	unsigned int i;
	for (i = 0; i < sizeof (value); i++)
		bytes[i] = bfr[n + i];
	value = *((float *) bytes);
	return value;
}
unsigned char get_x (char *bfr, int n)
{
	return get_uc (bfr, n + 1);
}
unsigned long GetTickCount()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec;
}
