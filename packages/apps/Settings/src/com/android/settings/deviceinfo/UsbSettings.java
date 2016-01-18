/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.settings.deviceinfo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.UserManager;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.os.SystemProperties;
import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.Utils;
import android.os.ServiceManager;
import android.os.IBinder;
import android.os.storage.IMountService;

/**
 * USB storage settings.
 */
public class UsbSettings extends SettingsPreferenceFragment {

    private static final String TAG = "UsbSettings";

    private static final String KEY_MTP = "usb_mtp";
    private static final String KEY_PTP = "usb_ptp";
	private static final String KEY_MASS = "usb_mass";

    private UsbManager mUsbManager;
    private CheckBoxPreference mMtp;
    private CheckBoxPreference mPtp;
    private boolean mUsbAccessoryMode;
    private CheckBoxPreference mMass;
    private String BuildWithUMS ="";
	private IMountService mMountService;
    private final BroadcastReceiver mStateReceiver = new BroadcastReceiver() {
        public void onReceive(Context content, Intent intent) {
            String action = intent.getAction();
            if (action.equals(UsbManager.ACTION_USB_STATE)) {
               mUsbAccessoryMode = intent.getBooleanExtra(UsbManager.USB_FUNCTION_ACCESSORY, false);
               Log.e(TAG, "UsbAccessoryMode " + mUsbAccessoryMode);
            }
            updateToggles(mUsbManager.getDefaultFunction());
        }
    };

    private PreferenceScreen createPreferenceHierarchy() {
        PreferenceScreen root = getPreferenceScreen();
        if (root != null) {
            root.removeAll();
        }
        addPreferencesFromResource(R.xml.usb_settings);
        root = getPreferenceScreen();

        mMtp = (CheckBoxPreference)root.findPreference(KEY_MTP);
        mPtp = (CheckBoxPreference)root.findPreference(KEY_PTP);

        UserManager um = (UserManager) getActivity().getSystemService(Context.USER_SERVICE);
        if (um.hasUserRestriction(UserManager.DISALLOW_USB_FILE_TRANSFER)) {
            mMtp.setEnabled(false);
            mPtp.setEnabled(false);
        }

        mMass= (CheckBoxPreference)root.findPreference(KEY_MASS);
	BuildWithUMS = SystemProperties.get("ro.factory.hasUMS", "false");
	if(("true".equals(BuildWithUMS))&&(mMountService!=null))
	 {
                   try
		   {
			    String flashPratition = System.getenv("EXTERNAL_STORAGE_FLASH");
			    String flashState =mMountService.getVolumeState(flashPratition);
			    if(!"mounted".equals(flashState) && !"shared".equals(flashState))
			    {
				    //BuildWithUMS = "false";
				    Log.d(TAG,"boardconfig set enable UMS,but flash partition is not mounted,disable UMS");
			    }
		    }
		    catch(Exception e)
		    {

		    }
				Log.e(TAG, "Can't get mount service");
	    }
	 if(!"true".equals(BuildWithUMS)){
		root.removePreference(root.findPreference(KEY_MASS));
         }
        return root;
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mUsbManager = (UsbManager)getSystemService(Context.USB_SERVICE);
		if (mMountService == null) {
			IBinder service = ServiceManager.getService("mount");
			if (service != null) {
				mMountService = IMountService.Stub.asInterface(service);
			} else {
				Log.e(TAG, "Can't get mount service");
			}
		}
    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mStateReceiver);
    }

    @Override
    public void onResume() {
        super.onResume();

        // Make sure we reload the preference hierarchy since some of these settings
        // depend on others...
        createPreferenceHierarchy();

        // ACTION_USB_STATE is sticky so this will call updateToggles
        getActivity().registerReceiver(mStateReceiver,
                new IntentFilter(UsbManager.ACTION_USB_STATE));
    }

    private void updateToggles(String function) {
        Log.e(TAG,"BuildWithUMS +" + BuildWithUMS);
        if (UsbManager.USB_FUNCTION_MTP.equals(function)) {
            mMtp.setChecked(true);
            mPtp.setChecked(false);
	    if("true".equals(BuildWithUMS) )
	    	mMass.setChecked(false);
        } else if (UsbManager.USB_FUNCTION_PTP.equals(function)) {
            mMtp.setChecked(false);
            mPtp.setChecked(true);
	    if("true".equals(BuildWithUMS) ) 
	    	mMass.setChecked(false);
        }  else if (UsbManager.USB_FUNCTION_MASS_STORAGE.equals(function)) {
            mMtp.setChecked(false);
            mPtp.setChecked(false);
	    if("true".equals(BuildWithUMS) )
	    	mMass.setChecked(true);
        } else  {
            mMtp.setChecked(false);            
	    mPtp.setChecked(false);
	    if("true".equals(BuildWithUMS))
	    	mMass.setChecked(true);
        }
        UserManager um = (UserManager) getActivity().getSystemService(Context.USER_SERVICE);
        if (um.hasUserRestriction(UserManager.DISALLOW_USB_FILE_TRANSFER)) {
            Log.e(TAG, "USB is locked down");
            mMtp.setEnabled(false);
            mPtp.setEnabled(false);
        } else if (!mUsbAccessoryMode) {
            //Enable MTP and PTP switch while USB is not in Accessory Mode, otherwise disable it
            Log.e(TAG, "USB Normal Mode");
            mMtp.setEnabled(true);
            mPtp.setEnabled(true);
        } else {
            Log.e(TAG, "USB Accessory Mode");
            mMtp.setEnabled(false);
            mPtp.setEnabled(false);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        // Don't allow any changes to take effect as the USB host will be disconnected, killing
        // the monkeys
        if (Utils.isMonkeyRunning()) {
            return true;
        }
        // If this user is disallowed from using USB, don't handle their attempts to change the
        // setting.
        UserManager um = (UserManager) getActivity().getSystemService(Context.USER_SERVICE);
        if (um.hasUserRestriction(UserManager.DISALLOW_USB_FILE_TRANSFER)) {
            return true;
        }

        String function = "none";
        if (preference == mMtp && mMtp.isChecked()) {
            function = UsbManager.USB_FUNCTION_MTP;
        } else if (preference == mPtp && mPtp.isChecked()) {
            function = UsbManager.USB_FUNCTION_PTP;
        } else if (preference == mMass) {
            function = UsbManager.USB_FUNCTION_MASS_STORAGE;
        }

        mUsbManager.setCurrentFunction(function, true);
        updateToggles(function);

        return true;
    }
}
