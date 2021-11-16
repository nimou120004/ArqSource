//---------------------------------------------------------------------------
#ifndef VQpriorityH
#define VQpriorityH
#include "socket_io.h"

#define MAX_GOP_N          0xFF     //������������ ����� GOP

struct picture_type {

    unsigned char frame:2;     //��� �����
    unsigned char pred_num:6;  //����� ������������� �����

};

struct transmission_table_fields {//������ � ���������������� �������

    unsigned char nr;             //number of request
    unsigned char nt;               //tree number
    unsigned short priority;      //���������
    unsigned long sn;             //���������� �����
    transmission_table_fields *next;  //��������� ������
    struct sockaddr_in addr;             //child's address

};

class transmission_table //���������������� �������
{
    private:
        float k1;                   //����������� ���� �����
        unsigned short k2;          //����������� ������� �� ������� �����
        unsigned char cost_for_i;   //�������� �������� �����
        unsigned char cost_for_b;   //�������� ���������������� �����
        unsigned char *cost_for_p;  //�������� ������������� ������
        unsigned char pf_number;    //���������� ������������� ������ � GOP
        unsigned short critical_time_to_deadline; //������������ ����� �� ������� �����
        unsigned short table_length; //����� �������

    public:
        bool isEmpty;//���� ����, ��� ������ ������� ��� ������������ ����
        transmission_table_fields *h1,*h2; //��������� �� ������ � ����� �������

    public:
        transmission_table();
        virtual ~transmission_table();
        //���������� � ���������������� ������� ������ � ���������� ������� sn �
        //����������� priority:
        //void add_to_transmission_table(unsigned short priority, unsigned long sn);
        void add_to_transmission_table(unsigned short priority, unsigned long sn, struct sockaddr_in addr, unsigned char nr, unsigned char nt);
        //��������� ������ ������ � ��������� �����������,
        //emply - ����, ����������� �������� �������� ���� ������� �����:
        //unsigned long get_sn_to_send(bool *empty);
        unsigned long get_sn_to_send(struct sockaddr_in *addr, unsigned char *nr, unsigned char *nt);
        //������������� ��� ���������� ����������, gop_size - ������ GOP,
        //bf - ���������� ��������������� ������, ����������� ����� ������� � ������ �������������:
        void init_priority_calculation(unsigned char gop_size,unsigned char bf_number);
        //���������� ���������� ������ � ������ ���� p_t �
        //�� �������� �� ������� ����� time_to_dealine:
        unsigned short calcute_priority(picture_type p_t, unsigned short time_to_deadline);
        //����� ���������������� �������:
        void print(FILE *txt);
        //������������ ������, ������� ��������:
        void destroy();
};
//---------------------------------------------------------------------------
#endif
