#include<stdio.h>
//#include<stdlib.h>
#include<string.h>
#include "get_flac_id3.h"
#include "utils/Log.h"
#undef ALOGD
#define ALOGD
#define ALOGI ALOGD

int FlacId3::getFlacTag(FILE *stream)
{
	unsigned char buf[4];
	unsigned long temp;
	char *buffer;
	int isLast;//判断metadata是否结束标志
	int type;//metadata类型，所取信息在04，VORBIS_COMMENT表项中
	int length;
	int cbRead;

	memset(&flactag, 0, sizeof(struct FLACTAG));

	if(stream == NULL)
	{
		return FLAC_ERROR;
	}

	fseek(stream, 0, SEEK_SET);

	cbRead = fread(buf, 1, 4, stream);
	if(cbRead != 4)
	{
		return FLAC_ERROR; // file too short
	}

	if(!(buf[0]==0x66 && buf[1]==0x4c && buf[2]==0x61 && buf[3]==0x43))
	{
		return FLAC_ERROR; // not flac file
	}


	while(1)
	{
		cbRead = fread(&temp, 1, 4, stream);
		if(cbRead != 4)
		{
			return FLAC_ERROR; // file too short
		}

		isLast = (temp&0xFF) >> 7;
		type = temp & 0x7F;
		length = ((temp&0xFF000000)>>24) | ((temp&0xFF0000)>>8) | ((temp&0xFF00)<<8);

		/*从表项四中提取相关信息*/
		if(type == 4)
		{
			buffer=(char *)malloc(length);
			if(buffer == NULL)
			{
				return FLAC_ERROR;
			}
			fread(buffer, length, 1, stream);//先将相关信息读取到buffer中
			handleFlacTag(buffer);//对buffer中的信息进行提取函数

			free(buffer);
			buffer=NULL;

			break;
		}

		if(isLast == 1)
		{
			return FLAC_END; // end of metadata，未找到相关信息
		}

		fseek(stream, length, SEEK_CUR);
	}

	return FLAC_SUCCESS;
}

/*对buffer中的Tag信息进行处理*/
int  FlacId3::handleFlacTag(char *buffer)
{
	int i,j,k;
	int num;
	char s[FLAC_LENGTH];
	char *str;
	char *strDest;

	j=(int)buffer[0] + (((int)buffer[1])<<8) + (((int)buffer[2])<<16) + (((int)buffer[3])<<24);
	i = 4;
	i += j;
	num = buffer[i];//有效信息表项的个数

	i += 4;

	for(; num > 0; num--)
	{
		j = (int)buffer[i] + (((int)buffer[i+1])<<8);//printf("j=%d",j);
		if(j == 0)
			break;

		i += 4;
		k = 0;
		while(buffer[i] != '=') //
		{
			if(buffer[i]<'a')		//将表头信息中大写字母转化为小写字母
				buffer[i]+=32;
			if(k < FLAC_LENGTH-1)//fixed by Charles Chen 09/12/12
				s[k]=buffer[i];//先读取表头信息，遇到等号结束读取
			else
				k = FLAC_LENGTH-2;

			i++;
			k++;
			j--;
		}
		s[k] = '\0';
		j--;
		i++;

		/*将表项中内容缓存入str中*/
		str=(char *)malloc(j+1);
		if(str == NULL)
		{
			return FLAC_ERROR;
		}

		k = 0;
		while(j > 0)
		{
			str[k] = buffer[i];
			i++;
			k++;
			j--;
		}
		str[k]='\0';

		/*将相应的信息存入结构体中*/
			/*save the ID3 info to the struct*/
		 strDest = NULL;
		if(strcmp(s,"title") == 0)
		{
			strDest = flactag.Title;
		}
		else if(strcmp(s,"artist") == 0)
		{
			strDest = flactag.Artist;
		}
		else if(strcmp(s,"album") == 0)
		{
			strDest = flactag.Album;
		}
		else if(strcmp(s,"date") == 0)
		{
			strDest = flactag.Data;
		}
		else if(strcmp(s,"comment") == 0)
		{
			strDest = flactag.Comment;
		}
		else if(strcmp(s,"tracknumber")==0)
		{
			strDest = flactag.Tracknumber;
		}
		else if(strcmp(s,"genre") == 0)
		{
			strDest = flactag.Genre;
		}

		ALOGD("tag = %s ,value = %s",s,str);
		if(strDest != NULL)
		{
			if(FLAC_LENGTH < (k+1))//fixed by Charles Chen 09/12/12
			{
				memcpy(strDest, str, (FLAC_LENGTH-1));
				strDest[FLAC_LENGTH - 1] = '\0';
			}
			else
				memcpy(strDest, str, k+1);
			//LOGD("set the ID3 value");
		}

		//LOGD("tag = %s ,value = %s",s,str);
		free(str);
		str=NULL;
	}
	return FLAC_SUCCESS;
}
