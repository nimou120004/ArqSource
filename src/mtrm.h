#ifndef MTRM_H
#define MTRM_H

#include <string>
#include <time.h>
#include <algorithm>
#include "gec.h"
#include "playback_buffer.h"
#include "MPEG_parser.h"

#define AVR_PLR_DT  10000
#define MAX_PLR_TIME 8000
#define MAX_PLR_COUNT 2
#define MAX_PN	0xFFFFFFFF
#define PLR_INIT_VALUE 2

class mtrm //multiple tree root machine
{
    private:
        bool isStarted; //isStarted==true when first packet has arrived via udp socket
        unsigned char mtratio; //multiple tree ratio
        int	total_ap; //total accepted peers
        char *bfr_in, //recv buffer
             *bfr_out; //send buffer
        unsigned short lost_numbers[MAX_ACCEPTED_PEERS]; //a list of deleted peer numbers
        unsigned char lost_numbers_count;//a count of lost peer numbers
        long starttime;//time when first packet arrived via udp socket
        FILE *log_file, //a file with info about accepted peers
//             *avr_plr_file, //a file with average packet loss rate
             *init_file, //a file with initial setup
             *mpeg_file; //a file with picture types
        unsigned long gal_pn; //new global packet number for video packets
        unsigned long p2p_pn[MTR]; //packet numbers for video packets between two peers
        unsigned char tree_number; //tree_number of current packet
        playback_buffer pbb; //arq buffer
        transmission_table pt, pt_g; //priority tables
        Gec g; //Gilbert-Elliott model
        unsigned char  prev_pic_type, //тип кадра, содержащегося в предыдущем пакете
               pred_n,   //номер предсказаного кадра
               gop_n;    //номер группы кадров
        long   prev_avr_plr_t, //a time when previous plr was counted
               avr_plr_dt;

    public:
        Root *root; //a root of multiple tree
        int s; //udp socket
        bool isArqEnabled,
             isVqpEnabled; //video quality priority
        double timeout; //poll timeout

    private:
        int attach_peer(Peer *peer);
        int delete_peer(Peer * peer);
        int delete_tree();
        float count_average_plr ();
        int write_plr_to_file();
        int put_child (Peer *peer, unsigned char i);
        int put_children(Peer *peer);
        bool cmdOptionExists(char** begin, char** end, const std::string& option);
        char* getCmdOption(char ** begin, char ** end, const std::string & option);

    public:
        mtrm();
        virtual ~mtrm();
        int init_mtrm(int argc, char* argv[]);
        int stop_mtrm();
        int check_vqp();
        int check_udp_socket();

};

#endif // MTRM_H
