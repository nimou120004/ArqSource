//---------------------------------------------------------------------------
#ifndef MPEG_parserH
#define MPEG_parserH
#include <stdio.h>
extern unsigned char hflags, hflags_old; //7 ��� - picture header, 6 - GOP header, 5 - sequence header

//�������� ������� ��� ��������� RTP
//����������� ���� ����� ��� ������� MPEG4,
//*data - ��������� �� TS ������, data_len - ����� ����� TS �������,
//sn - ���������� ����� ������:
unsigned char define_p_type_for_mpeg4(char *data, short data_len, unsigned long sn, FILE *mpeg_file);
//---------------------------------------------------------------------------
#endif
