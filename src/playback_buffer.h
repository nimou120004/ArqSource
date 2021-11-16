#ifndef PLAYBACK_BUFFER_H
#define PLAYBACK_BUFFER_H

#include "socket_io.h"
#include "VQpriority.h"

#define MAX_PBBFR_SIZE	134

class playback_buffer
{
    private:
        struct pbb_packet {

            unsigned char	nr;	// repeat number
            unsigned char   nt; //tree number
            unsigned char   gop_num; //group of pictures number
            picture_type    p_t; //picture type
            unsigned short  pl_size; //payload size
            unsigned long	p2p_pn; //packet number between this peer and one of its children
            unsigned long	pn; //global packet number
            pbb_packet		*next; // a pointer to next pbb_packet (4 b)
            char            data [MTU_SIZE]; //video data

        };
        int length; //current length of a buffer

	public:
		pbb_packet *first_packet, //first packet in a buffer
                   *last_packet, //last packet in a buffer
					new_packet; //a variable for new arrived packet
		int max_length; //maximum length

    private:
        int copy_packet (pbb_packet *from, pbb_packet *to);
        int add_packet (pbb_packet *packet);
        int send_first_packet (int s, struct sockaddr_in addr); //send packet
		int delete_first_packet(); //delete only first packet from a buffer
		int show_pn(); //printf packets from pbb
		int clear_pbb (); //delete all packets from a buffer

    public:
        playback_buffer(); //ctor
        virtual ~playback_buffer(); //dtor
        bool shift_buffer (); //add packet without sending first packet
		bool shift_buffer (int s, struct sockaddr_in addr, unsigned long &pn); //add new packet and send first packet
		int get_packet_by_pn (unsigned long pn); //get data from pn packet into new_packet var
		int get_packet_by_p2p_pn (unsigned long p2p_pn, unsigned char nt);//get data from p2p_pn packet into new_packet var
		int get_pn_by_p2p_pn (unsigned long p2p_pn, unsigned char nt, unsigned long &pn);
};
#endif
