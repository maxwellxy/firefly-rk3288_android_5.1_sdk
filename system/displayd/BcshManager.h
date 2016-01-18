#ifndef _BCSHMANAGER_H__
#define _BCSHMANAGER_H__
#define BUFFER_LENGTH	256

class BcshManger
{
public:
	BcshManger();
	~BcshManger();
	int setBrightness(int display, int brightness);//0~255
	int setContrast(int display, float contrast);
	int setSaturation(int display, float saturation);
	int setHue(int display, float degree);
private:
	int mBrightness;
	float mContrast;
	float mSaturation;
	float mDegree;
	void init();
};
#endif
