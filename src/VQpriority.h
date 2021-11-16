//---------------------------------------------------------------------------
#ifndef VQpriorityH
#define VQpriorityH
#include "socket_io.h"

#define MAX_GOP_N          0xFF     //максимальный номер GOP

struct picture_type {

    unsigned char frame:2;     //тип кадра
    unsigned char pred_num:6;  //номер предсказаного кадра

};

struct transmission_table_fields {//запись в приоритизирующей таблице

    unsigned char nr;             //number of request
    unsigned char nt;               //tree number
    unsigned short priority;      //приоритет
    unsigned long sn;             //порядковый номер
    transmission_table_fields *next;  //следующая запись
    struct sockaddr_in addr;             //child's address

};

class transmission_table //приоритизирующая таблица
{
    private:
        float k1;                   //коэффициент типа кадра
        unsigned short k2;          //коэффициент времени до мертвой точки
        unsigned char cost_for_i;   //значение опорного кадра
        unsigned char cost_for_b;   //значение двунаправленного кадра
        unsigned char *cost_for_p;  //значения предсказанных кадров
        unsigned char pf_number;    //количество предсказанных кадров в GOP
        unsigned short critical_time_to_deadline; //критичиеское время до мертвой точки
        unsigned short table_length; //длина таблицы

    public:
        bool isEmpty;//флаг того, что список пакетов для ретрансляции пуст
        transmission_table_fields *h1,*h2; //указатели на начало и конец таблицы

    public:
        transmission_table();
        virtual ~transmission_table();
        //добавление в приоритизирующую таблицу пакета с порядковым номером sn и
        //приоритетом priority:
        //void add_to_transmission_table(unsigned short priority, unsigned long sn);
        void add_to_transmission_table(unsigned short priority, unsigned long sn, struct sockaddr_in addr, unsigned char nr, unsigned char nt);
        //получение номера пакета с наивысшим приоритетом,
        //emply - флаг, принимающий истенное значение если таблица пуста:
        //unsigned long get_sn_to_send(bool *empty);
        unsigned long get_sn_to_send(struct sockaddr_in *addr, unsigned char *nr, unsigned char *nt);
        //инициализация для вычисления приоритета, gop_size - размер GOP,
        //bf - количество двунаправленных кадров, вставляемых между опорным и первым предсказанным:
        void init_priority_calculation(unsigned char gop_size,unsigned char bf_number);
        //вычисление приоритета пакета с кадром типа p_t и
        //со временем до мертвой точки time_to_dealine:
        unsigned short calcute_priority(picture_type p_t, unsigned short time_to_deadline);
        //вывод приоритизирующей таблицы:
        void print(FILE *txt);
        //освобождение памяти, занятой таблицей:
        void destroy();
};
//---------------------------------------------------------------------------
#endif
