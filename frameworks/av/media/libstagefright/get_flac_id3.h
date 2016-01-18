#ifndef _GETFLACTAG_H_
#define _GETFLACTAG_H_

#define FLAC_ERROR -1
#define FLAC_END -1
#define FLAC_SUCCESS 0

#define FLAC_LENGTH 128


struct FLACTAG
{
	char Title[FLAC_LENGTH];	//标题
	char Artist[FLAC_LENGTH];	//艺术家
	char Album[FLAC_LENGTH];	//专辑
	char Data[FLAC_LENGTH];	//年代
	char Comment[FLAC_LENGTH];	//备注
	char Tracknumber[FLAC_LENGTH];	//音轨
	char Genre[FLAC_LENGTH];	//流派
};

class FlacId3
{
	public:
			struct FLACTAG flactag;
			int handleFlacTag(char *buffer);
			int getFlacTag(FILE *stream);
};
#endif

