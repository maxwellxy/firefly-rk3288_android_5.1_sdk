/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/


package com.broadcom.fm.fmreceiver;


import com.broadcom.fm.fmreceiver.FmProxy;
import com.broadcom.fm.fmreceiver.FmNativehandler;
import com.broadcom.fm.fmreceiver.IFmReceiverCallback;
import com.broadcom.fm.fmreceiver.IFmReceiverService;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class FmService extends Service {
    public static final String TAG = "FmService";
    private static final boolean D = true,V = true;

    private FmReceiverServiceStub mBinder;
    public FmNativehandler mSvcHandler = new FmNativehandler(this);

    public FmService() {
        super();


    }

    @Override
    public void onCreate() {
        super.onCreate();
        mBinder = new FmReceiverServiceStub(this);

        mBinder.mSvc.mSvcHandler.start();
        if (V) Log.v(TAG, "FM Service  onCreate");
     }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
        mBinder.cleanUp();
        mBinder =null;

        Log.d(TAG, "onDestroy done");
    }

    public IBinder onBind(Intent intent) {
        if (D) {
            Log.d(TAG, "Binding service...");
        }
        return mBinder;
    }

    private static final class FmReceiverServiceStub extends IFmReceiverService.Stub {

        private static final String TAG = "FmService";

        /* Local execution context track. */
        private FmService mSvc;

        /**
         * Constructor used by the system to initialize the operating context of
         * the server. Should not be called directly by an application. Instead,
         * use the local application contexts getSystemService() function to get
         * a reference to a dedicated proxy {@link android.fm#FmProxy} class
         * instance.
         */
        public FmReceiverServiceStub(FmService service) {
            /* Store parameters locally. */
            mSvc = service;

            Log.d(TAG,"FmReceiverServiceStub created"+FmReceiverServiceStub.this+"service"+service);

        }

        public void cleanUp() {
            mSvc.mSvcHandler.stop();
            mSvc.mSvcHandler.finish();
            mSvc.mSvcHandler =null;
        }



        /**
         * Registers an event handler to receive callbacks and callouts from the
         * server.
         *
         * @param cb
         *            the callback to register.
         */
        public synchronized void registerCallback(IFmReceiverCallback cb) throws RemoteException {
            if (mSvc == null) {
                return;
            }
            mSvc.mSvcHandler.registerCallback(cb);
        }

        /**
         * Unregisters an event handler to receive callbacks and callouts from
         * the server.
         *
         * @param cb
         *            the callback to unregister.
         */
        public synchronized void unregisterCallback(IFmReceiverCallback cb)
                throws RemoteException {
            if (mSvc == null) {
                return;
            }
            mSvc.mSvcHandler.unregisterCallback(cb);
        }

        /**
         * Turns on the radio and plays audio using the specified functionality
         * mask.
         * <p>
         * After executing this function, the application should wait for a
         * confirmatory status event callback before calling further API
         * functions. Furthermore, applications should call the
         * {@link #turnOffRadio()} function before shutting down.
         *
         * @param functionalityMask
         *            is a bitmask comprised of one or more of the following
         *            fields: {@link FmProxy#FUNC_REGION_NA},
         *            {@link FmProxy#FUNC_REGION_JP},
         *            {@link FmProxy#FUNC_REGION_EUR},
         *            {@link FmProxy#FUNC_RDS}, {@link FmProxy#FUNC_RDBS}
         *            and {@link FmProxy#FUNC_AF}
         * @param clientPackagename
         *                 is the the client application package name , this is required for the
         *                 fm service to clean up it state when the client process gets killed
         *                 eg scenario: when client app dies without calling turnOffRadio()
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
       @Override
        public synchronized int turnOnRadio(int functionalityMask,
                char[] clientPackagename) throws RemoteException{
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            Log.d(TAG, "turnOnRadio() mSvc:"+mSvc);
            return mSvc.mSvcHandler.turnOnRadio(functionalityMask,clientPackagename);
        }

        /**
         * Turns off the radio.
         * <p>
         * After executing this function, the application should wait for a
         * confirmatory status event callback before shutting down.
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int turnOffRadio() throws RemoteException{
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.turnOffRadio();
        }

        /**
         * Force cleans the FM Receiver Service
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int cleanupFmService() {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.cleanupFmService();
        }

        /**
         * Tunes the radio to a specific frequency. If successful results in a
         * status event callback.
         *
         * @param freq
         *            the frequency to tune to.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int tuneRadio(int freq) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.tuneRadio(freq);
        }

        /**
         * Gets current radio status. This results in a status event callback.
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int getStatus() {
            if (mSvc == null) {
                return FmProxy.STATUS_ILLEGAL_COMMAND;
            }
            return mSvc.mSvcHandler.getStatus();
        }

        /**
         * Mutes/unmutes radio audio. If muted the hardware will stop sending
         * audio. This results in a status event callback.
         *
         * @param mute
         *            TRUE to mute audio, FALSE to unmute audio.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int muteAudio(boolean mute) {
            if (mSvc == null) {
                return FmProxy.STATUS_ILLEGAL_COMMAND;
            }
            return mSvc.mSvcHandler.muteAudio(mute);
        }

        /**
         * Scans FM toward higher/lower frequency for next clear channel. Will
         * result in a seek complete event callback.
         *
         * @param scanMode
         *            see {@link FmProxy#SCAN_MODE_NORMAL},
         *            {@link FmProxy#SCAN_MODE_DOWN},
         *            {@link FmProxy#SCAN_MODE_UP} and
         *            {@link FmProxy#SCAN_MODE_FULL}.
         * @param minSignalStrength
         *            minimum signal strength, default =
         *            {@link FmProxy#MIN_SIGNAL_STRENGTH_DEFAULT}
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int seekStation(int scanMode, int minSignalStrength) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.seekStation(scanMode, minSignalStrength);
        }

        /**
         * Scans FM toward higher/lower frequency for next clear channel. Will
         * result in a seek complete event callback. Will seek out a station
         * that matches the requested value for the desired RDS functionality
         * support.
         *
         * @param scanMode
         *            see {@link FmProxy#SCAN_MODE_NORMAL},
         *            {@link FmProxy#SCAN_MODE_DOWN},
         *            {@link FmProxy#SCAN_MODE_UP} and
         *            {@link FmProxy#SCAN_MODE_FULL}.
         * @param minSignalStrength
         *            Minimum signal strength, default =
         *            {@link FmProxy#MIN_SIGNAL_STRENGTH_DEFAULT}
         * @param rdsCondition
         *            the type of RDS condition to scan for.
         * @param rdsValue
         *            the condition value to match.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int seekRdsStation(int scanMode, int minSignalStrength,
                int rdsCondition, int rdsValue) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.seekRdsStation(scanMode, minSignalStrength,
                rdsCondition, rdsValue);
        }

        /**
         * Aborts the current station seeking operation if any. Will result in a
         * seek complete event containing the last scanned frequency.
         * <p>
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int seekStationAbort() {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.seekStationAbort();
        }

        /**
         * Scans FM toward higher/lower frequency for next clear channel depending on 
         * the scanDirection.will do wrap around when reached to mMaxFreq/mMinFreq,
         * when no wrap around is needed, use the low_bound or high_bound as endFrequency.
         * Will result in a seek complete event callback.
         * <p>
         *
         * @param startFrequency
         *            Starting frequency of search operation range.
         * @param endFrequency
         *            Ending frequency of search operation
         * @param minSignalStrength
         *            Minimum signal strength, default =
         *            {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
         * @param scanDirection
         *            the direction to search in, it can only be either
         *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_DOWN}.
         * @param scanMethod
         *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_FAST},
         * @param multi_channel
         *            Is multiple channels are required, or only find next valid channel(seek).
         * @param rdsType
         *            the type of RDS condition to scan for.
         * @param rdsTypeValue
         *            the condition value to match.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
         *         code.
         *
         * @see IFmReceiverEventHandler.onSeekCompleteEvent().
         */
        public synchronized int seekStationCombo (int startFrequency, int endFrequency,
                int minSignalStrength, int scanDirection,
                int scanMethod, boolean multi_channel, int rdsType, int rdsTypeValue) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.seekStationCombo (startFrequency, endFrequency,
                minSignalStrength, scanDirection, scanMethod, multi_channel,
                        rdsType, rdsTypeValue);
        }

        /**
         * Enables/disables RDS/RDBS feature and AF algorithm. Will result in an
         * RDS mode event callback.
         * <p>
         *
         * @param rdsMode
         *            Turns on the RDS or RBDS. See
         *            {@link FmProxy#RDS_MODE_OFF},
         *            {@link FmProxy#RDS_MODE_DEFAULT_ON},
         *            {@link FmProxy#RDS_MODE_RDS_ON},
         *            {@link FmProxy#RDS_MODE_RDBS_ON}
         * @param rdsFields
         *            the mask specifying which types of RDS data to signal
         *            back.
         * @param afmode
         *            enables AF algorithm if True. Disables it if False
         * @param afThreshold
         *            the RSSI threshold when the AF should jump frequencies.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setRdsMode(int rdsMode, int rdsFeatures, int afMode,
                    int afThreshold) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setRdsMode(rdsMode, rdsFeatures, afMode, afThreshold);
        }

        /**
         * Configures FM audio mode to be mono, stereo or blend. Will result in
         * an audio mode event callback.
         *
         * @param audioMode
         *            the audio mode such as stereo or mono. The following
         *            values should be used {@link FmProxy#AUDIO_MODE_AUTO},
         *            {@link FmProxy#AUDIO_MODE_STEREO},
         *            {@link FmProxy#AUDIO_MODE_MONO} or
         *            {@link FmProxy#AUDIO_MODE_BLEND}.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setAudioMode(int audioMode) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setAudioMode(audioMode);
        }

        /**
         * Configures FM audio path to AUDIO_PATH_NONE, AUDIO_PATH_SPEAKER,
         * AUDIO_PATH_WIRED_HEADSET or AUDIO_PATH_DIGITAL. Will result in an
         * audio path event callback.
         *
         * @param audioPath
         *            the audio path such as AUDIO_PATH_NONE,
         *            AUDIO_PATH_SPEAKER, AUDIO_PATH_WIRED_HEADSET or
         *            AUDIO_PATH_DIGITAL. The following values should be used
         *            {@link FmProxy#AUDIO_PATH_NONE},
         *            {@link FmProxy#AUDIO_PATH_SPEAKER},
         *            {@link FmProxy#AUDIO_PATH_WIRED_HEADSET} or
         *            {@link FmProxy#AUDIO_PATH_DIGITAL}.
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setAudioPath(int audioPath) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setAudioPath(audioPath);
        }

        /**
         * Sets the minimum frequency step size to use when scanning for
         * stations. This function does not result in a status callback and the
         * calling application should therefore keep track of this setting.
         *
         * @param stepSize
         *            a frequency interval set to
         *            {@link FmProxy#FREQ_STEP_100KHZ} or
         *            {@link FmProxy#FREQ_STEP_50KHZ}.
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setStepSize(int stepSize) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setStepSize(stepSize);
        }

        /**
         * Sets the volume to use.
         *
         * @param volume
         *            range from 0 to 0x100
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setFMVolume(int volume) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setFMVolume(volume);
        }

        /**
         * Sets the SNR threshold value to use.
         *
         * @param snrThreshold  The SNR threshold value
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error code.
         */
        public synchronized int setSnrThreshold(int snrThreshold) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setSnrThreshold(snrThreshold);
        }

        /**
         * Sets a the world frequency region and deemphasis time. This results
         * in a world region event callback.
         *
         * @param worldRegion
         *            the world region the FM receiver is located. Set to
         *            {@link FmProxy#FUNC_REGION_NA},
         *            {@link FmProxy#FUNC_REGION_EUR} or
         *            {@link FmProxy#FUNC_REGION_JP}.
         * @param deemphasisTime
         *            the deemphasis time that can be set to either
         *            {@link FmProxy#DEEMPHASIS_50U} or
         *            {@link FmProxy#DEEMPHASIS_75U}.
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setWorldRegion(int worldRegion, int deemphasisTime) {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setWorldRegion(worldRegion, deemphasisTime);
        }

        /**
         * Estimates the current frequencies noise floor level. Generates an
         * Estimated NFL Event when complete. The returned NFL value can be used
         * to determine which minimum signal strength to use seeking stations.
         *
         * @param estimatedNoiseFloorLevel
         *            Estimate noise floor to {@link FmProxy#NFL_LOW},
         *            {@link FmProxy#NFL_MED} or {@link FmRecei
         *            -vver#NFL_FINE}.
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int estimateNoiseFloorLevel(int nflLevel) throws RemoteException {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.estimateNoiseFloorLevel(nflLevel);
        }

        /**
         * Sets the live audio polling function that can provide RSSI data on
         * the currently tuned frequency at specified intervals.
         *
         * @param liveAudioPolling
         *            enable/disable live audio data quality updating.
         * @param signalPollInterval
         *            time between RSSI signal polling in milliseconds.
         *
         * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero
         *         error code.
         */
        public synchronized int setLiveAudioPolling(boolean liveAudioPolling,
                int signalPollInterval)
                throws RemoteException {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.setLiveAudioPolling(liveAudioPolling, signalPollInterval);
        }

        /**
         * Returns the FM Audio Mode state.
         * @param none
         * @return {@link FmProxy#AUDIO_MODE_AUTO},
         *            {@link FmProxy#AUDIO_MODE_STEREO},
         *            {@link FmProxy#AUDIO_MODE_MONO} or
         *            {@link FmProxy#AUDIO_MODE_BLEND}.
         */
        public synchronized int getMonoStereoMode() throws RemoteException {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return  mSvc.mSvcHandler.getMonoStereoMode();
        }

        /**
         * Returns the present tuned FM Frequency
         * @param none
         * @return Tuned frequency
         */
        public synchronized int getTunedFrequency()  throws RemoteException {
            if (mSvc == null) {
                return FmProxy.STATUS_SERVER_FAIL;
            }
            return mSvc.mSvcHandler.getTunedFrequency();
        }

        /**
         * Returns whether MUTE is turned ON or OFF
         * @param none
         * @return false if MUTE is OFF ; true otherwise
         */
        public synchronized  boolean getIsMute()  throws RemoteException {
            if (mSvc == null) {
                return false;
            }
            return mSvc.mSvcHandler.getIsMute();
        }

        public void init() throws RemoteException {

        }
        public boolean getRadioIsOn() throws RemoteException {
            if (mSvc == null) {
                return false;
            }
            return mSvc.mSvcHandler.getRadioIsOn();
        }

    }

}
