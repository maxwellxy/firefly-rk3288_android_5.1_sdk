#ifndef _GETAPETAGEX_H_
#define _GETAPETAGEX_H_

#define APE_ERROR -1
#define APE_SUCCESS 0

#define APE_LENGTH 256

struct APETAGEX
{
	char Title[APE_LENGTH];	//标题
	char Artist[APE_LENGTH];	//艺术家
	char Album[APE_LENGTH];	//专辑
	char Year[APE_LENGTH];	//年代
	char Comment[APE_LENGTH];	//备注
	char Track[APE_LENGTH];	//音轨
	char Genre[APE_LENGTH];	//流派
};

class ApeId3
{
		public:
			int getapetagex(FILE *stream);
			struct APETAGEX apetagex;
};

#endif

