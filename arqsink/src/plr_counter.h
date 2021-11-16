#ifndef PLR_H
#define PLR_H

#define MAX_BURST_LENGTH_PLRC 1000
#define TM_PLRC               1000

class plr_counter
{

    public:
        unsigned long cur,
                      prev,
                      tm, //a number of packets in one measure
                      starttime,
                      max_pn;
        double plr;
        bool isStarted;
        short stat[MAX_BURST_LENGTH_PLRC];
    private:
        unsigned long ct,
                      loss_ct,
                      loss_ct1,
                      ct_isk,
                      timer;
        bool justStarted; //don't count plr for first measure
        unsigned short measure_number;

    public:
        plr_counter();
        int check(FILE *file);
        int calculate();
        int write_plr_to_file(FILE *file);
        int write_stat_to_file (FILE *file);

};
#endif
