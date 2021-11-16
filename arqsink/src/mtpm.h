#ifndef MTPM_H
#define MTPM_H

#include <string>
#include <algorithm>
#include "socket_io.h" //a hdr for put/get functions related to socket/data
#include "gec.h" //a header for channel modeling functions (simple GEC model)
#include "simple_c.h" //a header for channel modeling functions (simple GEC model)
#include "playback_buffer.h" //a hdr for playback buffer functions
//#include "delay_line.h" //a hdr for delay line buffer and related functions
#include "arq_line.h" //a header for ARQ line class
//#include "pbb_arq.h" //a header for global ARQ class
#include "plr_counter.h" //a header for PLR counter class
#include "VQpriority.h" //a header for transmission table class
//#include "fec.h"
#include <sys/stat.h>

#define MAX_PING_TIME	200 //RTT for parent peer that doesn't send any packets
#define MAX_PING_COUNT	3 //a count of attempts to communicate with unresponsive parent peer

class mtpm
{
    private:
        unsigned char   mtratio; //multiple tree ratio (how many parents/children will get this peer in this kind of multiple tee)
        FILE    *plr_f_corr,    //corrected packet loss ratio file
                *plr_f_pure,    //pure packet loss ratio file
                *plr_f_corr2,
                *plr_f_pure2,
                *plr_f_corr3,
                *plr_f_pure3,
                *log_file;
        char    *bfr_in, //a buffer for incoming packets
                *bfr_out; //a buffer for send func
        struct sockaddr_in addr_vlc; //addr_vlc is the address to view video with help of VLC player
        plr_counter plr_c_corr, //packet loss counter after ARQ recovery
                    plr_c_pure; //packet loss counter without any correction method
        struct plr_counter listPLR_corr[3];
        struct plr_counter listPLR_pure[3];
        arq_line    al[MTR], // arq lines for each parent peer
                    al2[MTR],
                    al3[MTR];
        struct arq_line List_al[3];
        unsigned long       p2p_pn[MTR]; //variables for new p2p packet numbers
//        pbb_arq     gal; //global arq line
//        delay_line  dl; //delay line for packets from different parents
        playback_buffer pbb; //playback buffer
//        transmission_table  pt,
//                            pt_g;
//        forward_error_correction fec;
        Gec g[MTR]; //Gilbert model
        simple_c ctrl_c; //control channel model
        int file_n; //a number to mark files of current session
        std::string path;


    public:
        int s;//udp socket
        MyPeer  *my_peer;
        double timeout;
        bool isArqEnabled;

//    private:
//        int get_child ();
//        int get_children ();

    public:
        mtpm();
        virtual ~mtpm();
        int init_mtpm(int argc, char *argv[]);
        int stop_mtpm();
//        int check_arq_line();
//        int check_arq_global();
//        int check_tcp_socket();
//        int check_parent_peers();
        int check_udp_socket();
        int parse_udp_message(int bytes_in, struct sockaddr_in addr);
//        int check_delay_line();
        int check_pbb();
//        int play_fecb();
        bool cmdOptionExists(char** begin, char** end, const std::string& option);
        char* getCmdOption(char ** begin, char ** end, const std::string & option);
};

#endif // MTPM_H
