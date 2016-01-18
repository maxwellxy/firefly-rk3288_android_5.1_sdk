/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *	VER		Descriptions										Date			Author
 *	1.0 		Created for display management								2011-06-29		Zheng Yang
 *	1.1		Fix some bug										2011-09-13		Zheng Yang
 *	1.2		Add USB OTG host and slave mode management.						2011-10-09		Zheng Yang
 *	1.3		Separate YPbPr from TV interface.							2011-11-11		Zheng Yang
 *	1.4		Initialize screen scale after system reset.						2011-11-24		Zheng Yang
 *	1.5		Modified for Android 4.0 platform.							2011-12-15		Zheng Yang
 *	1.6		Support scale display screen with x/y direction.					2012-05-24		Zheng Yang
 *	1.7		Support load HDCP key and SRM file.							2012-05-30		Zheng Yang
 *	1.8		Support Dual screen management.								2012-06-30		Zheng Yang
 *	1.9		Support LED light control.								2012-07-03		Zheng Yang
 *	2.0		Modified for Android 4.1 platform.							2012-08-03		Zheng Yang
 *	2.1		Add HDMI 3D interface.									2012-11-26		Zheng Yang
 *	2.2		Audo detect scale sysfs node.								2012-12-18		Zheng Yang
 *	2.3		1. Rename rk29_display.cfg to display.cfg						2014-06-10		Zheng Yang
 			2. Only save config when user modified setting.
 *	2.4		support bcsh :brightness contrast sat_con hue						2014-06-10		Aishaoxiang
 *	2.5		DisplayManager: hdmi interface is always enabled.					2014-08-09		Zheng Yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "DisplayD"

#include "cutils/log.h"

#include "Config.h"
#include "CommandListener.h"
#include "NetlinkManager.h"
#include "DisplayManager.h"
#if ENABLE_OTG_MANAGER
#include "OtgManager.h"
#endif
#include "ScreenScaleManager.h"
#include "Hdcp.h"

static void coldboot(const char *path);
static void sigchld_handler(int sig);

int main() {

	CommandListener *cl;
	NetlinkManager *nm;
	DisplayManager *dm;
#if ENABLE_OTG_MANAGER	
	OtgManager *om;
#endif
	ScreenScaleManager	*ssm;
		
	ALOGI("Display damen 2.1 starting");
	
	Hdcp_init();
	//    signal(SIGCHLD, sigchld_handler);
	ssm = new ScreenScaleManager();	
	dm = new(DisplayManager);
	if (!(nm = NetlinkManager::Instance(dm))) {
		ALOGE("Unable to create NetlinkManager");
		exit(1);
	};

	cl = new CommandListener(dm, ssm);
	nm->setBroadcaster((SocketListener *) cl);
	
	if (nm->start()) {
		ALOGE("Unable to start NetlinkManager (%s)", strerror(errno));
		exit(1);
	}
#if ENABLE_OTG_MANAGER
	/* Start otg manager */
	om = new OtgManager();
#endif
	/*
	* Now that we're up, we can respond to commands
	*/
	if (cl->startListener()) {
		ALOGE("Unable to start CommandListener (%s)", strerror(errno));
		exit(1);
	}
	sleep(1);
	Hdcp_enable();
	// Eventually we'll become the monitoring thread
	while(1) {
		sleep(1000);
	}
	
	ALOGI("Displayd exiting");
	exit(0);
}

static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if(fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}

static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if(d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}

static void sigchld_handler(int sig) {
    pid_t pid = wait(NULL);
    ALOGD("Child process %d exited", pid);
}
