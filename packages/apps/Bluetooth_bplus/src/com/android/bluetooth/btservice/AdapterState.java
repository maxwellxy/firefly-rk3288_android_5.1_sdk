/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.bluetooth.btservice;

import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.Intent;
import android.os.Message;
import android.util.Log;

import com.android.internal.util.State;
import com.android.internal.util.StateMachine;

/**
 * This state machine handles Bluetooth Adapter State.
 * States:
 *      {@link OnState} : Bluetooth is on at this state
 *      {@link OffState}: Bluetooth is off at this state. This is the initial
 *      state.
 *      {@link PendingCommandState} : An enable / disable operation is pending.
 * TODO(BT): Add per process on state.
 */

final class AdapterState extends StateMachine {
    private static final boolean DBG = true;
    private static final boolean VDBG = false;
    private static final String TAG = "BluetoothAdapterState";

    static final int USER_TURN_ON = 1;
    static final int STARTED=2;
    static final int ENABLED_READY = 3;

    static final int USER_TURN_OFF = 20;
    static final int BEGIN_DISABLE = 21;
    static final int ALL_DEVICES_DISCONNECTED = 22;

    static final int DISABLED = 24;
    static final int STOPPED=25;

    static final int START_TIMEOUT = 100;
    static final int ENABLE_TIMEOUT = 101;
    static final int DISABLE_TIMEOUT = 103;
    static final int STOP_TIMEOUT = 104;
    static final int SET_SCAN_MODE_TIMEOUT = 105;

    static final int USER_TURN_OFF_DELAY_MS=500;

    static final int ENABLED_RADIO = 200;
    static final int BEGIN_ENABLE_RADIO = 201;
    // For other radio can be handled in similar to FM or same as FM
    static final int USER_TURN_ON_RADIO = 202;
    static final int USER_TURN_OFF_RADIO = 203;
    static final int DISABLED_RADIO = 204;
    static final int BEGIN_DISABLE_RADIO = 205;

    //TODO: tune me
    private static final int ENABLE_TIMEOUT_DELAY = 20000;
    private static final int DISABLE_TIMEOUT_DELAY = 20000;
    private static final int START_TIMEOUT_DELAY = 5000;
    private static final int STOP_TIMEOUT_DELAY = 5000;
    private static final int PROPERTY_OP_DELAY =2000;
    private AdapterService mAdapterService;
    private AdapterProperties mAdapterProperties;
    private PendingCommandState mPendingCommandState = new PendingCommandState();
    private OnState mOnState = new OnState();
    private OffState mOffState = new OffState();
    // Irrespective of the Bluetooth State  isRadioOn is used to maintain the Fm state
    private boolean isRadioOn = false;

    public boolean isRadioOn() {
        return isRadioOn;
    }

    public boolean isTurningOn() {
        boolean isTurningOn=  mPendingCommandState.isTurningOn();
        if (VDBG) Log.d(TAG,"isTurningOn()=" + isTurningOn);
        return isTurningOn;
    }

    public boolean isTurningOff() {
        boolean isTurningOff= mPendingCommandState.isTurningOff();
        if (VDBG) Log.d(TAG,"isTurningOff()=" + isTurningOff);
        return isTurningOff;
    }

    private AdapterState(AdapterService service, AdapterProperties adapterProperties) {
        super("BluetoothAdapterState:");
        addState(mOnState);
        addState(mOffState);
        addState(mPendingCommandState);
        mAdapterService = service;
        mAdapterProperties = adapterProperties;
        setInitialState(mOffState);
    }

    public static AdapterState make(AdapterService service, AdapterProperties adapterProperties) {
        Log.d(TAG, "make");
        AdapterState as = new AdapterState(service, adapterProperties);
        as.start();
        return as;
    }

    public void doQuit() {
        quitNow();
    }

    public void cleanup() {
        if(mAdapterProperties != null)
            mAdapterProperties = null;
        if(mAdapterService != null)
            mAdapterService = null;
    }

    private class OffState extends State {
        @Override
        public void enter() {
            infoLog("Entering OffState");
        }

