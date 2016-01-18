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
 */

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#include <linux/if.h>

#define LOG_TAG "CommandListener"
#include <cutils/log.h>

#include <sysutils/SocketClient.h>
#include "Config.h"
#include "CommandListener.h"
#include "ResponseCode.h"

DisplayManager	*CommandListener::mDisplayManager = NULL;
ScreenScaleManager	*CommandListener::mScreenScaleManager = NULL;
BcshManger	*CommandListener::mBcshManger = NULL;
CommandListener::CommandListener(DisplayManager *dm, ScreenScaleManager *ssm) :
                 FrameworkListener("displayd", true) {
	ALOGD(" FrameworkListener displayd start");
	registerCmd(new InterfaceCmd());
	registerCmd(new ModeCmd());
	registerCmd(new UtilsCmd());
	mDisplayManager = dm;
	mScreenScaleManager = ssm;
	mBcshManger = new BcshManger();
}

CommandListener::InterfaceCmd::InterfaceCmd() :
                 DisplaydCommand("interface") {
}

int CommandListener::InterfaceCmd::runCommand(SocketClient *cli, int argc, char **argv) {
	if (argc < 3) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }
    if (!strcmp(argv[1], "list")) {
    	CommandListener::mDisplayManager->getIfaceInfo(cli, atoi(argv[2]));
	} else if (!strcmp(argv[1], "getcur")) {
		CommandListener::mDisplayManager->getCurIface(cli, atoi(argv[2]));
	} else if(!strcmp(argv[1], "set")) {
		char enable = 0;
		if(!strcmp(argv[4], "true"))
			enable = 1;
		else if(!strcmp(argv[4], "false"))
			enable = 0;
		if(!mDisplayManager->enableIface(atoi(argv[2]), argv[3], enable))
			cli->sendMsg(ResponseCode::CommandOkay, "Interface set completed", false);
		else
			cli->sendMsg(ResponseCode::OperationFailed, "Interface set failed", false);
	} else if(!strcmp(argv[1], "switch")) {
		CommandListener::mDisplayManager->selectNextIface(atoi(argv[2]));
		cli->sendMsg(ResponseCode::CommandOkay, "Switch interface completed", false);
	} else
		cli->sendMsg(ResponseCode::CommandSyntaxError, "Unkown command", false);
	return 0;
}

CommandListener::ModeCmd::ModeCmd() :
                 DisplaydCommand("mode") {
}

int CommandListener::ModeCmd::runCommand(SocketClient *cli, int argc, char **argv) {

	if (argc < 4) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }
	ALOGD(":ModeCmd::runCommand argv[1] = %s",argv[1]);
    if(!strcmp(argv[1], "list")) {
	mDisplayManager->getModeList(cli, atoi(argv[2]), argv[3]);
    } else if(!strcmp(argv[1], "get")) {
    	mDisplayManager->getCurMode(cli, atoi(argv[2]), argv[3]);
    } else if(!strcmp(argv[1], "set")) {
	mDisplayManager->setMode(atoi(argv[2]), argv[3], argv[4]);
		cli->sendMsg(ResponseCode::CommandOkay, "Mode set completed", false);
    } else if(!strcmp(argv[1], "get3dmodes")) {
    	mDisplayManager->get3DModes(cli, atoi(argv[2]), argv[3]);
    } else if(!strcmp(argv[1], "get3dmode")) {
    	mDisplayManager->get3DMode(cli, atoi(argv[2]), argv[3]);
    } else if(!strcmp(argv[1], "set3dmode")) {
    	if(mDisplayManager->set3DMode(atoi(argv[2]), argv[3], argv[4]) == 0)
    		cli->sendMsg(ResponseCode::CommandOkay, "Set 3D Mode completed", false);
    	else
    		cli->sendMsg(ResponseCode::OperationFailed, "Set 3D Mode failed", false);
    } else
    	cli->sendMsg(ResponseCode::CommandSyntaxError, "Unkown command", false);
	return 0;
}

CommandListener::UtilsCmd::UtilsCmd() :
                 DisplaydCommand("utils") {
}

int CommandListener::UtilsCmd::runCommand(SocketClient *cli, int argc, char **argv) {

	//ALOGD("runCommand argv[1] = %s argv[2] = %s",argv[1],argv[2]);
	if (argc < 2) {
		ALOGD("argc = %d",argc);
        	cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        	return 0;
	}
	if (!strcmp(argv[1], "switch")) {
		#if ENABLE_SWITCH_FRAMEBUFFER
		cli->sendMsg(ResponseCode::CommandOkay, "true", false);
		#else
		cli->sendMsg(ResponseCode::CommandOkay, "false", false);
		#endif
	} else if (!strcmp(argv[1], "scaleset")) {
		if(argc != 5)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			int direction = atoi(argv[3]);
			int value = atoi(argv[4]);
			if(direction <= DISPLAY_OVERSCAN_ALL)
				mScreenScaleManager->SSMCtrl(atoi(argv[2]), direction, value);
			else
				cli->sendMsg(ResponseCode::CommandParameterError, "Unkown direction", false);
			cli->sendMsg(ResponseCode::CommandOkay, "Screen scale set ok", false);
		}
	} else if (!strcmp(argv[1], "switchfb")) {
		if(argc != 5)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			mDisplayManager->switchFramebuffer(atoi(argv[2]),atoi(argv[3]), atoi(argv[4]));
			cli->sendMsg(ResponseCode::CommandOkay, "switch fb ok", false);
		}
	} else if (!strcmp(argv[1], "brightness")){
		if(argc != 4)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			mBcshManger->setBrightness(atoi(argv[2]), atoi(argv[3]));
			cli->sendMsg(ResponseCode::CommandOkay, "set brightness ok", false);
		}
	} else if (!strcmp(argv[1], "contrast")){
		if(argc != 4)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			mBcshManger->setContrast(atoi(argv[2]), atof(argv[3]));
			cli->sendMsg(ResponseCode::CommandOkay, "set contrast ok", false);
		}
	} else if (!strcmp(argv[1], "saturation")){
		if(argc != 4)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			mBcshManger->setSaturation(atoi(argv[2]), atof(argv[3]));
			cli->sendMsg(ResponseCode::CommandOkay, "set sta_con ok", false);
		}
	} else if (!strcmp(argv[1], "hue")){
		if(argc != 4)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			mBcshManger->setHue(atoi(argv[2]), atof(argv[3]));
			cli->sendMsg(ResponseCode::CommandOkay, "set hue ok", false);
		}
	} else if (!strcmp(argv[1], "save")){
		if(argc != 2)
			cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
		else {
			CommandListener::mDisplayManager->saveConfig();
			cli->sendMsg(ResponseCode::CommandOkay, "set save ok", false);
		}
	}
	else
		cli->sendMsg(ResponseCode::CommandSyntaxError, "Unkown command", false);
	return 0;
}
