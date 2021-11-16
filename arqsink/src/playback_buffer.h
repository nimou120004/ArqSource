#ifndef PLAYBACK_BUFFER_H
#define PLAYBACK_BUFFER_H

#include "socket_io.h"
#include "VQpriority.h"
#include <string>

#define MAX_PBBFR_SIZE	134
#define MAX_PBBFR_TIME  1000000000 //nanoseconds for timespec var

class playback_buffer
{
    private:
        struct pbb_packet {

            unsigned char	nr;	// repeat number
            unsigned char   nt; //tree number
            //gop and p_t fields are for vqpririty algorithm
            unsigned char   gop_num; //group of pictures number
            picture_type    p_t; //picture type
            unsigned short  pl_size; // payload size
            unsigned long	p2p_pn; //packet number between this peer and one of its children
            unsigned long	pn; //global packet number
            pbb_packet		*next; // a pointer to next pbb_packet (4 b)
            char            data [MTU_SIZE]; //video data
            std::string     packet_id;

        };

	public:
	    bool isReadyToPlay; //check is required time interval has passed before video playing
		pbb_packet *first_packet, //first packet in a buffer
                   *last_packet, //last packet in a buffer
					new_packet; //a variable for new arrived packet
        int length; //current length of a buffer
        //time or length can be used to bound the buffer
		int max_size; //maximum length
		unsigned long max_time; //maximum buffer time (0..10e10 nanoseconds)
		unsigned long ts0, //first packet timestamp
                      ts_lp; //last packet timestamp

    private:
        int copy_packet (pbb_packet *from, pbb_packet *to);
        int send_first_packet (int s, struct sockaddr_in addr); //send packet
		int delete_first_packet(); //delete only first packet from a buffer
		int show_pn(); //packets from pbb
		int clear_pbb (); //delete all packets from a buffer

    public:
        playback_buffer(); //ctor
        virtual ~playback_buffer(); //dtor
        int add_packet (pbb_packet *packet); //add new packet without checking buffer size
        bool play(int s, struct sockaddr_in addr, unsigned long &pn, unsigned char &nr, std::string &id); //for sink part of an application
        bool shift_buffer (); //add packet without sending first packet
		bool shift_buffer (int s, struct sockaddr_in addr, unsigned long &pn); //add new packet and send first packet
		bool check_buffer (int s, struct sockaddr_in addr, unsigned long &pn); //for source part of an applicatoin
		int get_packet_by_pn (unsigned long pn); //get data from pn packet into new_packet var
		int get_packet_by_p2p_pn (unsigned long p2p_pn, unsigned char nt);//get data from p2p_pn packet into new_packet var
		int get_pn_by_p2p_pn (unsigned long p2p_pn, unsigned char nt, unsigned long &pn);
};
#endif
