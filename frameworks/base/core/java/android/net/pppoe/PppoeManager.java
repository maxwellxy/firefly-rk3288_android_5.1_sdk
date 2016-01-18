/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.net.pppoe;

import android.content.Context;
import android.os.RemoteException;
import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.net.DhcpInfo;
import android.os.Binder;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.Looper;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.util.Log;


/**
 * A class to control the pppoe network.
 *
 * @hide
 */
public class PppoeManager {
    private static final String TAG = "PppoeManager";

    private final IPppoeManager mService;

    Handler mHandler;
    private PppoeHandler mPppoeHandler;
    
    public static boolean DEBUG=true;

    private static void LOG(String msg) {
        if ( DEBUG ) {
            Log.d(TAG, msg);
        }
    }

    /**
     * @hide
     */
    public static final String EXTRA_PPPOE_ERRMSG = "pppoe_errmsg";

    /**
     * @hide
     */

    public static final String PPPOE_STATE_CHANGED_ACTION  = "android.net.pppoe.PPPOE_STATE_CHANGED";

    /**
     * @hide
     */
    public static final String EXTRA_PPPOE_STATE = "pppoe_state";

    /**
     * @hide
     */
    public static final String EXTRA_PREVIOUS_PPPOE_STATE = "previous_pppoe_state";

    /**
     * @hide
     */
    public static final int PPPOE_STATE_UNKNOWN = 4;
    /**
     * @hide
     */
    public static final int PPPOE_STATE_DISCONNECTED = 3;

    /**
     * @hide
     */
    public static final int PPPOE_STATE_DISCONNECTING = 2;

    /**
     * @hide
     */
    public static final int PPPOE_STATE_CONNECTED = 1;  

    /**
     * @hide
     */
    public static final int PPPOE_STATE_CONNECTING = 0;

    /**
     */
    public PppoeManager(IPppoeManager service, Handler handler) {
        mHandler = handler;
        mService = service;

        if ( null == mPppoeHandler ) {
            LOG("PppoeManager() : start 'pppoe handle thread'.");
            HandlerThread handleThread = new HandlerThread("Pppoe Handler Thread");
            handleThread.start();
            mPppoeHandler = new PppoeHandler(handleThread.getLooper()/*, this*/);
        }
    }

    private class PppoeHandler extends Handler {
        private static final int COMMAND_START_PPPOE = 1;
        private static final int COMMAND_STOP_PPPOE = 2;

        private Handler mTarget;
        
        public PppoeHandler(Looper looper) {
            super(looper);
        }

        public void handleMessage(Message msg) {       
            
            int event;
            LOG("PppoeHandler::handleMessage() : Entered : msg = " + msg);

            switch (msg.what) {
                case COMMAND_START_PPPOE:
                    try {
                        mService.startPppoe();
                    } catch (RemoteException e) {
                        Log.e(TAG, "startPppoe failed");
                    }
                    break;
                case COMMAND_STOP_PPPOE:
                    try {
                        mService.stopPppoe();
                    } catch (RemoteException e) {
                        Log.e(TAG, "stopPppoe failed");
                    }
                    break;
                default:
                    break;
            }
        }
    }

    public boolean setupPppoe(String user, String iface, String dns1, String dns2, String password) {
        try {
            return mService.setupPppoe(user, iface, dns1, dns2, password);
        } catch (RemoteException e) {
            return false;
        }
    }

    public boolean startPppoe() {
        return mPppoeHandler.sendEmptyMessage(PppoeHandler.COMMAND_START_PPPOE);
    }

    public boolean stopPppoe() {
        return mPppoeHandler.sendEmptyMessage(PppoeHandler.COMMAND_STOP_PPPOE);
    }

    public String getPppoePhyIface() {
        try {
            return mService.getPppoePhyIface();
        } catch (RemoteException e) {
            return null;
        }
    }

    public int getPppoeState() {
        try {
            return mService.getPppoeState();
        } catch (RemoteException e) {
            Log.e(TAG, "stopPppoe failed");
            return -1;
        }
    }
}
