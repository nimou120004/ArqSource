#include <sys/stat.h>
//#include <time.h>
#include "mtrm.h"

mtrm::mtrm()
{
    //ctor
    bfr_in = NULL;
    bfr_out = NULL;
    total_ap = 0; //total accepted peers
    starttime = 0; //for counting of AVR PLR
    prev_avr_plr_t = 0;
    avr_plr_dt = AVR_PLR_DT;
    lost_numbers_count = 0;
    tree_number = 0; //a number of tree for current packet
    gal_pn = 0; //global packet number for arq
    for (int i = 0; i < MTR; i++) p2p_pn[i] = 0; //arq line packet number
    isStarted = false;
    mtratio = MTR; //multiple tree ratio
    isArqEnabled = true;
    isVqpEnabled = true;
    timeout = 1.0;
}

mtrm::~mtrm()
{
    //dtor
    stop_mtrm();
}
char* mtrm::getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool mtrm::cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}
int mtrm::stop_mtrm()
{
    //to delete all peers
    delete_tree ();
    close(s);
	delete (root);
	if (log_file)
        fclose(log_file);

//    fclose(avr_plr_file);
    fclose(mpeg_file);
    return EXIT_SUCCESS;
}
int mtrm::init_mtrm(int argc, char* argv[])
{
    //input data
    unsigned long ip = inet_addr ("127.0.0.1"); //ip address of root peer
    unsigned short port = htons (5010);	//the port, where videocontent is captured
    unsigned long ip_sink = inet_addr ("127.0.0.1");
    unsigned short port_sink = htons (5018);
    double lb = 0; //bundle length
    double ploss = 0; //packet loss
    int max_pbbfr_size = MAX_PBBFR_SIZE;
    //argv parsing
    char *option = NULL;
    option = getCmdOption(argv, argv + argc, "-ip_s");
    if (option)
        ip_sink = inet_addr(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-port_s");
    if (option)
        port_sink = htons((unsigned short)atoi(std::string(option).c_str()));
    option = getCmdOption(argv, argv + argc, "-ip");
    if (option)
        ip = inet_addr(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-port");
    if (option)
        port = htons((unsigned short)atoi(std::string(option).c_str()));
    option = getCmdOption(argv, argv + argc, "-ploss");
    if (option)
        ploss = (double)atof(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-lb");
    if (option)
        lb = (double)atof(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-bsize");
    if (option)
        max_pbbfr_size = atoi(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-timeout");
    if (option)
        timeout = (double)atof(std::string(option).c_str());
    isArqEnabled = !cmdOptionExists(argv, argv + argc, "-noarq");
    isVqpEnabled = !cmdOptionExists(argv, argv + argc, "-novqp");
    int size_sa = sizeof(struct sockaddr);
    //show input values
    struct sockaddr_in addr_temp;
    memset (&addr_temp, 0, size_sa);
    addr_temp.sin_family = AF_INET;
    addr_temp.sin_addr.s_addr = ip;
    printf ("source=%s:%d ",inet_ntoa(addr_temp.sin_addr),ntohs(port));
    addr_temp.sin_addr.s_addr = ip_sink;
    printf ("sink=%s:%d ploss=%2.4f lb=%2.2f bsize=%d timeout=%4.2fms arq=%s vqp=%s\n", inet_ntoa(addr_temp.sin_addr), ntohs(port_sink), ploss, lb, max_pbbfr_size, timeout, isArqEnabled ? "true" : "false", isVqpEnabled ? "true" : "false");

	int i;
    root = new Root;
    //root info
    for (i = 0; i < mtratio; i ++)
		root->child[i] = NULL;

    bfr_in = new char[MTU_SIZE], //buffer for recieve packets
    bfr_out = new char[MTU_SIZE]; //buffer for send packets
    //to create udp socket
    s = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == -1)
	{
        printf("Error %d: socket s descriptor is invalid. \n", errno);
        return EXIT_FAILURE;
    }
    //non-blocking mode for s
    if (fcntl(s, F_SETFL, O_NONBLOCK)) {
	    printf("Error %d: socket s could not be set to nonblocking mode.\n", errno);
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = port;
    if (bind (s, (struct sockaddr*) &addr, sizeof(struct sockaddr)) == -1)
	{
        printf("Error %d: unable to bind socket s.\n", errno);
        return EXIT_FAILURE;
    }
    //statistics
    init_file = fopen ("N.txt","a+");
    fclose(init_file);
    init_file = fopen ("N.txt","r+");
    int file_n=0; //serial number of current statistics files
    if (!feof(init_file))
        fscanf(init_file, "%d", &file_n);
    file_n++;
    rewind(init_file);
    fprintf(init_file, "%d", file_n);
    fclose(init_file);

    char s_temp[100]; //temporary string for sprintf func
    sprintf(s_temp,"%d",file_n);
    std::string s_dir = "Res";
    mkdir(s_dir.c_str(), 0777);
    s_dir += "//Source";
    mkdir(s_dir.c_str(), 0777);
    std::string s_path = s_dir + "//test_" + std::string(s_temp) + "_slog.txt";
    log_file = fopen (s_path.c_str(), "w");
//    s_path = s_dir + "//avr_plr(t)_" + std::string(s_temp) + ".csv";
//    avr_plr_file = fopen(s_path.c_str(),"w");
    //an array for numbers of deleted peers
    for (i = 0; i < MAX_ACCEPTED_PEERS; i++) lost_numbers[i] = 0;
    s_path = s_dir + "//test_" + std::string(s_temp) + "_mpeg_p_type.txt";
    mpeg_file = fopen(s_path.c_str(),"w+");
    //Playback buffer
    pbb.max_length = max_pbbfr_size;
    //GEC channel
    g.initGec(ploss, lb);
    //for VQPriority
    pred_n = 0xFF;   //number of predicted frame
    gop_n = 0xFF;    //number of frame group
    printf("Waiting for connections... \n");
    //add child
    Peer *peer_new = new Peer; //empty peer
	peer_new->ping_n = 0;
	peer_new->to_delete = 0;
	peer_new->addr.sin_family = AF_INET;
    peer_new->addr.sin_addr.s_addr = ip_sink;
    peer_new->addr.sin_port = port_sink;
    attach_peer(peer_new);
    return EXIT_SUCCESS;
}
int mtrm::check_vqp()
{
    if (isVqpEnabled)
    {
        int j,
            bytes_out;
        struct sockaddr_in saved_addr;
        saved_addr.sin_family = AF_INET;
        unsigned char nr = 0;
        unsigned char nt;
        unsigned long got_sn;
        got_sn = pt.get_sn_to_send(&saved_addr, &nr, &nt);
        while (!pt.isEmpty)
        {
            fprintf(log_file,"AL%d nr=%d sn=%lu",nt,nr,got_sn);
            //pt.print(log_file);
            if (pbb.first_packet->pn <= got_sn)
            {
                if (pbb.get_packet_by_pn(got_sn) == EXIT_SUCCESS)
                {
                    fprintf (log_file," p2p_pn=%lu",pbb.new_packet.p2p_pn);
                    put_uc (bfr_out, 0, IDM_UDP_ARQ_VIDEO);
                    put_uc (bfr_out, 1, nr);
                    put_uc (bfr_out, 2, pbb.new_packet.nt);
                    put_ul (bfr_out, 3, pbb.new_packet.pn);
                    put_ul (bfr_out, 7, pbb.new_packet.p2p_pn);
                    for (j = 0; j < pbb.new_packet.pl_size; j++)
                        bfr_out[j + ARQ_HDR_SIZE] = pbb.new_packet.data[j];
                    if ((bytes_out = sendto(s, bfr_out, pbb.new_packet.pl_size + ARQ_HDR_SIZE, 0, (struct sockaddr *) &saved_addr, sizeof(struct sockaddr))) < 0)
                        fprintf(log_file, " Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_VIDEO);
                    else
                    {
                        printf ("r");
                        fprintf (log_file," sent\n");
                    }

                }
                else
                    fprintf (log_file," not in buffer\n");
            }
            else
            {
                put_uc (bfr_out, 0, IDM_UDP_ARQ_DNWM_AL);
                put_uc (bfr_out, 1, nt);
                put_ul (bfr_out, 2, got_sn);
                put_uc (bfr_out, 6, 1);
                if ((bytes_out = sendto(s, bfr_out, 7, 0, (struct sockaddr *) &saved_addr, sizeof(struct sockaddr))) < 0)
                   fprintf(log_file," Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_DNWM_AL);
                else
                   fprintf (log_file," dnwm\n");
            }
            got_sn = pt.get_sn_to_send(&saved_addr, &nr, &nt);
        }
        got_sn = pt_g.get_sn_to_send(&saved_addr, &nr, &nt);
        if (!pt_g.isEmpty)
        {
            fprintf(log_file,"GAL%d nr=%d sn=%lu",nt,nr,got_sn);
            //pt_g.print(log_file);
            if (pbb.first_packet->pn <= got_sn)
            {
                if (pbb.get_packet_by_pn(got_sn) == EXIT_SUCCESS)
                {
                    put_uc (bfr_out, 0, IDM_UDP_ARQ_VIDEO_R);
                    put_uc (bfr_out, 1, nr);
                    put_uc (bfr_out, 2, nt);
                    put_ul (bfr_out, 3, pbb.new_packet.pn);
                    put_ul (bfr_out, 7, 0);
                    for (j = 0; j < pbb.new_packet.pl_size; j++)
                        bfr_out[j + ARQ_HDR_SIZE] = pbb.new_packet.data[j];
                    if ((bytes_out = sendto(s, bfr_out, pbb.new_packet.pl_size + ARQ_HDR_SIZE, 0, (struct sockaddr *) &saved_addr, sizeof(struct sockaddr))) < 0)
                        fprintf(log_file, " Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_VIDEO);
                    else
                    {
                        printf ("R");
                        fprintf (log_file," sent\n");
                    }


                }
                else
                    fprintf (log_file," not in buffer\n");
            }
            else
            {
                put_uc (bfr_out, 0, IDM_UDP_ARQ_DNWM_G);
                put_uc (bfr_out, 1, nt);
                put_ul (bfr_out, 2, got_sn);
                put_uc (bfr_out, 6, 1);
                if ((bytes_out = sendto(s, bfr_out, 7, 0, (struct sockaddr *) &saved_addr, sizeof(struct sockaddr))) < 0)
                    fprintf(log_file, " Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_DNWM_G);
                else
                    fprintf (log_file," dnwm\n");
            }
            got_sn = pt_g.get_sn_to_send(&saved_addr, &nr, &nt);
        }
        return EXIT_SUCCESS;
    }
    else
        return EXIT_FAILURE;
}


int mtrm::check_udp_socket()
{
    int i,j,
        //k,
        bytes_in,
        bytes_out;
    struct sockaddr_in addr;
    int size_sa = sizeof(struct sockaddr); //a size of socket address
    memset(&addr, 0, size_sa);
    addr.sin_family = AF_INET;
    //root recieves data from vlc-player
    bytes_in = recvfrom(s, bfr_in, MTU_SIZE, 0, (struct sockaddr *) &addr, (socklen_t*)&size_sa);
    while (bytes_in > 0)
    {
        unsigned char id = get_uc (bfr_in, 0); //messages are distinguisged by this id
        if (id == IDM_UDP_PING)
        {
            //activity request from one of child peers
            put_uc (bfr_out, 0, IDM_UDP_ACTIVE);
            if ((bytes_out = sendto( s, bfr_out, 1, 0, (struct sockaddr *) &addr, size_sa)) < 0)
                fprintf (log_file," Error %d: can't send M%d to peer.\n", errno, 1);
            for (i = 1; i < bytes_in; i++)
                bfr_in[i - 1] = bfr_in[i];
            bytes_in -= 1;
        }
        else if (isArqEnabled && id == IDM_UDP_ARQ_NACK_AL)
        {
            //NACK in arq line
            unsigned char nack_n = get_uc(bfr_in, 1);
            unsigned char nack_bc = get_uc(bfr_in, 2);
            unsigned char nack_nt = get_uc (bfr_in, 3);
            fprintf (log_file,"ADD TO AL%d nr=%d\n", nack_nt,nack_n);
            for (i = 0; i < nack_bc; i++)
            {
                unsigned long nack_pn = get_ul (bfr_in, i*5 + 4);
                unsigned char nack_bl = get_uc (bfr_in, i*5 + 8);
                unsigned char nack_dwbl = 0;
                unsigned long nack_fpn = nack_pn;
                while (nack_bl > 0)
                {
                    if (pbb.get_packet_by_p2p_pn(nack_pn, nack_nt) == EXIT_SUCCESS)
                    {
                        if (isVqpEnabled)
                        {
                            long number_in_buffer = pbb.new_packet.pn - pbb.first_packet->pn;
                            unsigned short priority = pt.calcute_priority(pbb.new_packet.p_t, 1000.0 / 134.0 * (number_in_buffer + 1));
                            pt.add_to_transmission_table(priority, pbb.new_packet.pn, addr, nack_n, nack_nt);
                            fprintf(log_file,"sn=%lu p2p_pn=%lu priority=%hu\n",pbb.new_packet.pn,pbb.new_packet.p2p_pn,priority);
                        }
                        else
                        {
                            fprintf (log_file," p2p_pn=%lu",pbb.new_packet.p2p_pn);
                            put_uc (bfr_out, 0, IDM_UDP_ARQ_VIDEO);
                            put_uc (bfr_out, 1, nack_n);
                            put_uc (bfr_out, 2, pbb.new_packet.nt);
                            put_ul (bfr_out, 3, pbb.new_packet.pn);
                            put_ul(bfr_out, 7, pbb.new_packet.p2p_pn);
                            for (j = 0; j < pbb.new_packet.pl_size; j++)
                                bfr_out[j + ARQ_HDR_SIZE] = pbb.new_packet.data[j];
                            if ((bytes_out = sendto(s, bfr_out, pbb.new_packet.pl_size + ARQ_HDR_SIZE, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0)
                                fprintf(log_file, " Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_VIDEO);
                            else
                            {
                                fprintf (log_file," sent\n");
                                printf ("r");
                            }

                        }
                    }
                    else
                        nack_dwbl++;
                    nack_pn++;
                    nack_bl--;
                }//while (nack_bl>0)
                if (nack_dwbl > 0)
                {
                    put_uc (bfr_out, 0, IDM_UDP_ARQ_DNWM_AL);
                    put_uc (bfr_out, 1, nack_nt);
                    put_ul (bfr_out, 2, nack_fpn);
                    put_uc (bfr_out, 6, nack_dwbl);
                    if ((bytes_out = sendto(s, bfr_out, 7, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0)
                        fprintf(log_file,"Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_DNWM_AL);
                    else
                        fprintf(log_file,"SENT DNWM sn=%lu fpn=%lu dwbl=%d\n",nack_pn,nack_fpn,nack_dwbl);

                }
            }//for (nack.bc)
            unsigned char nack_size = nack_bc * 5 + 4;
            for (i = nack_size; i < bytes_in; i++)
                bfr_in[i - nack_size] = bfr_in[i];
            bytes_in -= nack_size;
        }// if (id == 12)
        else if (isArqEnabled && id == IDM_UDP_ARQ_NACK_G)
        {
            //global NACK
            printf (" M%d", id);
            unsigned char nack_n = get_uc(bfr_in, 1);
            unsigned char nack_bc = get_uc(bfr_in, 2);
            unsigned char nack_nt = get_uc (bfr_in, 3);
            fprintf(log_file,"ADD TO GAL%d nr=%d\n",nack_nt,nack_n);
            for (i = 0; i < nack_bc; i++)
            {
                unsigned long nack_pn = get_ul (bfr_in, i*5 + 4);
                unsigned char nack_bl = get_uc (bfr_in, i*5 + 8);
                unsigned char nack_dwbl = 0;
                unsigned long nack_fpn = nack_pn;
                while (nack_bl > 0)
                {
                    if (pbb.get_packet_by_pn(nack_pn) == EXIT_SUCCESS)
                    {
                        if (isVqpEnabled)
                        {
                            long number_in_buffer = pbb.new_packet.pn - pbb.first_packet->pn;
                            unsigned short priority = pt.calcute_priority(pbb.new_packet.p_t, 1000.0 / 134.0 * (number_in_buffer + 1));
                            pt_g.add_to_transmission_table(priority, pbb.new_packet.pn, addr, nack_n, nack_nt);
                            fprintf(log_file,"sn=%lu p2p_pn=%lu priority=%hu\n",nack_pn,pbb.new_packet.p2p_pn,priority);
                        }
                        else
                        {
                            put_uc (bfr_out, 0, IDM_UDP_ARQ_VIDEO_R);
                            put_uc (bfr_out, 1, nack_n);
                            put_uc (bfr_out, 2, nack_nt);
                            put_ul (bfr_out, 3, pbb.new_packet.pn);
                            put_ul (bfr_out, 7, 0);
                            for (j = 0; j < pbb.new_packet.pl_size; j++)
                                bfr_out[j + ARQ_HDR_SIZE] = pbb.new_packet.data[j];
                            if ((bytes_out = sendto(s, bfr_out, pbb.new_packet.pl_size + ARQ_HDR_SIZE, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0)
                                fprintf(log_file, " Error %d: can't send message %d to my child.\n", errno, IDM_UDP_ARQ_VIDEO_R);
                            else
                            {
                                fprintf (log_file," sent\n");
                                printf ("R");
                            }
                        }
                    }
                    else
                        nack_dwbl++;
                    nack_pn += mtratio;
                    nack_bl--;
                }//while (nack_bl>0)
                if (nack_dwbl > 0)
                {
                    put_uc (bfr_out, 0, IDM_UDP_ARQ_DNWM_G);
                    put_uc (bfr_out, 1, nack_nt);
                    put_ul (bfr_out, 2, nack_fpn);
                    put_uc (bfr_out, 6, nack_dwbl);
                    if ((bytes_out = sendto(s, bfr_out, 7, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0)
                        fprintf(log_file,"Error %d: can't send message %d to child %d.", errno, IDM_UDP_ARQ_DNWM_G,nack_nt);
                    else
                        fprintf(log_file,"SENT DNWM pn=%lu fpn=%lu dwbl=%d\n",nack_pn, nack_fpn,nack_dwbl);
                }
            }//for (nack.bc)
            unsigned char nack_size = nack_bc * 5 + 4;
            for (i = nack_size; i < bytes_in; i++)
                bfr_in[i - nack_size] = bfr_in[i];
            bytes_in -= nack_size;
        }
        else if (bytes_in >= VIDEO_CHUNCK_SIZE)
        {
            //packets with video
            if (isStarted == false)
            {
                if (starttime == 0)
                {
                    struct timespec tp;
                    clock_gettime(CLOCK_MONOTONIC, &tp);
                    starttime = tp.tv_nsec;
                }
                isStarted = true;
                printf (" Video content\n");
            }
            put_uc (bfr_out, 0, IDM_UDP_ARQ_VIDEO);
            put_uc (bfr_out, 1, 0);
            tree_number++;
            if (tree_number == mtratio) tree_number = 0;
            put_uc (bfr_out, 2, tree_number);
            if (gal_pn == MAX_PN) gal_pn = 0; else gal_pn++; //new global packet number (ulong)
            put_ul (bfr_out, 3, gal_pn);
            if (p2p_pn[tree_number] == MAX_PN) p2p_pn[tree_number]=0; else p2p_pn[tree_number]++; //new p2p packet numbers for packets in each arq line
            put_ul (bfr_out, 7, p2p_pn[tree_number]);
            if (isVqpEnabled)
            {
                unsigned char pic_type = define_p_type_for_mpeg4( &bfr_in[RTP_HDR_SIZE], bytes_in - RTP_HDR_SIZE, gal_pn, mpeg_file);
                switch (pic_type)
                {
                    case 0:
                        pred_n = 0x3F;
                        if (gop_n >= MAX_GOP_N) gop_n = 0; else gop_n++;
                        break;
                    case 1:
                        //predicted frame
                        if (pred_n >= 0x3F) pred_n = 0; else pred_n++;
                        break;
                    case 0xFF:
                        //same frame
                        pic_type = prev_pic_type;
                        break;
                }
                if (pic_type <= prev_pic_type)
                {
                    prev_pic_type = pic_type;
                }
                else
                {
                    unsigned char tmp = prev_pic_type;
                    prev_pic_type = pic_type;
                    pic_type = tmp;
                }
                pbb.new_packet.p_t.frame = pic_type;
                pbb.new_packet.p_t.pred_num = pred_n;
                pbb.new_packet.gop_num = gop_n;
                pic_type<<=6;
                pic_type += pred_n;
                put_uc(bfr_out, 11, pic_type);
                put_uc(bfr_out, 12, gop_n);
            }
            else
            {
               put_uc(bfr_out, 11, 0);
               put_uc(bfr_out, 12, 0);
            }
            pbb.new_packet.nr = 0;// a number of repeat
            pbb.new_packet.nt = tree_number; //tree number of current packet for this peer
//            pbb.new_packet.rtp_n = get_us (bfr_in, 2);//rtp number of current packet
            pbb.new_packet.p2p_pn = p2p_pn[tree_number];
            pbb.new_packet.pl_size = bytes_in; //payload size
            pbb.new_packet.pn = gal_pn;// global packet number (unsigned long)
            pbb.new_packet.next = NULL; //a pointer to next packet in playback buffer
            for (i = 0; i < pbb.new_packet.pl_size; i++)
                pbb.new_packet.data[i] = bfr_in [i];
            pbb.shift_buffer(); //an attempt to shift pbb
            //new peer-to-peer packet number (ulong)
            for (i = 0; i < pbb.new_packet.pl_size; i++)
                bfr_out[i + ARQ_HDR_SIZE] = bfr_in[i];
            fprintf(log_file,"nt=%d sn=%lu send_p2psn=%lu",pbb.new_packet.nt,pbb.new_packet.pn,pbb.new_packet.p2p_pn);
            if (pbb.new_packet.p_t.frame==1)
              fprintf(log_file," frame=%d:%d",pbb.new_packet.p_t.frame,pbb.new_packet.p_t.pred_num);
            else
              fprintf(log_file," frame=%d",pbb.new_packet.p_t.frame);
            fprintf(log_file," GOP_N=%d",pbb.new_packet.gop_num);
            for (i = 0; i <  mtratio; i++)
            {
                if (root->child[i] != NULL)
                {
                    if (g.getState())
                    {
                        bytes_out = sendto(s, bfr_out, bytes_in + ARQ_HDR_SIZE, 0, (struct sockaddr *) &root->child[i]->addr, sizeof(struct sockaddr));
                        if (bytes_out)
                            printf (".");
                    }
                    else
                        printf ("l");
                    fprintf(log_file," %d",i);
                }
            }
            fprintf(log_file,"\n");
            bytes_in = 0;
        }
        else
        {
            fprintf (log_file,"M%d\n", id);
            bytes_in = 0;
        }
    }
    return EXIT_SUCCESS;
}

int mtrm::attach_peer (Peer *peer)
{
	int i,j,k,
        parent,	//parent peer number
		child;	//child peer number
	Peer *cur;

	total_ap++;
    peer->n = total_ap;
    printf ("[New%d,%d]", total_ap, peer->n);
    fprintf (log_file, "Peer %d (as %d peer) is connected!\n", total_ap, peer->n);
    peer->m = (peer->n - 1) % mtratio;

	for (i = 0; i < mtratio; i++)
    	peer->child[i] = NULL;
    //does current peer belong to first "mtratio" peers?
    if (peer->n <= mtratio) {
		if (root->child[peer->m] != NULL) {
			for (i = 0; i < mtratio; i++)
				peer->child[i] = root->child[peer->m]->child[i];
			delete (root->child[peer->m]);
		}
		root->child[peer->m] = peer;
		peer->parent[peer->m] = (Peer *)root;
    }
    else {
		for (k = 0; k < mtratio; k++) {
			parent = k - peer->m;
			if (parent < 0) parent += mtratio;
			parent = peer->n - peer->m - mtratio + parent;
			//parent = peer->n - mtratio + k;
			//if (parent >= (peer->n - peer->m)) parent -= mtratio;
			for (j = 0; j < mtratio; j++) {
				cur = root->child[j];
				while (cur != NULL) {
					if (cur->n == parent) {
						peer->parent[k] = cur;
					}
					cur = cur->child[k];
				}
			}
        }
    }
    if (peer->n < total_ap) {
		for (k = 0; k < mtratio; k++) {
			child = k - peer->m;
			if (child < 0) child += mtratio;
			child = peer->n - peer->m + mtratio + child;
			if (child >= (peer->n - peer->m + 2*mtratio)) child -= mtratio;
			for (j = 0; j < mtratio; j++) {
				cur = root->child[j];
				while (cur != NULL) {
					if (cur->n == child) {
						peer->child[k] = cur;
						cur->parent[k] = peer;
					}
					cur = cur->child[k];
				}
			}
        }
    }
    if (peer->n > mtratio)
    	for (i = 0; i < mtratio; i++)
			peer->parent[i]->child[i] = peer;
    return EXIT_SUCCESS;
}

int mtrm::delete_peer (Peer * peer)
{
	if (peer->n > mtratio)
	{
		for (int i = 0; i < mtratio; i++)
        {
			peer->parent[i]->child[i] = peer->child[i];
			if (peer->child[i] != NULL)
				peer->child[i]->parent[i] = peer->parent[i];
		}
        printf ("[Del%d]", peer->n);
        fprintf (log_file,"Peer %d is deleted.\n", peer->n);
		delete (peer);
	}
	else
    {
        put_uc (bfr_out, 0, IDM_TCP_DELETED);
        printf ("[Del%d]", peer->n);
        fprintf (log_file,"Peer %d is deleted.\n", peer->n);
		root->child[peer->m]->to_delete = 0;
	}
	return EXIT_SUCCESS;
}

int mtrm::delete_tree ()
{
	Peer *cur, *del;
    for (int j = 0; j < mtratio; j++) {
		cur = root->child[j];
		while (cur != NULL) {
			put_uc (bfr_out, 0, IDM_TCP_DELETED);
            printf ("[Del%d]", cur->n);
            fprintf (log_file,"Peer %d is deleted.\n", cur->n);
        	del = cur;
        	cur = del->child[0];
			delete(del);
		}
	}
	return EXIT_SUCCESS;
}
float mtrm::count_average_plr ()
{
	float plr_sum = 0,
		  plr_avr;
	Peer * cur;
	int j,
		max = 0;
	for (j = 0; j < mtratio; j++) {
		cur = root->child[j];
		while (cur != NULL) {
            if (cur->plr != PLR_INIT_VALUE)
            {
                max++;
                plr_sum += cur->plr;
            }
			cur = cur->child[0];
		}
	}
    if (max > 0) {
		plr_avr = plr_sum / max;
		return plr_avr;
    }
    else
    	return EXIT_FAILURE;
}

int mtrm::write_plr_to_file ()
{
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
//    fprintf (avr_plr_file, "%s;%d;%f\n", ts.tv_sec, count_average_plr());
    return EXIT_SUCCESS;
}

int mtrm::put_child (Peer *peer, unsigned char i)
{
    int n = 0;
	put_uc (bfr_out, n, IDM_TCP_CHILD);
	put_uc (bfr_out, n + 1, mtratio);
	put_us (bfr_out, n + 2, peer->n);
	put_uc (bfr_out, n + 4, i);
	if (peer->child[i] == NULL) {
		put_uc (bfr_out, n + 5, IDM_TCP_CHILDREN_N);
		return 6;
	}
	else {
		put_uc (bfr_out, n + 5, IDM_TCP_CHILDREN_Y);
		put_us (bfr_out, n + 6, peer->child[i]->n);
		put_us (bfr_out, n + 8, peer->child[i]->addr.sin_port);
		put_ul (bfr_out, n + 10, peer->child[i]->addr.sin_addr.s_addr);
		return 14;
	}
}
int mtrm::put_children (Peer *peer)
{
	int n = 0;
	put_uc (bfr_out, n, IDM_TCP_CHILDREN);
	put_uc (bfr_out, n + 1, mtratio);
	put_us (bfr_out, n + 2, peer->n);
	int i;
	for (i = 0; i < mtratio; i++) {
		if (peer->child[i] == NULL) {
			put_uc (bfr_out, n + i*9 + 4, IDM_TCP_CHILDREN_N);
			put_us (bfr_out, n + i*9 + 5, 0);
			put_us (bfr_out, n + i*9 + 7, 0);
			put_ul (bfr_out, n + i*9 + 9, 0);
		}
		else {
			put_uc (bfr_out, n + i*9 + 4, IDM_TCP_CHILDREN_Y);
			put_us (bfr_out, n + i*9 + 5, peer->child[i]->n);
			put_us (bfr_out, n + i*9 + 7, peer->child[i]->addr.sin_port);
			put_ul (bfr_out, n + i*9 + 9, peer->child[i]->addr.sin_addr.s_addr);
		}
	}
	return (4 + 9*mtratio);
}

