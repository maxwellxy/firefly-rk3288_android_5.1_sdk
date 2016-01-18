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

#ifndef _COMMANDLISTENER_H__
#define _COMMANDLISTENER_H__

#include <sysutils/FrameworkListener.h>
#include "DisplaydCommand.h"
#include "DisplayManager.h"
#include "ScreenScaleManager.h"
#include "BcshManager.h"
class CommandListener : public FrameworkListener {
	static DisplayManager	*mDisplayManager;
	static ScreenScaleManager *mScreenScaleManager;
	static BcshManger *mBcshManger;
public:
    CommandListener(DisplayManager *dm, ScreenScaleManager *ssm);
    virtual ~CommandListener() {}

private:
	class InterfaceCmd : public DisplaydCommand {
    public:
        InterfaceCmd();
        virtual ~InterfaceCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };
    
    class ModeCmd : public DisplaydCommand {
    public:
        ModeCmd();
        virtual ~ModeCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };
    class UtilsCmd : public DisplaydCommand {
    public:
        UtilsCmd();
        virtual ~UtilsCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };
};

#endif
