#include "mtpm.h"
#include <string>

mtpm::mtpm()
{
    //ctor
    timeout = 1.0;
    isArqEnabled = true;
}

mtpm::~mtpm()
{
    //dtor
   stop_mtpm();
}
char* mtpm::getCmdOption(char ** begin, char ** end, const std::string & option)
{
    //find "-option" and get next arg
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool mtpm::cmdOptionExists(char** begin, char** end, const std::string& option)
{
    //check "-option" existance
    return std::find(begin, end, option) != end;
}
int mtpm::init_mtpm(int argc, char *argv[])
{
    int i;
	unsigned long ip_server = inet_addr ("127.0.0.1"); //source ip
    unsigned long ip_peer = inet_addr ("127.0.0.1"); //my peer ip
    unsigned short port_server = htons(5010); //source port
    unsigned short port_peer = htons(5018); //my peer port
    unsigned short port_vlc = htons(5019); //port number where you can get video stream with help of VLC (rtp://ip:port , where port = port_vlc)
    double lb = 0; //bundle length
    double ploss = 0; //packet loss
    unsigned long tm = 1000; //calculation interval for plr (tm = lost + recieved -> plr = lost/tm)
    int max_pbbfr_size = MAX_PBBFR_SIZE; //max playback buffer size
    unsigned long max_pbbfr_time = MAX_PBBFR_TIME; //max playback buffer delay
    mtratio = MTR; //multiple tree ratio (number of children/parents)
    path = "Res"; //a path for statistics
    //argv parsing
    char *option = NULL;
    option = getCmdOption(argv, argv + argc, "-ip_s");
    if (option)
        ip_server = inet_addr(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-port_s");
    if (option)
        port_server = htons((unsigned short)atoi(std::string(option).c_str()));
    option = getCmdOption(argv, argv + argc, "-ip");
    if (option)
        ip_peer = inet_addr(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-port");
    if (option)
        port_peer = htons((unsigned short)atoi(std::string(option).c_str()));
    option = getCmdOption(argv, argv + argc, "-port_vlc");
    if (option)
        port_vlc = htons((unsigned short)atoi(std::string(option).c_str()));
    option = getCmdOption(argv, argv + argc, "-ploss");
    if (option)
        ploss = (double)atof(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-lb");
    if (option)
        lb = (double)atof(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-tm");
    if (option)
        tm = atoi(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-btime");
    if (option)
        max_pbbfr_time = (unsigned long) 1000000*atoi(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-bsize");
    if (option)
        max_pbbfr_size = atoi(std::string(option).c_str());
    option = getCmdOption(argv, argv + argc, "-timeout");
    if (option)
        timeout = (double)atof(std::string(option).c_str());
    isArqEnabled = !cmdOptionExists(argv, argv + argc, "-noarq");
    int size_sa = sizeof(struct sockaddr);
    //show input values
    struct sockaddr_in addr_temp;
    memset (&addr_temp, 0, size_sa);
    addr_temp.sin_family = AF_INET;
    addr_temp.sin_addr.s_addr = ip_server;
    printf ("source=%s:%d ",inet_ntoa(addr_temp.sin_addr),ntohs(port_server));
    addr_temp.sin_addr.s_addr = ip_peer;
    printf ("my_peer=%s:%d/%d ploss=%2.4f lb=%2.2f tm=%lu btime=%lu bsize=%d timeout=%4.2f arq=%s\n", inet_ntoa(addr_temp.sin_addr), ntohs(port_peer), ntohs(port_vlc), ploss, lb, tm, max_pbbfr_time/1000000, max_pbbfr_size, timeout, isArqEnabled ? "true" : "false");
    //my peer
    my_peer = new MyPeer;
    my_peer->n = 0;
    memset(&my_peer->addr, 0, size_sa);
    my_peer->addr.sin_addr.s_addr = ip_peer;
    my_peer->addr.sin_port = port_peer;
    my_peer->addr.sin_family = AF_INET;
    for (i = 0; i < mtratio; i++)
	{
		my_peer->child[i] = NULL;
		my_peer->parent[i] = new MyPeer;
		my_peer->parent[i]->ping_n = 0;
		my_peer->parent[i]->ping_t = GetTickCount();
		my_peer->parent[i]->addr.sin_addr.s_addr = 0;
		my_peer->parent[i]->addr.sin_port = 0;
		my_peer->parent[i]->addr.sin_family = AF_INET;
    }
    //the source is one of parent peers
    if (my_peer->parent[0]!= NULL)
    {
        my_peer->parent[0]->addr.sin_addr.s_addr = ip_server;
        my_peer->parent[0]->addr.sin_port = port_server;
    }
    //udp socket
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
	{
    	printf(" Error %d: socket s descriptor is invalid. \n", errno);
        exit(0);
    }
    if (bind(s, (struct sockaddr *)&my_peer->addr, size_sa))
	{
    	printf(" Error %d: socket s address is invalid. \n", errno);
        exit(0);
    }
    //non-blocking mode
    if (fcntl(s, F_SETFL, O_NONBLOCK))
        printf("Error %d: socket s could not be set to nonblocking mode.\n",errno);
    //video port
    memset(&addr_vlc, 0, size_sa);
    addr_vlc.sin_addr.s_addr = ip_peer;
    addr_vlc.sin_family = AF_INET;
    addr_vlc.sin_port = port_vlc;
    //source address
    struct sockaddr_in addr_server;
    memset (&addr_server, 0, size_sa);
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = ip_server;
    addr_server.sin_port = port_server;
    //buffers
    bfr_in = new char[MTU_SIZE]; //recv buffer
    bfr_out = new char[MTU_SIZE]; //send buffer
    //stats
    //a number of successful programm launchings is used for file indexing
    FILE *init_file;
    //try to open file and check its existance
    init_file = fopen ("N.txt","a+");
    fclose(init_file);
    //open for reading
    init_file = fopen("N.txt","r+");
    file_n=-1;
    if (!feof(init_file))
        fscanf(init_file, "%d", &file_n);
    file_n++;
    //write inc number back into the file
    rewind(init_file);
    fprintf(init_file, "%d", file_n);
    fclose(init_file);
    std::string path_temp;
    //make "Rec" directory
    mkdir(path.c_str(), 0777);
    char s_temp[10];
    snprintf (s_temp, sizeof (s_temp), "%d", file_n);
    //log file
    path_temp = path + "//test_" + std::string(s_temp) + "_log.txt";
    log_file = fopen(path_temp.c_str(), "w");
    char str_val [32];
    snprintf (str_val, sizeof (str_val), "%d", ntohs(my_peer->addr.sin_port)) ;
    std::string log_file_string = std::string("Peer ") + inet_ntoa(my_peer->addr.sin_addr) + ":" + std::string(str_val)+"\n";
    fprintf(log_file, "%s\n",log_file_string.c_str());
    //plr counter file
    path_temp = path + "//test_" + std::string(s_temp) + "_plr_corr.csv";
    plr_f_corr = fopen(path_temp.c_str(),"w");
    path_temp = path + "//test_" + std::string(s_temp) + "_plr_pure.csv";
    plr_f_pure = fopen(path_temp.c_str(),"w");
    path_temp = path + "//test_" + std::string(s_temp) + "_plr_corr2.csv";
    plr_f_corr2 = fopen(path_temp.c_str(),"w");
    path_temp = path + "//test_" + std::string(s_temp) + "_plr_pure2.csv";
    plr_f_pure2 = fopen(path_temp.c_str(),"w");
    path_temp = path + "//test_" + std::string(s_temp) + "_plr_corr3.csv";
    plr_f_corr3 = fopen(path_temp.c_str(),"w");
    path_temp = path + "//test_" + std::string(s_temp) + "_plr_pure3.csv";
    plr_f_pure3 = fopen(path_temp.c_str(),"w");
    //artificial packet loss
	for (i = 0; i < MTR; i++)
    {
        p2p_pn[i] = 0;
        g[i].initGec(ploss, lb);
    }
    //playback buffer
    pbb.max_time = max_pbbfr_time;
    pbb.max_size = max_pbbfr_size;
    //Control channel
    ctrl_c.init_c(ploss);
    //PLR counter
    for(unsigned int i = 0; i <sizeof(listPLR_corr)/sizeof(listPLR_corr[0]); i = i + 1 )
      {
        listPLR_corr[i].tm = tm;
        listPLR_pure[i].tm = tm;

      }

//    plr_c_corr.tm = tm;
//    plr_c_pure.tm = tm;
    //ARQ Lines
    for (i = 0; i < MTR; ++i)
    {
        if (!isArqEnabled)
        {
            al[i].isActive=false;
            al2[i].isActive=false;
            al3[i].isActive=false;
        }

        al[i].nt = i; //tree number for each arq line
        al2[i].nt = i;
        al3[i].nt = i;
    }
    printf("Waiting for video... \n");
    return EXIT_SUCCESS;
}

int mtpm::check_pbb()
{
    unsigned long sn; //temporary sequantial number
    unsigned char nr; //temporary packet repeat number
    std::string id;
    if (pbb.play(s,addr_vlc,sn, nr, id))
    {
        //printf("id=%s",id.c_str());
        if(id == "192.168.1.3")
          {
            //printf("id=%s",id.c_str());
            //printf("sn=%lu",sn);
            //calculate corrected plr if playback buffer was shifted
            fprintf(log_file,"plr_c_corr sn=%lu nr=%d\n",sn,nr);
            listPLR_corr[0].cur = sn; //input for plr counter
            listPLR_corr[0].check (log_file);

            if (listPLR_corr[0].calculate() == EXIT_SUCCESS)
                listPLR_corr[0].write_plr_to_file(plr_f_corr);
            //calculate pure plr if current outcoming packet isn't recovered one
            if (nr == 0)
            {
                fprintf(log_file,"plr_c_pure sn=%lu nr=%d\n",sn,nr);
                listPLR_pure[0].cur = sn; //input for plr counter
                listPLR_pure[0].check(log_file);
                if (listPLR_pure[0].calculate() == EXIT_SUCCESS)
                    listPLR_pure[0].write_plr_to_file(plr_f_pure);
            }
          }
        if(id == "192.168.1.6")
          {
            //calculate corrected plr if playback buffer was shifted
            fprintf(log_file,"plr_c_corr sn=%lu nr=%d\n",sn,nr);
            listPLR_corr[1].cur = sn; //input for plr counter
            listPLR_corr[1].check (log_file);

            if (listPLR_corr[1].calculate() == EXIT_SUCCESS)
                listPLR_corr[1].write_plr_to_file(plr_f_corr2);
            //calculate pure plr if current outcoming packet isn't recovered one
            if (nr == 0)
            {
                fprintf(log_file,"plr_c_pure sn=%lu nr=%d\n",sn,nr);
                listPLR_pure[1].cur = sn; //input for plr counter
                listPLR_pure[1].check(log_file);
                if (listPLR_pure[1].calculate() == EXIT_SUCCESS)
                    listPLR_pure[1].write_plr_to_file(plr_f_pure2);
            }
          }
        if(id == "192.168.1.20")
          {
            //calculate corrected plr if playback buffer was shifted
            fprintf(log_file,"plr_c_corr sn=%lu nr=%d\n",sn,nr);
            listPLR_corr[2].cur = sn; //input for plr counter
            listPLR_corr[2].check (log_file);

            if (listPLR_corr[2].calculate() == EXIT_SUCCESS)
                listPLR_corr[2].write_plr_to_file(plr_f_corr3);
            //calculate pure plr if current outcoming packet isn't recovered one
            if (nr == 0)
            {
                fprintf(log_file,"plr_c_pure sn=%lu nr=%d\n",sn,nr);
                listPLR_pure[2].cur = sn; //input for plr counter
                listPLR_pure[2].check(log_file);
                if (listPLR_pure[2].calculate() == EXIT_SUCCESS)
                    listPLR_pure[2].write_plr_to_file(plr_f_pure3);
            }
          }
/*        //calculate corrected plr if playback buffer was shifted
        fprintf(log_file,"plr_c_corr sn=%lu nr=%d\n",sn,nr);
        plr_c_corr.cur = sn; //input for plr counter
        plr_c_corr.check(log_file);
        if (plr_c_corr.calculate() == EXIT_SUCCESS)
            plr_c_corr.write_plr_to_file(plr_f_corr);
        //calculate pure plr if current outcoming packet isn't recovered one
        if (nr == 0)
        {
            fprintf(log_file,"plr_c_pure sn=%lu nr=%d\n",sn,nr);
            plr_c_pure.cur = sn; //input for plr counter
            plr_c_pure.check(log_file);
            if (plr_c_pure.calculate() == EXIT_SUCCESS)
                plr_c_pure.write_plr_to_file(plr_f_pure);

        }
*/
    }
    else
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


int mtpm::check_udp_socket()
{
    int size_sa = sizeof(struct sockaddr);
    struct sockaddr_in addr;
    memset(&addr, 0, size_sa);
    addr.sin_family = AF_INET;
    int bytes_in = recvfrom( s, bfr_in, MTU_SIZE, 0, (struct sockaddr *) &addr, (socklen_t*)&size_sa);

    while(bytes_in>0)
    {
        parse_udp_message(bytes_in,addr);
        bytes_in=0;

    }
    return EXIT_SUCCESS;
}

int mtpm::parse_udp_message(int bytes_in, struct sockaddr_in addr)
{
    int i,bytes_out;
    int size_sa = sizeof(struct sockaddr);
    while (bytes_in > 0)
    {
        unsigned char id = get_uc (bfr_in, 0); //messages are distinguisged by this id

        if (id == IDM_UDP_ACTIVE)
        {
            //this message contains an answer to a request of activity from one of parent peers
            for (i = 0; i < mtratio; i++)
            {
                if ((my_peer->parent[i]->addr.sin_addr.s_addr == addr.sin_addr.s_addr)
                    && (my_peer->parent[i]->addr.sin_port == addr.sin_port))
                {
                    my_peer->parent[i]->ping_n = 0;
                    my_peer->parent[i]->ping_t = GetTickCount();
                    fprintf (log_file,"M%dp%d\n", id,i);
                }
            }
            for (i = 1; i < bytes_in; i++)
                bfr_in[i - 1] = bfr_in[i];
            bytes_in -= 1;
        }
        else if (id == IDM_UDP_PING)
        {
            fprintf (log_file,"Ping\n");
            //this message is a request of activity of this peer
            //to send ping to child peer
            put_uc (bfr_out, 0, IDM_UDP_ACTIVE);
            if ((bytes_out = sendto( s, bfr_out, 1, 0, (struct sockaddr*) &addr, size_sa)) < 0)
                fprintf(log_file,"Error %d: can't send message %d to child.\n", errno, IDM_UDP_ACTIVE);
            for (i = 1; i < bytes_in; i++)
                bfr_in[i - 1] = bfr_in[i];
            bytes_in -= 1;
        }
        else if (((id == IDM_UDP_ARQ_VIDEO) || (id == IDM_UDP_ARQ_VIDEO_R)) && (bytes_in >= VIDEO_CHUNCK_SIZE))
        {
            printf(".");
            //recieved message is a packet with video content
            //recovered_packet = false;
            //to fill packet structure
            pbb.new_packet.nr = get_uc (bfr_in, 1);// a number of repeat
            pbb.new_packet.nt = get_uc (bfr_in, 2);// tree number
            unsigned char nt = pbb.new_packet.nt;//pbb.new_packet.pl_size % mtratio; //tree number of current packet for this peer
            pbb.new_packet.pn = get_ul (bfr_in, 3);// global packet number (unsigned long)
            pbb.new_packet.next = NULL; //a pointer to next packet in playback buffer
            pbb.new_packet.pl_size = bytes_in;
            for (i = 0; i < pbb.new_packet.pl_size; i++)
                pbb.new_packet.data[i] = bfr_in [ARQ_HDR_SIZE + i];
            pbb.new_packet.p_t.frame = get_ul (bfr_in, 11)>>6;
            pbb.new_packet.p_t.pred_num = get_ul (bfr_in, 11)&0x3F;
            pbb.new_packet.gop_num = get_ul (bfr_in, 12);
            //to send packet with videocontent to "nt" child peer
            if (p2p_pn[nt] == MAX_PACKET_NUMBER) p2p_pn[nt] = 0; else p2p_pn[nt]++; //to increase current p2p pn
            pbb.new_packet.p2p_pn = p2p_pn[nt]; //new p2p packet number
            pbb.new_packet.packet_id = inet_ntoa (addr.sin_addr);
            //printf("%s", pbb.new_packet.packet_id.c_str());
            fprintf(log_file,"id=%d nr=%d nt=%d sn=%lu old_p2p_n=%lu p2p_n=%lu pl_size=%hu",
                    id,pbb.new_packet.nr,pbb.new_packet.nt,pbb.new_packet.pn,
                    get_ul (bfr_in, 7),p2p_pn[nt],pbb.new_packet.pl_size);

            if (pbb.add_packet(&pbb.new_packet)==EXIT_SUCCESS)
            {
/*                if (my_peer->child[nt] != NULL)
                {
                    if (g[nt].getState())
                    {
                        put_uc (bfr_out, 0, IDM_UDP_ARQ_VIDEO);
                        put_uc (bfr_out, 1, 0);
                        put_uc (bfr_out, 2, nt);
                        put_ul (bfr_out, 3, pbb.new_packet.pn);
                        put_ul (bfr_out, 7, p2p_pn[nt]); //new p2p pn
                        for (i = 0; i < pbb.new_packet.pl_size; i++)
                            bfr_out[i + ARQ_HDR_SIZE] = pbb.new_packet.data[i];
                        if ((bytes_out = sendto(s, bfr_out, pbb.new_packet.pl_size + ARQ_HDR_SIZE, 0, (struct sockaddr *) &my_peer->child[nt]->addr, sizeof(struct sockaddr))) < 0)
                            fprintf(log_file," Error %d: can't send message %d to child %d.\n", errno, IDM_UDP_ARQ_VIDEO,nt);
                    }
                    else
                    {
                        fprintf(log_file," gec");
                    }
                }//if (my_peer->child[nt] != NULL...)
                else

                  {
*/                    fprintf(log_file," null child");
//                }
            }
            fprintf(log_file,"\n");
/*            if (id == IDM_UDP_ARQ_VIDEO)
            {
                if ((al[nt].isActive) && (!al[nt].isStarted))
                {
                    al[nt].first_in_transmission = pbb.new_packet.p2p_pn;
                    al[nt].isStarted = true;
                }
                if ((my_peer->parent[nt]->addr.sin_addr.s_addr != addr.sin_addr.s_addr)
                    || (my_peer->parent[nt]->addr.sin_port != addr.sin_port))
                {
                    //to define parent peer from what this packet has come
                    //to memorize addr of current parent peer
                    my_peer->parent[nt]->addr.sin_addr.s_addr = addr.sin_addr.s_addr;
                    my_peer->parent[nt]->addr.sin_port = addr.sin_port;
                    fprintf (log_file,"[%d] One of parents changed its ip or port to %s:%hu.\n",nt,std::string(inet_ntoa(addr.sin_addr)).c_str(),ntohs(addr.sin_port));
                    printf (" [%d]",nt);
                }
                my_peer->parent[nt]->ping_n = 0;//a number of activity requests (if ping_n=0 then OK)
                my_peer->parent[nt]->ping_t = GetTickCount();
            }
 */
            if(pbb.new_packet.packet_id == "192.168.1.3")
            {
                if ((al[nt].isActive) && (id == IDM_UDP_ARQ_VIDEO))
                {
                    //arq lines for packets with "nt" from 0 to mtratio-1
                    al[nt].cur = get_ul (bfr_in, 7); //old p2p packet number
                    if (al[nt].is_it_first_packet(pbb.new_packet.nr) == EXIT_FAILURE)
                        al[nt].check();
                    for (i=0;i<mtratio;i++)
                        if (al[i].isActive)
                            al[i].send_nack(s, addr, bfr_out,log_file,&ctrl_c);
                }
            }
            if(pbb.new_packet.packet_id == "192.168.1.6")
            {
                if ((al2[nt].isActive) && (id == IDM_UDP_ARQ_VIDEO))
                {
                    //arq lines for packets with "nt" from 0 to mtratio-1
                    al2[nt].cur = get_ul (bfr_in, 7); //old p2p packet number
                    if (al2[nt].is_it_first_packet(pbb.new_packet.nr) == EXIT_FAILURE)
                        al2[nt].check();
                    for (i=0;i<mtratio;i++)
                        if (al2[i].isActive)
                            al2[i].send_nack(s, addr, bfr_out,log_file,&ctrl_c);
                }
            }
            if(pbb.new_packet.packet_id == "192.168.1.20")
            {
                if ((al3[nt].isActive) && (id == IDM_UDP_ARQ_VIDEO))
                {
                    //arq lines for packets with "nt" from 0 to mtratio-1
                    al3[nt].cur = get_ul (bfr_in, 7); //old p2p packet number
                    if (al3[nt].is_it_first_packet(pbb.new_packet.nr) == EXIT_FAILURE)
                        al3[nt].check();
                    for (i=0;i<mtratio;i++)
                        if (al3[i].isActive)
                            al3[i].send_nack(s, addr, bfr_out,log_file,&ctrl_c);
                }
            }
            timespec ts;

            if(pbb.new_packet.packet_id == "192.168.1.3")
            {
                if (!listPLR_corr[0].isStarted)
                {

                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_corr[0].starttime = ts.tv_sec;
                    listPLR_corr[0].isStarted = true;
                }
                if (!listPLR_pure[0].isStarted)
                {
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_pure[0].starttime = ts.tv_sec;
                    listPLR_pure[0].isStarted = true;
                }

            }
            if(pbb.new_packet.packet_id == "192.168.1.6")
            {
                if (!listPLR_corr[1].isStarted)
                {

                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_corr[1].starttime = ts.tv_sec;
                    listPLR_corr[1].isStarted = true;
                }
                if (!listPLR_pure[1].isStarted)
                {
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_pure[1].starttime = ts.tv_sec;
                    listPLR_pure[1].isStarted = true;
                }

            }
            if(pbb.new_packet.packet_id == "192.168.1.20")
            {
                if (!listPLR_corr[2].isStarted)
                {

                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_corr[2].starttime = ts.tv_sec;
                    listPLR_corr[2].isStarted = true;
                }
                if (!listPLR_pure[2].isStarted)
                {
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_pure[2].starttime = ts.tv_sec;
                    listPLR_pure[2].isStarted = true;
                }

            }
/*            for(unsigned int a = 0; a <sizeof(listPLR_corr)/sizeof(listPLR_corr[0]); a = a + 1)
              {
                if (!listPLR_corr[a].isStarted)
                {

                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_corr[a].starttime = ts.tv_sec;
                    listPLR_corr[a].isStarted = true;
                }
                if (!listPLR_pure[a].isStarted)
                {
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    listPLR_pure[a].starttime = ts.tv_sec;
                    listPLR_pure[a].isStarted = true;
                }

              }
*/
/*
            if (!plr_c_corr.isStarted)
            {

                clock_gettime(CLOCK_MONOTONIC, &ts);
                plr_c_corr.starttime = ts.tv_sec;
                plr_c_corr.isStarted = true;
            }
            if (!plr_c_pure.isStarted)
            {
                clock_gettime(CLOCK_MONOTONIC, &ts);
                plr_c_pure.starttime = ts.tv_sec;
                plr_c_pure.isStarted = true;
            }
*/
            bytes_in = 0;
        }// id == IDM_UDP_ARQ_VIDEO
        else
        {
            //unknown message from one of peers
            bytes_in = 0;
        }
    }//if (bytes_in > 0)
    return EXIT_SUCCESS;
}

int mtpm::stop_mtpm()
{
    int i;
    close(s);
	//check statistics if we have current file number
	if (file_n)
	{
	    FILE *st_file;
        char s_temp[10];
        snprintf (s_temp, sizeof (s_temp), "%d", file_n) ;
        std::string path_temp;
        //lost packet bursts for plr counter with packet recovery mechanism
        path_temp = path + "//test_" + std::string(s_temp) + "_stat_corr.csv";
        st_file = fopen(path_temp.c_str(),"a+");
        fclose(st_file);
        st_file = fopen(path_temp.c_str(),"w+");
        plr_c_corr.write_stat_to_file(st_file);
        fclose(st_file);
        //lost packet bursts for pure plr counter without any correction technique
        path_temp = path + "//test_" + std::string(s_temp) + "_stat_pure.csv";
        st_file = fopen(path_temp.c_str(),"a+");
        fclose(st_file);
        st_file = fopen(path_temp.c_str(),"w+");
        plr_c_pure.write_stat_to_file(st_file);
        fclose(st_file);
        if (log_file)
            fclose(log_file);
        if (plr_f_corr)
            fclose(plr_f_corr);
        if (plr_f_pure)
            fclose(plr_f_pure);
	}
	//clear "my_peer" structure
	if (my_peer)
	{
        for (i = 0; i < MTR; i++) {
            if (my_peer->child[i] != NULL)
                delete(my_peer->child[i]);
            if (my_peer->parent[i] != NULL)
                delete(my_peer->parent[i]);
        }
        delete (my_peer);
	}
    return EXIT_SUCCESS;
}
