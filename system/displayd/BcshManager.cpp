#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <signal.h>
#include <sys/wait.h>
#include <cutils/properties.h>
#include <sys/stat.h>
#include <math.h>

#define LOG_TAG "BcshManager"
#include <cutils/log.h>


#include <sysutils/SocketClient.h>
#include "BcshManager.h"

#define BCSH_SYSFS_NODE	"/sys/class/graphics/fb0/bcsh"
#define BCSH_TYPE_BRIGHT "brightness "
#define BCSH_TYPE_CONTRAST "contrast"
#define BCSH_TYPE_STACON "sat_con"
#define BCSH_TYPE_HUE "hue"

BcshManger::BcshManger()
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, BCSH_SYSFS_NODE);
	char tmp[16] = "open";
	fd = fopen(buf, "wb");
	if(fd != NULL) {
		fwrite(tmp, strlen(tmp), 1, fd);
		fclose(fd);
	}
	init();

}

void BcshManger::init()
{
	char property[PROPERTY_VALUE_MAX];
	
	mBrightness = 0;
	mContrast = 1;
	mSaturation = 1;
	mDegree = 0;

	memset(property, 0, PROPERTY_VALUE_MAX);
	if(property_get("persist.sys.bcsh.brightness", property, 0) > 0)
		setBrightness(0, atoi(property));
	memset(property, 0, PROPERTY_VALUE_MAX);
	if(property_get("persist.sys.bcsh.contrast", property, 0) > 0)
		setContrast(0, atof(property));
	memset(property, 0, PROPERTY_VALUE_MAX);
	if(property_get("persist.sys.bcsh.saturation", property, 0) > 0)
		setSaturation(0, atof(property));
	memset(property, 0, PROPERTY_VALUE_MAX);	
	if(property_get("persist.sys.bcsh.hue", property, 0) > 0)
		setHue(0, atof(property));
}
/*
 * brightness: [-32, 31], default 0
 */
int BcshManger::setBrightness(int display,int brightness)
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];

	if (brightness < -32 || brightness > 31)
		return -1;

	if (brightness == mBrightness)
		return 0;

	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, BCSH_SYSFS_NODE);
	
	fd = fopen(buf, "wb");
	if(fd == NULL)
		return -1;
	ALOGD("%s %d %d", __func__, display, brightness);
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf,"brightness %d",brightness + 128);
	fwrite(buf, strlen(buf), 1, fd);
	fclose(fd);

	mBrightness = brightness;
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf,"%d", brightness);
	property_set("persist.sys.bcsh.brightness",buf);
	return 0;
}

/*
 * contrast: [0, 1.992], default 1;
 */
int BcshManger::setContrast(int display, float contrast)
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];

	if (contrast < 0 || contrast > 1.992)
		return -1;

	if (mContrast == contrast)
		return 0;

	ALOGD("%s %d %f", __func__, display, contrast);
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, BCSH_SYSFS_NODE);

	fd = fopen(buf, "wb");
	if(fd == NULL)
		return -1;
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf, "contrast %d", (int)round(contrast * 256));
	fwrite(buf,strlen(buf),1, fd);
	fclose(fd);
	mContrast = contrast;
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf, "%f", contrast);
	property_set("persist.sys.bcsh.contrast", buf);

	setSaturation(display, mSaturation);
	return 0;
}

/*
 * saturation: [0, 1.992], default 1;
 */
int BcshManger::setSaturation(int display,float saturation)
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];

	if (saturation < 0 || saturation > 1.992)
		return -1;

	ALOGD("%s %d %f mContrast %f", __func__, display, saturation, mContrast);
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, BCSH_SYSFS_NODE);
	fd = fopen(buf, "wb");
	if(fd == NULL)
		return -1;
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf,"sat_con %d",(int)round(saturation * mContrast * 256));
	fwrite(buf, strlen(buf), 1, fd);
	fclose(fd);
	if (saturation != mSaturation) {
		mSaturation = saturation;
		memset(buf, 0, BUFFER_LENGTH);
		sprintf(buf,"%f", saturation);
		property_set("persist.sys.bcsh.saturation",buf);
	}
	return 0;
}

/*
 * degree: [-30, 30], default 0
 */
int BcshManger::setHue(int display, float degree)
{
	FILE *fd = NULL;
	int sin_hue, cos_hue;
	char buf[BUFFER_LENGTH];
	
	if (degree < -30 || degree > 30)
		return -1;

	if (degree == mDegree)
		return 0;

	ALOGD("%s %d %f", __func__, display, degree);

	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, BCSH_SYSFS_NODE);
	fd = fopen(buf, "wb");
	if(fd == NULL)
		return -1;
	if (degree < 0)
		sin_hue = (int)(sin(degree * 3.1415926 / 180) * 256) + 256;
	else
		sin_hue = (int)(sin(degree * 3.1415926 / 180) * 256);
	cos_hue = (int)(cos(degree * 3.1415926 / 180) * 256);
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf,"hue %d %d", sin_hue, cos_hue);
	fwrite(buf,strlen(buf),1, fd);
	fclose(fd);

	mDegree = degree;
	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf,"%f",degree);
	property_set("persist.sys.bcsh.hue",buf);
	return 0;
}

