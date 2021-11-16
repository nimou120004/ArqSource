//---------------------------------------------------------------------------
#ifndef MPEG_parserH
#define MPEG_parserH
#include <stdio.h>
extern unsigned char hflags, hflags_old; //7 бит - picture header, 6 - GOP header, 5 - sequence header

//подавать следует без заголовка RTP
//определение типа кадра для формата MPEG4,
//*data - указатель на TS пакеты, data_len - общая длина TS пакетов,
//sn - порядковый номер пакета:
unsigned char define_p_type_for_mpeg4(char *data, short data_len, unsigned long sn, FILE *mpeg_file);
//---------------------------------------------------------------------------
#endif
