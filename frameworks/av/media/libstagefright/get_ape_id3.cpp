#include<stdio.h>
#include<string.h>
#include "get_ape_id3.h"
#include "utils/Log.h"

#undef ALOGD
#define ALOGD
#define ALOGI ALOGD


int ApeId3::getapetagex(FILE *stream)
{
	int i = 0;
	int l = 32,j,k;
	char c;
	char *str,*buffer ,*strDest;
	int *p;
	int cbRead;
	char s[APE_LENGTH];
	unsigned char buf[8];
	p=&i;
	ALOGD("1");
	memset(&apetagex, 0, sizeof(struct APETAGEX));

	if(stream == NULL)
	{
		return APE_ERROR;
	}

	fseek(stream, -32, SEEK_END);
	cbRead = fread(buf, 1, 8, stream);
	if(cbRead != 8)
	{
		return APE_ERROR; // file too short
	}
	ALOGD("2");
	if(!(buf[0]==0x41 && buf[1]==0x50 && buf[2]==0x45 && buf[3]==0x54 &&
		buf[4]==0x41 && buf[5]==0x47 && buf[6]==0x45 && buf[7]==0x58))
	{

		return APE_ERROR; //没有APETAGEX标志，返回
	}

	fseek(stream, -20, SEEK_END);
	fread(p, 4, 1, stream);
	buffer = (char *)malloc(i);
	if(buffer == NULL)
	{
		return APE_ERROR;
	}
	ALOGD("3");
	fseek(stream, -(i+32), SEEK_END);
	fread(buffer, i, 1, stream);//将apetagex读入到buffer中
	while(l < i-1 )
	{
		j=(int)buffer[l]+(((int)buffer[l+1])<<8);
		str=(char *)malloc(j+1);
		if(str == NULL)
		{
			return APE_ERROR;
		}
		l += 2;
		c = buffer[l];
		while(c == 0)
		{
			c = buffer[l];
			l++;
		}
		/*read ID3 tag*/

		s[0]=c;
		k=0;
		while(c)
		{
			k++;
			c=buffer[l];
			if(k < APE_LENGTH-1)//fixed by Charles Chen 09/12/12
				s[k]=c;
			else
				k = APE_LENGTH-1;
			l++;
		}
		s[k]='\0';

		/*read ID3 info*/

			c=buffer[l];
			l++;
		/*	if(c == 0xa4)
			{
				LOGI("find first char is 0xa4");
				c = 0xe4;
			}*/

			str[0]=c;
			for(k=1; k<=j-1; k++)
			{
				c=buffer[l];
				str[k]=c;
				l++;
				if(l > i-1)
					break;
			}
			str[k] ='\0';
			ALOGD("type = %s value = %s",s,str);




		/*save the ID3 info to the struct*/
		 strDest = NULL;
		if(strcmp(s,"Title") == 0)
		{
			strDest = apetagex.Title;
		}
		else if(strcmp(s,"Artist") == 0)
		{
			strDest = apetagex.Artist;
		}
		else if(strcmp(s,"Album") == 0)
		{
			strDest = apetagex.Album;
		}
		else if(strcmp(s,"Year") == 0)
		{
			strDest = apetagex.Year;
		}
		else if(strcmp(s,"Comment") == 0)
		{
			strDest = apetagex.Comment;
		}
		else if(strcmp(s,"Track")==0)
		{
			strDest = apetagex.Track;
		}
		else if(strcmp(s,"Genre") == 0)
		{
			strDest = apetagex.Genre;
		}
		ALOGD("tag = %s ,value = %s",s,str);
		if(strDest != NULL)
		{
			if(APE_LENGTH < (k+1))//fixed by Charles Chen 09/12/12
			{
				memcpy(strDest, str, (APE_LENGTH-1));
				strDest[APE_LENGTH - 1] = '\0';
			}
			else
				memcpy(strDest, str, k+1);
			ALOGD("set the ID3 value");
		}

		free(str);
		str=NULL;
		ALOGD("one tag get finish");
	}
	free(buffer);
	buffer=NULL;
	ALOGD("all finish");
	return APE_SUCCESS;
}
