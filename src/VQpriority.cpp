#include "VQpriority.h"

transmission_table::transmission_table()
{
    critical_time_to_deadline = 100;
    init_priority_calculation(15,2);
    h1 = NULL;
    h2 = NULL;
    table_length = 0;
    isEmpty = true;
}
transmission_table::~transmission_table()
{
    destroy();
    printf ("Transmission table is deleted.\n");
}
//добавление в приоритизирующую таблицу пакета с порядковым номером sn и
//приоритетом priority
void transmission_table::add_to_transmission_table(unsigned short priority, unsigned long sn, struct sockaddr_in addr, unsigned char nr,unsigned char nt)
{
    //int i;
    if ((!table_length) || (h1 == NULL))
    {
        //если таблица пуста,
        table_length++;
        h1 = new transmission_table_fields;
        h1->priority = priority;  //то добавляем в начало
        h1->sn = sn;
        h1->addr = addr;
        h1->nr = nr;
        h1->nt = nt;
        h2 = h1;
        h2->next = NULL;
    }
    else
    {
        //иначе ищем подходящее место
        transmission_table_fields *t = NULL;
        t = h1;
        while (t != NULL)
        {
            //отражен ли уже этот пакет в таблице
            if (t->sn == sn) break;
            t = t->next;
        }
        if (t != NULL)
        {
            //если отражен,
            table_length--;
            transmission_table_fields *s = NULL;
            if ((t->next) != NULL)
            {
                s = t->next;  //то удаляем
                *t = *s;      //старую
                delete s;   //запись
            }
            else
            {
                //если удаляемая запись оказалась
                s = t; //в конце таблице,
                if (table_length)
                {
                    //то если таблица не оказывается пустой,
                    t = h1;
                    while (t->next != s) t = t->next;
                    h2 = t; //укатель на конец устанавлицаем на предыдущий элемент
                    delete s; //удаляем старую запись
                    h2->next = NULL;
                }
                else
                {
                    //если же указывается пустой
                    h2 = NULL;  //то заNULLяем
                    h1 = NULL;  //указатели
                    delete s; //удаляем старую запись
                    printf(" h1=NULL!!!");
                    return;
                }
            }//endelse елемент оказалс последним
        }//endif этот пакет уже есть в таблице
        t = h1;
        if (t==NULL)
        {
            printf(" t=NULL!!!");
            return;
        }
        //поиск подходящей позиции под запись:
        while (priority < t->priority && t->next != NULL) t = t->next;
        while ((priority == t->priority) && (sn > t->sn) && (t->next != NULL)) t = t->next;
        table_length++;
        transmission_table_fields *s;
        s = new transmission_table_fields;
        if (t->next != NULL)
        {
            //вставка перед t:
            *s = *t;
            t->sn = sn;
            t->priority = priority;
            t->addr = addr;
            t->nr = nr;
            t->nt = nt;
            t->next = s;
        }
        else
        {
            //вставка в конец:
            s->sn = sn;
            s->priority = priority;
            s->addr = addr;
            s->nr = nr;
            s->nt = nt;
            t->next = s;
            h2 = s;
            h2->next = NULL;
        }
    }
    return;
}
//получение номера пакета с наивысшим приоритетом,
//emply - флаг, принимающий истенное значение если таблица пуста
unsigned long transmission_table::get_sn_to_send(struct sockaddr_in *addr, unsigned char *nr, unsigned char *nt)
{
    if (table_length)
    {
        //int i;
        unsigned long saved_sn = h1->sn; //узнаем порядковый номер пакета с наивысшим приорететом
        struct sockaddr_in saved_addr = h1->addr;
        unsigned char saved_nr = h1->nr;
        unsigned char saved_nt = h1->nt;
        //и удаляем его из таблицы:
        table_length--;
        if (table_length)
        {
            transmission_table_fields *t;
            t = h1;
            h1 = t->next;
            delete t;
        }
        else
        {
            delete h1;
            h1 = NULL;
            h2 = NULL;
        }
        isEmpty = false;
        *addr = saved_addr;
        *nr = saved_nr;
        *nt = saved_nt;
        return saved_sn;
    }
    else
    {
        isEmpty = true;
        return 0xFFFF;
    }
}
//инициализация для вычисления приоритета, gop_size - размер GOP,
//bf - количество двунаправленных кадров, вставляемых между опорным и первым предсказанным
void transmission_table::init_priority_calculation(unsigned char gop_size,unsigned char bf_number)
{
    k1 = 16.0 / gop_size; //коэффициент типа кадра
    k2 = 20.0 * critical_time_to_deadline; //коэффициент времени до мертвой точки
    cost_for_i = gop_size;
    int i;
    bf_number++;
    pf_number = gop_size / bf_number;
    pf_number--;
    h1 = NULL;
    h2 = NULL;
    table_length = 0;
    cost_for_p = new unsigned char[pf_number];
    gop_size--;
    if (pf_number) cost_for_p[0] = gop_size;
    for (i = 1; i < pf_number; i++) cost_for_p[i] = gop_size - i * bf_number;
    gop_size++;
    bf_number--;
    cost_for_b = 1;
    return;
}
//вычисление приоритета пакета с кадром типа p_t и
//со временем до мертвой точки time_to_dealine
unsigned short transmission_table::calcute_priority(picture_type p_t, unsigned short time_to_deadline)
{
    unsigned char d;
    switch (p_t.frame)
    {
        case 0: d = cost_for_i;  break;
        case 1: d = cost_for_p[p_t.pred_num]; break;
        case 2: d = cost_for_b; break;
    }
    unsigned short pr;
    pr = k1 * d + (float) k2 / time_to_deadline;
    return pr;
}
//вывод приоритизирующей таблицы
void transmission_table::print(FILE *txt)
{
    //int i;
    transmission_table_fields *t = NULL;
    t = h1;
    fprintf(txt,"---------priority table---------\n");
    while (t != NULL)
    {
        fprintf(txt, "%lu:%hu ", t->sn, t->priority);
        t = t->next;
    }
    fprintf(txt,"\n--------------------------------\n");
    return;
}
//освобождение памяти, занятой таблицей
void transmission_table::destroy()
{
    delete [] cost_for_p;
    transmission_table_fields *t=NULL;
    printf("\n---------transmission table---------\n");
    t = h1;
    while (t != NULL)
    {
        printf(" %lu:%hu", t->sn, t->priority);
        t = t->next;
        delete h1;
        h1 = t;
        table_length--;
    }
    printf("\ntable_length=%d\n",table_length);
    printf("\n------------------------------------\n");
    h1 = NULL;
    h2 = NULL;
    return;
}
