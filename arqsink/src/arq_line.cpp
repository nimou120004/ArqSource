#include "socket_io.h"
#include "arq_line.h"

arq_line::~arq_line()
{
    //dtor
    clear_wg();
}
arq_line::arq_line()
{
    //ctor
    isActive = true;
    isStarted = false;
    aowg = 0;
    cur = 0;
    prev = 0;
    first_in_transmission = 0;
    for (int i = 0; i < MAX_BURST_LENGTH_AL; ++i)
        stat[i] = 0;
    wg = NULL;
    doNotDrop = true;
    ct = 0;
    rec_ct = 0;
    ct_isk = 0;
    max_pn = MAX_PACKET_NUMBER;
    tr = 100;
    total_waiting_time = MAX_TOTAL_WAITING_TIME_AL;
    min_tr = 50;
    srtt = 0;
    sdev = 25;
}
int arq_line::reset()
{
    clear_wg();
    isStarted = false;
    isActive = true;
    cur = 0;
    prev = 0;
    first_in_transmission = 0;
    for (int i = 0; i < MAX_BURST_LENGTH_AL; ++i)
        stat[i] = 0;
    doNotDrop = true;
    ct = 0;
    rec_ct = 0;
    ct_isk = 0;
    max_pn = 0xFFFFFFF;
    tr = 100;
    total_waiting_time = MAX_TOTAL_WAITING_TIME_AL;
    min_tr = 50;
    srtt = 0;
    sdev = 25;
    return EXIT_SUCCESS;
}
int arq_line::is_it_first_packet (unsigned char nr)
{
     int i;
    //to recover lost packets
    if (nr && aowg && (cur == wg[0].b[0].first_sn))
    {
        rec_ct++;
        printf("!");
        if ((!wg[0].recalc_nr) || (wg[0].nr == nr))
        {
            rtt = GetTickCount() - wg[0].t_waiting;
            rtt = (wg[0].nr - nr) * tr + rtt;
            srtt = (7*srtt/8) + (rtt/8);
            dev = abs(srtt - rtt);
            sdev = (3*sdev/4) + (dev/4);
            tr = srtt + 4*sdev;
            if (tr < min_tr)
                tr = min_tr;
            wg[0].recalc_nr = true;
        } //endif
        wg[0].b[0].length--;
        if (wg[0].b[0].length == 0)
        {
            wg[0].amount_of_bursts--;
            if (wg[0].amount_of_bursts <= 0)
            {
                free(wg[0].b);
                wg[0].b = NULL;
                aowg--;
                if (aowg == 0)
                {
                    free(wg);
                    wg = NULL;
                } //endif
                else
                {
                    for (i = 0; i < aowg; i++)
                    {
                        wg[i] = wg[i+1];
                        wg[i].b = wg[i+1].b;
                    }
                    wg = (waited_group *)realloc(wg, aowg * sizeof(waited_group));
                } //endelse
            } //endif
            else
            {
                for (i = 0; i < wg[0].amount_of_bursts; i++)
                    wg[0].b[i] = wg[0].b[i+1];
                wg[0].b = (burst *)realloc(wg[0].b, wg[0].amount_of_bursts * sizeof(burst));
            } //endelse
        } //endif
        else
            wg[0].b[0].first_sn++;
        return EXIT_SUCCESS;
    }//endif
    else
        return EXIT_FAILURE;

}
int arq_line::clear_wg ()
{
    printf("\n------waiting groups AL%d------\n",nt);
    if (wg != NULL)
    {
        if (aowg)
        {
            for (int i = aowg-1; i >= 0; i--)
            {
                printf("wg=%d aob=%d",i,wg[i].amount_of_bursts);
                if (wg[i].amount_of_bursts)
                {
                    for (int j = wg[i].amount_of_bursts-1; j >= 0; j--)
                    {
                        printf(" b=%d",j);
                    }
                }
                free(wg[i].b);
                wg[i].b = NULL;
            }
        }
        free(wg);
        printf("\n");
    }
    printf("------------------------------\n");
    wg = NULL;
    aowg = 0;
    return EXIT_SUCCESS;
}
int arq_line::send_nack(int s, struct sockaddr_in addr, char* bfr_out, FILE *file,simple_c *ctrl_c)
{
    int tw;
    int i;
    int bytes_out;
    doNotDrop = true;
    if (aowg)
    {
        for (int wg_n = 0; wg_n < aowg; wg_n++)
        {
            if ((tw = (GetTickCount() - wg[wg_n].t_waiting)) >= tr)
            {
                wg[wg_n].t_waiting = GetTickCount();
                wg[wg_n].tt = wg[wg_n].tt + tr;
                if (wg[wg_n].tt > total_waiting_time)
                {
                    free(wg[wg_n].b);
                    wg[wg_n].b = NULL;
                    aowg--;
                    if (aowg == 0)
                    {
                        free(wg);
                        wg = NULL;
                    }
                    else
                    {
                        for (i = wg_n; i < aowg; i++)
                        {
                            wg[i] = wg[i+1];
                            wg[i].b = wg[i+1].b;
                        }
                        wg = (waited_group *)realloc(wg, aowg * sizeof(waited_group));
                        wg_n--;
                    } //endelse
                }
                else
                {
                    put_uc (bfr_out, 0, IDM_UDP_ARQ_NACK_AL);
                    wg[wg_n].nr++;
                    put_uc (bfr_out, 1, wg[wg_n].nr);
                    put_uc (bfr_out, 2, (unsigned char) wg[wg_n].amount_of_bursts);
                    put_uc (bfr_out, 3, nt);
                    fprintf(file,"NACK%d wg_n=%d nr=%d aob=%d",nt,wg_n,wg[wg_n].nr,wg[wg_n].amount_of_bursts);
                    for (i = 0; i < wg[wg_n].amount_of_bursts; i++)
                    {
                        put_ul (bfr_out, i*5 + 4, wg[wg_n].b[i].first_sn);
                        put_uc (bfr_out, i*5 + 8, wg[wg_n].b[i].length);
                        fprintf(file," %d[%lu:%d]",i,wg[wg_n].b[i].first_sn,wg[wg_n].b[i].length);
                    }
                    if (!ctrl_c->error())
                    {
                        if ((bytes_out = sendto(s, bfr_out, 4 + wg[wg_n].amount_of_bursts * 5 , 0, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0)
                            fprintf(file," Error %d: can't send message %d.\n", errno, IDM_UDP_ARQ_NACK_AL);
                        else
                        {
                            fprintf(file,"\n");
                            printf(" n%d",nt);
                        }
                    }
                    else
                    {
                        printf("l");
                        fprintf(file,"l%d",nt);
                    }

                }   //endelse
            }  //endif
        }
    }//if wait
    if ((cur != prev + 1) && (cur >= first_in_transmission + 10) && (prev != max_pn || cur))
    {
        if (((long)(cur - prev - 1)) > 0)
        {
            if ((cur - prev - 1) < MAX_BURST_LENGTH_AL)
                stat[cur - prev - 1]++;
            if (aowg < 255)
            {
                aowg++;
                wg = (waited_group *)realloc(wg, aowg * sizeof(waited_group));
                wg[aowg - 1].b = NULL;
            }
            wg[aowg - 1].t_waiting = GetTickCount();
            wg[aowg - 1].b = (burst *) realloc(wg[aowg-1].b, sizeof(burst));
            wg[aowg - 1].b[0].first_sn = prev + 1;
            wg[aowg - 1].b[0].length = cur - wg[aowg - 1].b[0].first_sn;
            wg[aowg - 1].amount_of_bursts = 1;
            wg[aowg - 1].tt = 0;
            wg[aowg - 1].nr = 1;
            wg[aowg - 1].recalc_nr = false;
            //to fill NACK message to current parent peer
            put_uc (bfr_out, 0, IDM_UDP_ARQ_NACK_AL); //nack id
            put_uc (bfr_out, 1, wg[aowg - 1].nr );  //request number
            put_uc (bfr_out, 2, wg[aowg - 1].amount_of_bursts); //a number of bursts
            put_uc (bfr_out, 3, nt); //tree number
            put_ul (bfr_out, 4, wg[aowg - 1].b[0].first_sn);
            put_uc (bfr_out, 8, wg[aowg - 1].b[0].length);
            fprintf(file,"NACK%d wg_n=%d [%lu:%d]",nt,aowg - 1,wg[aowg - 1].b[0].first_sn,wg[aowg - 1].b[0].length);
            if (!ctrl_c->error())
            {
                if ((bytes_out = sendto(s, bfr_out, 9, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0)
                    fprintf(file," Error %d: can't send message %d.\n", errno, IDM_UDP_ARQ_NACK_AL);
                else
                {
                    fprintf(file,"\n");
                    printf (" n%d", nt);
                }
            }
            else
            {
                printf("l");
                fprintf(file,"l%d",nt);
            }
            prev = cur;
            //}
        }  // endif
        else if (((prev - cur) >= 134) && (long (prev - cur) < 10000) )
        {
            ct_isk++;
            doNotDrop = false;
        }
        else if (prev > cur)
        {
            ct_isk++;
        }
        else
            prev = cur;
    }//endif (cur!=prev+1...
    else
        prev = cur;
    return EXIT_SUCCESS;
}
int arq_line::check()
{
    //to check packets in waiting groups
    if (aowg)
    {
        for (int i = 0; i < aowg; i++)
            for (int j = 0; j < wg[i].amount_of_bursts; j++)
                if (wg[i].b[j].first_sn == cur)
                {
                    wg[i].b[j].length--;
                    if (wg[i].b[j].length == 0)
                    {
                        wg[i].amount_of_bursts--;
                        if (wg[i].amount_of_bursts <= 0)
                        {
                            free(wg[i].b);
                            wg[i].b = NULL;
                            aowg--;
                            if (aowg)
                            {
                                for (int wg_n = i; wg_n < aowg; wg_n++)
                                {
                                    wg[wg_n] = wg[wg_n+1];
                                    wg[wg_n].b = wg[wg_n+1].b;
                                }
                                wg = (waited_group *)realloc(wg, aowg * sizeof(waited_group));
                                i--;
                            }
                            else
                            {
                                free(wg);
                                wg=NULL;
                            }
                            rec_ct++;
                            cur = prev;
                            return EXIT_SUCCESS;
                        }
                        else
                        {
                            for (int b_n = j; b_n < wg[i].amount_of_bursts; b_n++)
                                wg[i].b[b_n] = wg[i].b[b_n+1];
                            wg[i].b = (burst *) realloc(wg[i].b, wg[i].amount_of_bursts * sizeof(burst));
                            rec_ct++;
                            cur = prev;
                            return EXIT_SUCCESS;
                        }//endelse
                    }//endif
                    else
                    {
                        wg[i].b[j].first_sn++;
                        rec_ct++;
                        cur = prev;
                        return EXIT_SUCCESS;
                    }
                }
                else if (cur == (wg[i].b[j].first_sn + wg[i].b[j].length - 1))
                {
                    wg[i].b[j].length--;
                    rec_ct++;
                    cur = prev;
                    return EXIT_SUCCESS;
                }
                else if ((wg[i].b[j].first_sn < cur) && (cur < (wg[i].b[j].first_sn + wg[i].b[j].length - 1))) //&& ((cur - wg[i].b[j].first_sn) == 0))
                {
                    wg[i].amount_of_bursts++;
                    wg[i].b = (burst *) realloc(wg[i].b, wg[i].amount_of_bursts * sizeof(burst));
                    for (int b_n = wg[i].amount_of_bursts - 1; b_n > j; b_n--)
                        wg[i].b[b_n] = wg[i].b[b_n - 1];
                    wg[i].b[j].length = cur - wg[i].b[j].first_sn;
                    wg[i].b[j+1].first_sn = cur + 1;
                    wg[i].b[j+1].length = wg[i].b[j+1].length - wg[i].b[j].length - 1;
                    rec_ct++;
                    cur = prev;
                    return EXIT_SUCCESS;
                }
    }
    if (doNotDrop)
        return EXIT_SUCCESS;
    else
    {
        printf("D");
        cur = prev;
        return EXIT_FAILURE;
    }
}
int arq_line::do_not_wait(unsigned long sn, unsigned char length)
{
    if (aowg)
    {
        for (int i = 0; i < aowg; i++)
            for (int j = 0; j < wg[i].amount_of_bursts; j++)
                if (wg[i].b[j].first_sn == sn)
                {
                    wg[i].b[j].length = wg[i].b[j].length - length;
                    if (wg[i].b[j].length == 0)
                    {
                        wg[i].amount_of_bursts--;
                        if (wg[i].amount_of_bursts <= 0)
                        {
                            free(wg[i].b);
                            wg[i].b = NULL;
                            aowg--;
                            if (aowg)
                            {
                                for (int wg_n = i; wg_n < aowg; wg_n++)
                                {
                                    wg[wg_n] = wg[wg_n+1];
                                    wg[wg_n].b = wg[wg_n+1].b;
                                }
                                wg = (waited_group *)realloc(wg, aowg * sizeof(waited_group));
                                i--;
                            }
                            else
                            {
                                free(wg);
                                wg=NULL;
                            }
                            return EXIT_SUCCESS;
                        }
                        else
                        {
                            for (int b_n = j; b_n < wg[i].amount_of_bursts; b_n++)
                                wg[i].b[b_n]=wg[i].b[b_n+1];
                            wg[i].b = (burst *) realloc(wg[i].b, wg[i].amount_of_bursts * sizeof(burst));
                        }//endelse
                    }//endif
                    else
                    {
                        wg[i].b[j].first_sn = wg[i].b[j].first_sn + length;
                        return EXIT_SUCCESS;
                    }
                }
                else if ((sn + length - 1) == (wg[i].b[j].first_sn + wg[i].b[j].length - 1))
                {
                    wg[i].b[j].length -= length;
                    return EXIT_SUCCESS;
                }
                else if ((wg[i].b[j].first_sn < (sn + length - 1)) &&
                        ((sn + length - 1) < (wg[i].b[j].first_sn + wg[i].b[j].length - 1)))
                {
                    wg[i].amount_of_bursts++;
                    wg[i].b = (burst *) realloc(wg[i].b, wg[i].amount_of_bursts * sizeof(burst));
                    for (int b_n = wg[i].amount_of_bursts-1; b_n > j; b_n--)
                        wg[i].b[b_n] = wg[i].b[b_n-1];
                    wg[i].b[j].length = (sn - wg[i].b[j].first_sn);
                    wg[i].b[j+1].first_sn = sn + length;
                    wg[i].b[j+1].length = wg[i].b[j+1].length - wg[i].b[j].length - length;
                    return EXIT_SUCCESS;
                }
    }
    return EXIT_FAILURE;
}