        @Override
        public boolean processMessage(Message msg) {
            AdapterService adapterService = mAdapterService;
            if (adapterService == null) {
                Log.e(TAG,"receive message at OffState after cleanup:" +
                          msg.what);
                return false;
            }
            switch(msg.what) {
               case USER_TURN_ON:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=OFF, MESSAGE = USER_TURN_ON");
                   notifyAdapterStateChange(BluetoothAdapter.STATE_TURNING_ON);
                   mPendingCommandState.setTurningOn(true);
                   transitionTo(mPendingCommandState);
                   sendMessageDelayed(START_TIMEOUT, START_TIMEOUT_DELAY);
                   adapterService.processStart();
                   break;
               case USER_TURN_ON_RADIO:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=OFF, MESSAGE = USER_TURN_ON_RADIO");
                   if (!isRadioOn) {
                       //Enable
                        mPendingCommandState.setTurningOnRadio(true);
                        transitionTo(mPendingCommandState);
                        sendMessage(BEGIN_ENABLE_RADIO);
                    } else {
                        Log.d(TAG,"Radio already turned ON");
                    }

                   break;

               case USER_TURN_OFF:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=OFF, MESSAGE = USER_TURN_OFF");
                   //TODO: Handle case of service started and stopped without enable
                   break;
               case USER_TURN_OFF_RADIO:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=OFF, MESSAGE = USER_TURN_OFF_RADIO, requestId= " + msg.arg1);

                   mPendingCommandState.setTurningOffRadio(true);
                   transitionTo(mPendingCommandState);

                   Message m = obtainMessage(BEGIN_DISABLE_RADIO);
                   m.arg1 = msg.arg1;
                   sendMessage(m);

                   break;

               default:
                   if (DBG) Log.d(TAG,"ERROR: UNEXPECTED MESSAGE: CURRENT_STATE=OFF, MESSAGE = " + msg.what );
                   return false;
            }
            return true;
        }
    }

    private class OnState extends State {
        @Override
        public void enter() {
            infoLog("Entering On State");
            AdapterService adapterService = mAdapterService;
            if (adapterService == null) {
                Log.e(TAG,"enter OnState after cleanup");
                return;
            }
            // If OnState entry is because of turning ON radio the auto connect
            // should not be started as it is BT ON specific behaviour.
            if (mPendingCommandState.isTurningOnRadio())
                mPendingCommandState.setTurningOnRadio(false);
            else
                adapterService.autoConnect();
        }

        @Override
        public boolean processMessage(Message msg) {
            AdapterProperties adapterProperties = mAdapterProperties;
            if (adapterProperties == null) {
                Log.e(TAG,"receive message at OnState after cleanup:" +
                          msg.what);
                return false;
            }

            switch(msg.what) {
               case USER_TURN_OFF:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=ON, MESSAGE = USER_TURN_OFF");
                   notifyAdapterStateChange(BluetoothAdapter.STATE_TURNING_OFF);
                   mPendingCommandState.setTurningOff(true);
                   transitionTo(mPendingCommandState);

                   // Invoke onBluetoothDisable which shall trigger a
                   // setScanMode to SCAN_MODE_NONE
                   Message m = obtainMessage(SET_SCAN_MODE_TIMEOUT);
                   sendMessageDelayed(m, PROPERTY_OP_DELAY);
                   adapterProperties.onBluetoothDisable();
                   break;

               case USER_TURN_OFF_RADIO:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=ON, MESSAGE = USER_TURN_OFF_RADIO");
                   if (isRadioOn) {
                       mPendingCommandState.setTurningOffRadio(true);
                       transitionTo(mPendingCommandState);

                       Message m1 = obtainMessage(BEGIN_DISABLE_RADIO);
                       m1.arg1 = msg.arg1;
                       sendMessage(m1);
                    } else {
                        Log.d(TAG,"Radio already turned OFF");
                    }
                   break;

               case USER_TURN_ON:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=ON, MESSAGE = USER_TURN_ON");
                   Log.i(TAG,"Bluetooth already ON, ignoring USER_TURN_ON");
                   break;
               case USER_TURN_ON_RADIO:
                   if (DBG) Log.d(TAG,"CURRENT_STATE=ON, MESSAGE = USER_TURN_ON_RADIO");
                   if (!isRadioOn) {
                       //Enable
                        mPendingCommandState.setTurningOnRadio(true);
                        transitionTo(mPendingCommandState);
                        sendMessage(BEGIN_ENABLE_RADIO);
                    } else {
                        Log.d(TAG,"Radio already turned ON");
                    }

                   break;

               default:
                   if (DBG) Log.d(TAG,"ERROR: UNEXPECTED MESSAGE: CURRENT_STATE=ON, MESSAGE = " + msg.what );
                   return false;
            }
            return true;
        }
    }

