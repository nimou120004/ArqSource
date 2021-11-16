//---------------------------------------------------------------------------
#include "MPEG_parser.h"
//подавать следует без заголовка RTP
//определение типа кадра для формата MPEG4,
//*data - указатель на TS пакеты, data_len - общая длина TS пакетов,
//sn - порядковый номер пакета
unsigned char define_p_type_for_mpeg4(char *data, short data_len, unsigned long sn, FILE *mpeg_file)
{
    unsigned char p_type = 0xFF;  //тип кадра
    short i, j, k;
    unsigned char   tsp_number = 1, //номер TS пакета
                    n_in_ts = 4;    //номер байта в TS пакете
    for (i = 4; i < data_len - 3; i++)
    {
        //если байты относятся к полезной нагрузке и TS пакет содержит заголовочную информацию
        if ((i%188>=4) && (data[i-n_in_ts+1]&0x40))
        {
            //если распознан заголовок последовательности:
            if (!data[i] && !data[i+1] && data[i+2] == 1 && (unsigned char)data[i+3] == 0xB0)
			{
                hflags = hflags | 0x08;
                fprintf(mpeg_file, "sn:%lu VO sequence header,TS packet number:%d TS header:%x %x %x %x HF:%d\n", sn, tsp_number, data[i-n_in_ts], data[i-n_in_ts+1], data[i-n_in_ts+2], data[i-n_in_ts+3], hflags);
            }
            //если распознан заголовок VOL:
            if (!data[i] && !data[i+1] && data[i+2] == 1 && ((unsigned char)data[i+3]&0xF0) == 0x20)
			{
                hflags = hflags | 0x04;
                fprintf(mpeg_file, "sn:%lu VOL header,TS packet number:%d TS header:%x %x %x %x HF:%x\n", sn, tsp_number, data[i-n_in_ts], data[i-n_in_ts+1], data[i-n_in_ts+2], data[i-n_in_ts+3], hflags);
            }
            //если распознан заголовок группы VOP:
            if (!data[i] && !data[i+1] && data[i+2] == 1 && (unsigned char)data[i+3] == 0xB3)
			{
                hflags = hflags | 0x02;
                fprintf(mpeg_file, "sn:%lu GoVOP header,TS packet number:%d TS header:%x %x %x %x HF:%x\n", sn, tsp_number, data[i-n_in_ts], data[i-n_in_ts+1], data[i-n_in_ts+2], data[i-n_in_ts+3], hflags);
            }
            //если распознан заголовок VOP (кадра):
            if (!data[i] && !data[i+1] && (data[i+2] == 1) && ((unsigned char)data[i+3] == 0xB6))
			{
                hflags = hflags | 0x01;
                fprintf(mpeg_file, "sn:%lu VOP header,TS packet number:%d TS header:%x %x %x %x HF:%x\n", sn, tsp_number, data[i-n_in_ts], data[i-n_in_ts+1], data[i-n_in_ts+2], data[i-n_in_ts+3], hflags);
                fprintf(mpeg_file, "position in TS:%d ", k = i % 188);
                k = i - k;
                for (j = k; j < i; j++) fprintf(mpeg_file, "%x ", (unsigned char) data[j]);
                    fprintf(mpeg_file, "\n");
                p_type = data[i+4]>>6&0x03; //то определяем тип кадра
                fprintf(mpeg_file, "VOP type:%d\n", p_type);
            }
        }//endif байты относятся к полезной нагрузке и TS пакет содержит заголовочную информацию
        n_in_ts++;
        if (n_in_ts == 188) { n_in_ts = 0; tsp_number++;}
    }//endfor
    hflags_old = hflags;
    hflags = 0;
    return p_type;
}
//---------------------------------------------------------------------------
