#include <string>
#include "socket_io.h"
#include "plr_counter.h"

plr_counter::plr_counter ()
{
    cur = 0;
    prev = 0;
    tm = TM_PLRC;
    starttime = 0;
    max_pn = MAX_PACKET_NUMBER;
    isStarted = false;
    plr = 0.0;
    ct = 0;
    loss_ct = 0;
    loss_ct1 = 0;
    ct_isk = 0;
    timer = 0;
    justStarted = true;
    measure_number = 0;
    for (int i = 0; i < MAX_BURST_LENGTH_PLRC + 1; i++)
		stat[i] = 0;
}

int plr_counter::write_plr_to_file (FILE * file)
{
    fprintf (file, "%lu;%f\n", timer, plr);
    return EXIT_SUCCESS;
}
int plr_counter::check(FILE *file)
{
    ct++;
    if ((cur != (prev + 1)) && (!justStarted) && (prev != max_pn) && (cur != 0))
    {
        if ((long)(cur - prev - 1) > 0)
        {
            if ((cur - prev - 1) <= MAX_BURST_LENGTH_PLRC)
               stat[cur - prev - 1]++;
            if ((cur - prev - 1) > 1)
                fprintf(file, " PACKET LOSS SN:%lu-SN:%lu\n", prev + 1, cur - 1);
            else
                fprintf(file, " PACKET LOSS SN:%lu lost\n", prev + 1);
            loss_ct = loss_ct + (cur - prev - 1);
            ct = ct + cur - prev - 1;
            prev = cur;
        }//lost packets > 0
        else if ((cur <= prev) && ((long)(prev - cur) < 10000) )
        {
            ct_isk++;
            fprintf(file, "!!!! PREV SN%lu >= CUR SN:%lu\n", prev, cur);
            ct--;
            cur = prev;
        }
        else
        {
            loss_ct = loss_ct + (max_pn - prev) + cur;
            ct = ct + (max_pn - prev) + cur;
            prev = cur;
            if ((max_pn - prev + cur) > 1)
                fprintf(file, "PACKET LOSS SN:%lu-SN:%lu\n", prev + 1, cur - 1);
            else
                fprintf(file, "PACKET LOSS SN:%lu lost\n", prev + 1);
        }
    }  //endif (cur==prev+1)
    else
        prev = cur;
    return EXIT_SUCCESS;
}
int plr_counter::calculate()
{
    //counting PLR every "tm" packets
    if (ct >= tm)
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        timer = ts.tv_sec - starttime;
        justStarted = false;
        if (ct > tm)
        {
            loss_ct1 = ct - tm;
            ct = tm;
            loss_ct = loss_ct - loss_ct1;
        }
        if ((loss_ct + ct) != 0)
            plr = (float) loss_ct / (float) (ct);
        measure_number++;
        if (loss_ct1 == 0)
        {
            ct = 0;
            loss_ct = 0;
        }
        else
        {
            ct = loss_ct = loss_ct1;
            loss_ct1 = 0;
        }
    }//if (ct >= tm)
    else
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
int plr_counter::write_stat_to_file (FILE *file)
{
	fprintf(file, "%s;%s;%s;%s;%s\n","Loss Birst Size","Amount","Packet Count","Probability I","Probability II");
    long total_lost_packets = 0,
		 total_loss_birsts = 0;
    int i;
    for (i = 1; i < MAX_BURST_LENGTH_PLRC + 1; i++)
    {
		total_loss_birsts += stat[i];
		total_lost_packets += stat[i] * i;
	}
    for (i = 1; i < MAX_BURST_LENGTH_PLRC + 1; i++)
		fprintf(file, "%d;%d;%d;%f;%f\n", i, stat[i], stat[i] * i, (float) stat[i] / total_loss_birsts, (float) stat[i] * i / total_lost_packets);
    return EXIT_SUCCESS;
}