    private class PendingCommandState extends State {
        private boolean mIsTurningOn;
        private boolean mIsTurningOff;
        private boolean mIsTurningOnRadio;
        private boolean mIsTurningOffRadio;

        private int mRequestId;

        public void enter() {
            infoLog("Entering PendingCommandState State: isTurningOn()=" + isTurningOn() + ", isTurningOff()=" + isTurningOff());
        }

        public void setTurningOn(boolean isTurningOn) {
            mIsTurningOn = isTurningOn;
        }

        public boolean isTurningOn() {
            return mIsTurningOn;
        }

        public void setTurningOff(boolean isTurningOff) {
            mIsTurningOff = isTurningOff;
        }

        public boolean isTurningOff() {
            return mIsTurningOff;
        }

        public void setTurningOnRadio(boolean isTurningOnRadio) {
            mIsTurningOnRadio = isTurningOnRadio;
        }

        public boolean isTurningOnRadio() {
            return mIsTurningOnRadio;
        }

        public void setTurningOffRadio(boolean isTurningOffRadio) {
            mIsTurningOffRadio = isTurningOffRadio;
        }

        public boolean isTurningOffRadio() {
            return mIsTurningOffRadio;
        }
        @Override
        public boolean processMessage(Message msg) {

            boolean isTurningOn= isTurningOn();
            boolean isTurningOff = isTurningOff();
            boolean isTurningOnRadio= isTurningOnRadio();
            boolean isTurningOffRadio = isTurningOffRadio();

            AdapterService adapterService = mAdapterService;
            AdapterProperties adapterProperties = mAdapterProperties;
            if ((adapterService == null) || (adapterProperties == null)) {
                Log.e(TAG,"receive message at Pending State after cleanup:" +
                          msg.what);
                return false;
            }

            switch (msg.what) {
                case USER_TURN_ON:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = USER_TURN_ON"
                            + ", isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    if (isTurningOn) {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Alreadying turning on bluetooth... Ignoring USER_TURN_ON...");
                    } else {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Deferring request USER_TURN_ON");
                        deferMessage(msg);
                    }
                    break;
                case USER_TURN_OFF:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = USER_TURN_ON"
                            + ", isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    if (isTurningOff) {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Alreadying turning off bluetooth... Ignoring USER_TURN_OFF...");
                    } else {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Deferring request USER_TURN_OFF");
                        deferMessage(msg);
                    }
                    break;
                case USER_TURN_ON_RADIO:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = USER_TURN_ON_RADIO"
                            + ", isTurningOn=" + isTurningOnRadio + ", isTurningOff=" + isTurningOffRadio);
                    if (isTurningOnRadio) {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Alreadying turning on Radio... Ignoring USER_TURN_ON_RADIO...");
                    } else {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Deferring request USER_TURN_ON_RADIO");
                        deferMessage(msg);
                    }
                    break;
                case USER_TURN_OFF_RADIO:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = USER_TURN_OFF_RADIO"
                            + ", isTurningOn=" + isTurningOnRadio + ", isTurningOff=" + isTurningOffRadio);
                    if (isTurningOffRadio) {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Alreadying turning off Radio... Ignoring USER_TURN_OFF_RADIO...");
                    } else {
                        Log.i(TAG,"CURRENT_STATE=PENDING: Deferring request USER_TURN_OFF_RADIO");
                        deferMessage(msg);
                    }
                    break;

                case STARTED:   {
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = STARTED, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    //Remove start timeout
                    removeMessages(START_TIMEOUT);

                    //Enable
                    boolean ret = adapterService.enableNative();
                    if (!ret) {
                        Log.e(TAG, "Error while turning Bluetooth On");
                        notifyAdapterStateChange(BluetoothAdapter.STATE_OFF);
                        transitionTo(mOffState);
                    } else {
                        sendMessageDelayed(ENABLE_TIMEOUT, ENABLE_TIMEOUT_DELAY);
                    }
                }
                    break;

                case BEGIN_ENABLE_RADIO:   {
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = BEGIN_ENABLE_RADIO, isTurningOnRadio="
                        + isTurningOnRadio + ", isTurningOffRadio=" + isTurningOffRadio);

                    //Enable
                    boolean ret = mAdapterService.enableRadioNative();
                    if (!ret) {
                        Log.e(TAG, "Error while turning Radio On");
                        if (BluetoothAdapter.STATE_OFF == mAdapterProperties.getState())
                            transitionTo(mOffState);
                        else
                            transitionTo(mOnState);
                        mPendingCommandState.setTurningOnRadio(false);
                    } else {
                        sendMessageDelayed(ENABLE_TIMEOUT, ENABLE_TIMEOUT_DELAY);
                    }

                }
                    break;

                case ENABLED_READY:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = ENABLE_READY, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    removeMessages(ENABLE_TIMEOUT);
                    adapterProperties.onBluetoothReady();
                    mPendingCommandState.setTurningOn(false);
                    transitionTo(mOnState);
                    notifyAdapterStateChange(BluetoothAdapter.STATE_ON);
                    break;
                case ENABLED_RADIO:
                    if (DBG) Log.d(TAG,
                        "CURRENT_STATE=PENDING, MESSAGE = ENABLED_RADIO, isTurningOnRadio="
                        + isTurningOnRadio + ", isTurningOffRadio=" + isTurningOffRadio);
                    removeMessages(ENABLE_TIMEOUT);
                    isRadioOn = true;
                    if (BluetoothAdapter.STATE_OFF == mAdapterProperties.getState()){
                        mPendingCommandState.setTurningOnRadio(false);
                        transitionTo(mOffState);
                        notifyAdapterRadioStateChange(BluetoothAdapter.STATE_RADIO_ON);
                    }
                    else {
                        transitionTo(mOnState);
                        // Retain TurningOnRadio info till new state enter happens
                        // for skipping auto connect for radio turn ON
                        notifyAdapterRadioStateChange(BluetoothAdapter.STATE_RADIO_ON);
                    }

                    break;
                case SET_SCAN_MODE_TIMEOUT:
                     Log.w(TAG,"Timeout will setting scan mode..Continuing with disable...");
                     //Fall through
                case BEGIN_DISABLE: {
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = BEGIN_DISABLE, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    removeMessages(SET_SCAN_MODE_TIMEOUT);
                    sendMessageDelayed(DISABLE_TIMEOUT, DISABLE_TIMEOUT_DELAY);
                    boolean ret = adapterService.disableNative();
                    if (!ret) {
                        removeMessages(DISABLE_TIMEOUT);
                        Log.e(TAG, "Error while turning Bluetooth Off");
                        //FIXME: what about post enable services
                        mPendingCommandState.setTurningOff(false);
                        notifyAdapterStateChange(BluetoothAdapter.STATE_ON);
                    }
                }
                    break;
                case BEGIN_DISABLE_RADIO: {
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = BEGIN_DISABLE_RADIO" +
                        isTurningOnRadio + ", isTurningOffRadio=" + isTurningOffRadio);
                    sendMessageDelayed(DISABLE_TIMEOUT, DISABLE_TIMEOUT_DELAY);
                    boolean ret = mAdapterService.disableRadioNative();
                    if (!ret) {
                        removeMessages(DISABLE_TIMEOUT);
                        Log.e(TAG, "Error while turning Radio Off");
                        mPendingCommandState.setTurningOffRadio(false);
                    }
                  }
                    break;

                case DISABLED:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = DISABLED, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    if (isTurningOn) {
                        removeMessages(ENABLE_TIMEOUT);
                        errorLog("Error enabling Bluetooth - hardware init failed");
                        mPendingCommandState.setTurningOn(false);
                        transitionTo(mOffState);
                        adapterService.stopProfileServices();
                        notifyAdapterStateChange(BluetoothAdapter.STATE_OFF);
                        break;
                    }
                    removeMessages(DISABLE_TIMEOUT);
                    sendMessageDelayed(STOP_TIMEOUT, STOP_TIMEOUT_DELAY);
                    if (adapterService.stopProfileServices()) {
                        Log.d(TAG,"Stopping profile services that were post enabled");
                        break;
                    }
                    //Fall through if no services or services already stopped
                case STOPPED:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = STOPPED, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    removeMessages(STOP_TIMEOUT);
                    setTurningOff(false);
                    transitionTo(mOffState);
                    notifyAdapterStateChange(BluetoothAdapter.STATE_OFF);
                    break;
                case DISABLED_RADIO:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = DISABLED_RADIO, isTurningOnRadio="
                        + isTurningOnRadio + ", isTurningOffRadio=" + isTurningOffRadio());
                    if (isTurningOnRadio) {
                        removeMessages(ENABLE_TIMEOUT);
                        errorLog("Error enabling radio - hardware init failed");
                        mPendingCommandState.setTurningOnRadio(false);
                    }
                    removeMessages(DISABLE_TIMEOUT);
                    setTurningOffRadio(false);
                    isRadioOn = false;
                    if (BluetoothAdapter.STATE_ON == mAdapterProperties.getState()) {
                        transitionTo(mOnState);
                        notifyAdapterRadioStateChange(BluetoothAdapter.STATE_RADIO_OFF);
                    } else {
                        transitionTo(mOffState);
                        notifyAdapterRadioStateChange(BluetoothAdapter.STATE_RADIO_OFF);
                        //mAdapterService.startShutdown(requestId1);
                    }

                    break;

                case START_TIMEOUT:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = START_TIMEOUT, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    errorLog("Error enabling Bluetooth");
                    mPendingCommandState.setTurningOn(false);
                    transitionTo(mOffState);
                    notifyAdapterStateChange(BluetoothAdapter.STATE_OFF);
                    break;
                case ENABLE_TIMEOUT:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = ENABLE_TIMEOUT, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    errorLog("Error enabling Bluetooth/FM-Radio");
                    mPendingCommandState.setTurningOn(false);
                    mPendingCommandState.setTurningOnRadio(false);
                    isRadioOn = false;
                    transitionTo(mOffState);
                    notifyAdapterStateChange(BluetoothAdapter.STATE_OFF);
                    break;
                case STOP_TIMEOUT:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = STOP_TIMEOUT, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    errorLog("Error stopping Bluetooth profiles");
                    mPendingCommandState.setTurningOff(false);
                    transitionTo(mOffState);
                    notifyAdapterStateChange(BluetoothAdapter.STATE_OFF);
                    break;
                case DISABLE_TIMEOUT:
                    if (DBG) Log.d(TAG,"CURRENT_STATE=PENDING, MESSAGE = DISABLE_TIMEOUT, isTurningOn=" + isTurningOn + ", isTurningOff=" + isTurningOff);
                    errorLog("Error disabling Bluetooth");
                    mPendingCommandState.setTurningOff(false);
                    mPendingCommandState.setTurningOffRadio(false);
                    transitionTo(mOnState);
                    notifyAdapterStateChange(BluetoothAdapter.STATE_ON);
                    break;
                default:
                    if (DBG) Log.d(TAG,"ERROR: UNEXPECTED MESSAGE: CURRENT_STATE=PENDING, MESSAGE = " + msg.what );
                    return false;
            }
            return true;
        }
    }


    private void notifyAdapterStateChange(int newState) {
        AdapterService adapterService = mAdapterService;
        AdapterProperties adapterProperties = mAdapterProperties;
        if ((adapterService == null) || (adapterProperties == null)) {
            Log.e(TAG,"notifyAdapterStateChange after cleanup:" + newState);
            return;
        }

        int oldState = adapterProperties.getState();
        adapterProperties.setState(newState);
        infoLog("Bluetooth adapter state changed: " + oldState + "-> " + newState);
        adapterService.updateAdapterState(oldState, newState);
    }

    private void notifyAdapterRadioStateChange(int newState) {
        infoLog("Bluetooth adapter radio state changed: " + newState);
        // Use the already defined callback prototype to send the bt_state and new_fm_state
        // TBD to use seperate callback while integration wtih FM app
        mAdapterService.updateAdapterState(mAdapterProperties.getState(), newState);
    }

    void stateChangeCallback(int status) {
        if (status == AbstractionLayer.BT_STATE_OFF) {
            sendMessage(DISABLED);
        } else if (status == AbstractionLayer.BT_STATE_ON) {
            // We should have got the property change for adapter and remote devices.
            sendMessage(ENABLED_READY);
        } else if (status == AbstractionLayer.BT_RADIO_OFF) {
            sendMessage(DISABLED_RADIO);
        } else if (status == AbstractionLayer.BT_RADIO_ON) {
            sendMessage(ENABLED_RADIO);
        } else {
            errorLog("Incorrect status in stateChangeCallback");
        }
    }

    private void infoLog(String msg) {
        if (DBG) Log.i(TAG, msg);
    }

    private void errorLog(String msg) {
        Log.e(TAG, msg);
    }

}
