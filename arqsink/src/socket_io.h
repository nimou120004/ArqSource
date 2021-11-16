#ifndef SOCKET_IO_H
#define SOCKET_IO_H

//socket programming
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdarg.h>
#include <time.h>

#define MAX_PACKET_NUMBER  0xFFFFFFFF

#define MTU_SIZE        1500 //maximum transmission unit
#define VIDEO_CHUNCK_SIZE 188
#define VIDEO_SIZE      1314 //7*188
#define RTP_HDR_SIZE    12 //12(rtp_hdr) + 7*188
#define ARQ_HDR_SIZE	13 //arq hdr size with id
#define FEC_HDR_SIZE	4   //fec hdr size with id
#define FEC_VIDEO_HDR_SIZE 5
#define FEC_RED_HDR_SIZE   7

#define MTR				1	//multiple tree ratio
#define MAX_ACCEPTED_PEERS 100

#define IDM_TCP_IP_PORT     10  //a message with ip and port of accepted peer
#define IDM_TCP_OFF         2   //accepted peer is off
#define IDM_TCP_DELETED     8   //accepted peer is deleted
#define IDM_TCP_COMPLAINT   4   //a complaint on one of parents of accepted peer
#define IDM_TCP_PING        9   //a request of activity to one of accepted peers
#define IDM_TCP_ACTIVE      1   //an approval of activity from one of accepted peers
#define IDM_TCP_PLR         3   //a message with plr data from one of accepted peers
#define IDM_TCP_CHILD       0   //a message with data about one of children of accepted peer
#define IDM_TCP_CHILDREN    5   //a message with data about all children of accepted peer
#define IDM_TCP_CHILDREN_N  6   // that is no current child in messages IDM_TCP_CHILD and IDM_TCP_CHILDREN
#define IDM_TCP_CHILDREN_Y  10  //current child exists in messages IDM_TCP_CHILD and IDM_TCP_CHILDREN

#define IDM_UDP_ACTIVE      1   //an approval of activity from one of accepted peers
#define IDM_UDP_PING        9   //a request of activity to one of accepted peers

#define IDM_UDP_ARQ_VIDEO   7   //a packet with video content (ARQ)
#define IDM_UDP_ARQ_VIDEO_R 8   //restored packet with video content (ARQ)
#define IDM_UDP_ARQ_NACK_AL 12  //nack for arq line (ARQ)
#define IDM_UDP_ARQ_DNWM_AL 13  //do not wait message for arq line (ARQ)
#define IDM_UDP_ARQ_NACK_G  14  //nack for global arq (ARQ)
#define IDM_UDP_ARQ_DNWM_G  15  //do not wait message for global arq (ARQ)

#define IDM_UDP_FEC_VIDEO   20  //a packet with video content (FEC)
#define IDM_UDP_FEC_RED     21  //redundancy packet (FEC)
#define IDM_UDP_DL_IS_BUSY  255 //a message from delay line when there is no packet to unbuf (DELAY LINE)

#define ID_ERR_STOP         100

struct Peer {

	unsigned char			to_delete;	//flag
    unsigned char           ping_n;     //unconfirmed activity requests number
    unsigned char           m;			//m = n % mtratio
    unsigned short          n;			//peer number
    Peer			*child[MTR], //child peers
    				*parent[MTR];//parent peers
    long            ping_t;       //last ping time
    long            plr_t;		//arrival time of last plr message
    float           plr;		//peer packet loss ratio
    struct sockaddr_in     addr;  //peer address

};

struct Root {

    Peer *child[MTR];

};

struct MyPeer {

    unsigned char           ping_n;// activity request number
    unsigned char           m;//m = n % MTR
    unsigned short          n;//peer number in order of connection
    MyPeer			*child[MTR], //pointers to children structures
    				*parent[MTR];//pointers to parents structures
    long            ping_t;//last activity time
    struct sockaddr_in     addr;//address structure for udp socket

};

int put_uc (char *bfr, int n, unsigned char x);
int put_us (char *bfr, int n, unsigned short x);
int put_ul (char *bfr, int n, unsigned long x);
int put_f (char *bfr, int n, float x);

unsigned char get_uc (char *bfr, int n);
unsigned short get_us (char *bfr, int n);
unsigned long get_ul (char *bfr, int n);
float get_f (char *bfr, int n);
unsigned char get_x (char *bfr, int n);

unsigned long GetTickCount();
#endif
