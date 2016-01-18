/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.settings;

import com.android.internal.view.RotationPolicy;
import com.android.settings.notification.DropDownPreference;
import com.android.settings.notification.DropDownPreference.Callback;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;

import static android.provider.Settings.Secure.DOZE_ENABLED;
import static android.provider.Settings.Secure.WAKE_GESTURE_ENABLED;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;

import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.Dialog;
import android.app.admin.DevicePolicyManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import android.preference.CheckBoxPreference;
import java.util.ArrayList;
import java.io.File;
//$_rbox_$_modify_$_by aisx
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.DisplayOutputManager;
//$_rbox_$_modify_$_end
import java.util.List;
import android.os.DisplayOutputManager;
import android.os.UserHandle;
import android.database.ContentObserver;

import android.view.IWindowManager;
import android.view.WindowManagerGlobal;
import android.content.Intent;


public class DisplaySettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener, OnPreferenceClickListener, Indexable {
    private static final String TAG = "DisplaySettings";

    /** If there is no setting in the provider, use this. */
    private static final int FALLBACK_SCREEN_TIMEOUT_VALUE = 30000;
    /** If there is no setting in the provider, use this. */
    private static final int FALLBACK_BUTTON_LIGHTS_TIMEOUT_VALUE = 1500;

    private static final String KEY_SYSTEMBAR_HIDE = "systembar_hide";
    private static final String KEY_SCREEN_ORIENTATION = "screen_orientation";

    private static final String KEY_SCREEN_TIMEOUT = "screen_timeout";
    private static final String KEY_BUTTON_LIGHTS_ENABLE = "button_lights_enable";
    private static final String KEY_BUTTON_LIGHTS_TIMEOUT = "button_lights_timeout";
    private static final String KEY_FONT_SIZE = "font_size";
    private static final String KEY_SCREEN_SAVER = "screensaver";
    private static final String KEY_LIFT_TO_WAKE = "lift_to_wake";
    private static final String KEY_DOZE = "doze";
    private static final String KEY_AUTO_BRIGHTNESS = "auto_brightness";
    private static final String KEY_AUTO_ROTATE = "auto_rotate";

    private static final int DLG_GLOBAL_CHANGE_WARNING = 1;

    private static final String KEYBOARD_BACKLIGHTS_FILE = "/sys/class/leds/keyboard-backlight/brightness";

    private CheckBoxPreference mSystemBarHide;
    private CheckBoxPreference mAccelerometer;
    private WarnedListPreference mFontSizePref;
    private CheckBoxPreference mNotificationPulse;

    private final Configuration mCurConfig = new Configuration();

    private ListPreference mScreenOrientationPreference;
    private ListPreference mScreenTimeoutPreference;
    private Preference mScreenSaverPreference;
	private final boolean DBG = true;
	private static final String KEY_MAIN_DISPLAY_INTERFACE = "main_screen_interface";
    private static final String KEY_MAIN_DISPLAY_MODE = "main_screen_mode";
    private static final String KEY_AUX_DISPLAY_INTERFACE = "aux_screen_interface";
    private static final String KEY_AUX_DISPLAY_MODE = "aux_screen_mode";
    private static final String KEY_SCREENCALE = "screenscale";
    private ListPreference	mMainDisplay;
    private SwitchPreference mLiftToWakePreference;
    private SwitchPreference mDozePreference;
    private SwitchPreference mAutoBrightnessPreference;
    private SwitchPreference mButtonLightsPreference;
    private ListPreference mButtonLightsTimeoutPreference;
    private ListPreference	mMainModeList;
	private ListPreference	mAuxDisplay;
    private ListPreference	mAuxModeList;
	private DisplayOutputManager mDisplayManagement = null;
    private int mMainDisplay_last = -1;
    private int mMainDisplay_set = -1;
    private String mMainMode_last = null;
    private String mMainMode_set = null;
    private int mAuxDisplay_last = -1;
    private int mAuxDisplay_set = -1;
    private String mAuxMode_last = null;
    private String mAuxMode_set = null;
    private static final int DIALOG_ID_RECOVER = 2;
    private AlertDialog mDialog;
    private static int mTime = -1;
    private Handler mHandler;
    private Runnable mRunnable;
	private SettingsObserver mSettingsObserver;
    private boolean mUserSet=false;
    boolean first=true;
    private boolean isTablet;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Activity activity = getActivity();
        final ContentResolver resolver = activity.getContentResolver();

        addPreferencesFromResource(R.xml.display_settings);
	//$_rbox_$_modify_by lly$_begin
        isTablet = "tablet".equals(SystemProperties.get("ro.target.product", "tablet"));
	//$_rbox_$_modify_by lly$_end
        
	mScreenSaverPreference = findPreference(KEY_SCREEN_SAVER);
        if (mScreenSaverPreference != null
                && getResources().getBoolean(
                        com.android.internal.R.bool.config_dreamsSupported) == false) {
            getPreferenceScreen().removePreference(mScreenSaverPreference);
        }

        mSystemBarHide = (CheckBoxPreference) findPreference(KEY_SYSTEMBAR_HIDE);

        mScreenOrientationPreference = (ListPreference) findPreference(KEY_SCREEN_ORIENTATION);
   		 mScreenOrientationPreference.setOnPreferenceChangeListener(this);

        mScreenTimeoutPreference = (ListPreference) findPreference(KEY_SCREEN_TIMEOUT);
        final long currentTimeout = Settings.System.getLong(resolver, SCREEN_OFF_TIMEOUT,
                FALLBACK_SCREEN_TIMEOUT_VALUE);
        mScreenTimeoutPreference.setValue(String.valueOf(currentTimeout));
        mScreenTimeoutPreference.setOnPreferenceChangeListener(this);
        disableUnusableTimeouts(mScreenTimeoutPreference);
        updateTimeoutPreferenceDescription(currentTimeout);

        mFontSizePref = (WarnedListPreference) findPreference(KEY_FONT_SIZE);
        mFontSizePref.setOnPreferenceChangeListener(this);
        mFontSizePref.setOnPreferenceClickListener(this);

        if (isButtonLightsAvailable()) {
            mButtonLightsPreference = (SwitchPreference) findPreference(KEY_BUTTON_LIGHTS_ENABLE);
            mButtonLightsPreference.setOnPreferenceChangeListener(this);
            mButtonLightsTimeoutPreference = (ListPreference) findPreference(KEY_BUTTON_LIGHTS_TIMEOUT);
            mButtonLightsTimeoutPreference.setOnPreferenceChangeListener(this);
            final int currentButtonLightsTimeout = Settings.System.getInt(resolver, Settings.System.BUTTON_LIGHTS_OFF_TIMEOUT,
                FALLBACK_BUTTON_LIGHTS_TIMEOUT_VALUE);
            mButtonLightsTimeoutPreference.setValue(String.valueOf(currentButtonLightsTimeout));
            updateButtonLightsTimeoutPreferenceDescription(currentButtonLightsTimeout);
        } else {
            removePreference(KEY_BUTTON_LIGHTS_ENABLE);
            removePreference(KEY_BUTTON_LIGHTS_TIMEOUT);
        }

        if (isAutomaticBrightnessAvailable(getResources())) {
            mAutoBrightnessPreference = (SwitchPreference) findPreference(KEY_AUTO_BRIGHTNESS);
            mAutoBrightnessPreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_AUTO_BRIGHTNESS);
        }

        if (isLiftToWakeAvailable(activity)) {
            mLiftToWakePreference = (SwitchPreference) findPreference(KEY_LIFT_TO_WAKE);
            mLiftToWakePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_LIFT_TO_WAKE);
        }

        if (isDozeAvailable(activity)) {
            mDozePreference = (SwitchPreference) findPreference(KEY_DOZE);
            mDozePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_DOZE);
        }

        if (RotationPolicy.isRotationLockToggleVisible(activity)) {
			mSettingsObserver=new SettingsObserver(mHandler);
			mSettingsObserver.observe();
            DropDownPreference rotatePreference =
                    (DropDownPreference) findPreference(KEY_AUTO_ROTATE);
            rotatePreference.addItem(activity.getString(R.string.display_auto_rotate_rotate),
                    false);
            int rotateLockedResourceId;
            // The following block sets the string used when rotation is locked.
            // If the device locks specifically to portrait or landscape (rather than current
            // rotation), then we use a different string to include this information.
            if (allowAllRotations(activity)) {
                rotateLockedResourceId = R.string.display_auto_rotate_stay_in_current;
            } else {
                if (RotationPolicy.getRotationLockOrientation(activity)
                        == Configuration.ORIENTATION_PORTRAIT) {
                    rotateLockedResourceId =
                            R.string.display_auto_rotate_stay_in_portrait;
                } else {
                    rotateLockedResourceId =
                            R.string.display_auto_rotate_stay_in_landscape;
                }
            }
            rotatePreference.addItem(activity.getString(rotateLockedResourceId), true);
            rotatePreference.setSelectedItem(RotationPolicy.isRotationLocked(activity) ?
                    1 : 0);
            rotatePreference.setCallback(new Callback() {
                @Override
				public boolean onItemSelected(int pos, Object value) {

                    if (first){
                        first=false;
                    }
                    else
                    {
                        mUserSet=true;
                    }
					RotationPolicy.setRotationLock(activity, (Boolean) value);
					return true;
				}
			});
        } else {
            removePreference(KEY_AUTO_ROTATE);
        }
		//$_rbox_$_modify_$_begin
		mMainDisplay = (ListPreference) findPreference(KEY_MAIN_DISPLAY_INTERFACE);
		mMainModeList = (ListPreference) findPreference(KEY_MAIN_DISPLAY_MODE);
		mAuxDisplay = (ListPreference) findPreference(KEY_AUX_DISPLAY_INTERFACE);
		mAuxModeList = (ListPreference) findPreference(KEY_AUX_DISPLAY_MODE);
		if(isTablet){
                   removePreference(KEY_MAIN_DISPLAY_INTERFACE);
                   removePreference(KEY_MAIN_DISPLAY_MODE);
                   removePreference(KEY_AUX_DISPLAY_INTERFACE);
                   removePreference(KEY_AUX_DISPLAY_MODE);
                   removePreference(KEY_SCREENCALE);
		   return;
                }
		try {
			mDisplayManagement = new DisplayOutputManager();
		}catch (RemoteException doe) {
			
		}
		
		int[] main_display = mDisplayManagement.getIfaceList(mDisplayManagement.MAIN_DISPLAY);
		if(main_display == null)	{
			Log.e(TAG, "Can not get main display interface list");
			return;
		}
		int[] aux_display = mDisplayManagement.getIfaceList(mDisplayManagement.AUX_DISPLAY);

		mMainDisplay.setOnPreferenceChangeListener(this);
		mMainModeList.setOnPreferenceChangeListener(this);
		
		int curIface = mDisplayManagement.getCurrentInterface(mDisplayManagement.MAIN_DISPLAY);
		mMainDisplay_last = curIface;
		
		if (aux_display == null) {
			mMainDisplay.setTitle(getString(R.string.screen_interface));
		} else {
			mMainDisplay.setTitle("1st " + getString(R.string.screen_interface));
		}

		// Fill main interface list.
		CharSequence[] IfaceEntries = new CharSequence[main_display.length];
		CharSequence[] IfaceValue = new CharSequence[main_display.length];		
		for(int i = 0; i < main_display.length; i++) {
			IfaceEntries[i] = getIfaceTitle(main_display[i]);
			IfaceValue[i] = Integer.toString(main_display[i]);
		}
		mMainDisplay.setEntries(IfaceEntries);
        mMainDisplay.setEntryValues(IfaceValue);
        mMainDisplay.setValue(Integer.toString(curIface));
		
		// Fill main display mode list.
		mMainModeList.setTitle(getIfaceTitle(curIface) + " " + getString(R.string.screen_mode_title));
		SetModeList(mDisplayManagement.MAIN_DISPLAY, curIface);
		String mode = mDisplayManagement.getCurrentMode(mDisplayManagement.MAIN_DISPLAY, curIface);

		if (savedInstanceState != null){
			String saved_mode_last = savedInstanceState.getString("main_mode_last", null);
			String saved_mode_set = savedInstanceState.getString("main_mode_set", null);
			if (DBG) Log.d(TAG,"get savedInstanceState mainmodelast="+saved_mode_last
					+",mainmodeset="+saved_mode_set);
			if (saved_mode_last != null && saved_mode_set != null) {
				mMainModeList.setValue(saved_mode_last);
				mMainMode_last = saved_mode_last;
				mMainDisplay_set = mMainDisplay_last;
				mMainMode_set = saved_mode_set;
			}
		} else if(mode != null) {
				mMainModeList.setValue(mode);
				mMainMode_last = mode;
				mMainDisplay_set = mMainDisplay_last;
				mMainMode_set = mMainMode_last;
		}

		
		// Get Aux screen infomation
		mAuxDisplay.setOnPreferenceChangeListener(this);
		mAuxModeList = (ListPreference) findPreference(KEY_AUX_DISPLAY_MODE);
		mAuxModeList.setOnPreferenceChangeListener(this);
		if(aux_display != null) {
			curIface = mDisplayManagement.getCurrentInterface(mDisplayManagement.AUX_DISPLAY);
			mAuxDisplay_last = curIface;
			mAuxDisplay.setTitle("2nd " + getString(R.string.screen_interface));
			// Fill aux interface list.
			IfaceEntries = new CharSequence[aux_display.length];
			IfaceValue = new CharSequence[aux_display.length];		
			for(int i = 0; i < aux_display.length; i++) {
				IfaceEntries[i] = getIfaceTitle(aux_display[i]);
				IfaceValue[i] = Integer.toString(aux_display[i]);
			}
			mAuxDisplay.setEntries(IfaceEntries);
			mAuxDisplay.setEntryValues(IfaceValue);
			mAuxDisplay.setValue(Integer.toString(curIface));
			
			// Fill aux display mode list.
			mAuxModeList.setTitle(getIfaceTitle(curIface) + " " + getString(R.string.screen_mode_title));
			SetModeList(mDisplayManagement.AUX_DISPLAY, curIface);
			mode = mDisplayManagement.getCurrentMode(mDisplayManagement.AUX_DISPLAY, curIface);
		if (savedInstanceState != null){
			String saved_mode_last = savedInstanceState.getString("aux_mode_last", null);
			String saved_mode_set = savedInstanceState.getString("aux_mode_set", null);
			if (DBG) Log.d(TAG,"get savedInstanceState auxmodelast="+saved_mode_last
					+",auxmodeset="+saved_mode_set);
			if (saved_mode_last != null && saved_mode_set != null) {
				mAuxModeList.setValue(saved_mode_last);
				mAuxMode_last = saved_mode_last;
				mAuxDisplay_set = mAuxDisplay_last;
				mAuxMode_set = saved_mode_set;
			}
		}
			if(mode != null) {
				mAuxModeList.setValue(mode);
				mAuxMode_last = mode;
				mAuxDisplay_set = mAuxDisplay_last;
				mAuxMode_set = mAuxMode_last;
			}
		} else {
			mAuxDisplay.setShouldDisableView(true);
			mAuxDisplay.setEnabled(false);
			getPreferenceScreen().removePreference(mAuxDisplay);
			mAuxModeList.setShouldDisableView(true);
			mAuxModeList.setEnabled(false); 	
			getPreferenceScreen().removePreference(mAuxModeList);
		}
		
    		mHandler = new Handler();
    		
    		mRunnable = new Runnable(){
    			@Override
    			public void run() {
    				// TODO Auto-generated method stub
    			   if(mDialog == null || mTime < 0)
    				   return;
    			   if(mTime > 0) {
    				   mTime--;
				       if(isAdded()) {
    			           CharSequence text = getString(R.string.screen_control_ok_title) + " (" + String.valueOf(mTime) + ")";
    				       mDialog.getButton(DialogInterface.BUTTON_POSITIVE).setText(text);
					   }
    				   mHandler.postDelayed(this, 1000);
    			   }  else {
    				   //Restore display setting.
    				   RestoreDisplaySetting();
				       removeDialog(DIALOG_ID_RECOVER);
				       mDialog = null;
    			   }
    			}
    		};		
		
    }
	
    
    public void Resume(){
		Log.d(TAG,"resume fill interface and mode");

		int curIface = 0;
		String mode = null;
		
		// Fill main interface list.
		int[] mainFace = mDisplayManagement.getIfaceList(mDisplayManagement.MAIN_DISPLAY);
		if(mainFace != null) {			
	        // get current main iface
	        curIface = mDisplayManagement.getCurrentInterface(mDisplayManagement.MAIN_DISPLAY);
			mMainDisplay_last = curIface;
			
	        String curInterface = getIfaceTitle(curIface);
	        Log.d(TAG,"cur interface:"+curInterface);
	        
	        // Fill main display mode list.
	     	SetModeList(mDisplayManagement.MAIN_DISPLAY,curIface);
	     	mMainModeList.setTitle(getIfaceTitle(curIface) + " " + getString(R.string.screen_mode_title));
			mode = mDisplayManagement.getCurrentMode(mDisplayManagement.MAIN_DISPLAY, curIface);
			Log.d(TAG,"cur mode = " + mode);
			if(mode != null) {
				mMainMode_last = mode;
				mMainDisplay_set = mMainDisplay_last;
				mMainMode_set = mMainMode_last;
				mMainModeList.setValue(mode);
	     	}		
		}

		// Fill aux interface list.
		int[] aux_display = mDisplayManagement.getIfaceList(mDisplayManagement.AUX_DISPLAY);
		if(aux_display != null) {
			// get current aux iface
			curIface = mDisplayManagement.getCurrentInterface(mDisplayManagement.AUX_DISPLAY);
			mAuxDisplay_last = curIface;
			
	        String curInterface = getIfaceTitle(curIface);
	        Log.d(TAG,"cur interface:"+curInterface);

			// Fill aux display mode list.
	        mAuxModeList.setTitle(getIfaceTitle(curIface) + " " + getString(R.string.screen_mode_title));
			SetModeList(mDisplayManagement.AUX_DISPLAY, curIface);
			mode = mDisplayManagement.getCurrentMode(mDisplayManagement.AUX_DISPLAY, curIface);
			if(mode != null) {
				mAuxMode_last = mode;
				mAuxDisplay_set = mAuxDisplay_last;
				mAuxMode_set = mAuxMode_last;
				mAuxModeList.setValue(mode);
			}
		} 
	}


	@Override
	public void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
		if(DBG) Log.d(TAG,"onStop()");
                if(!isTablet && mHandler != null && mRunnable != null)
		   mHandler.removeCallbacks(mRunnable);
	}
	
	@Override
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		//getActivity().unregisterReceiver(mHdmiReceiver);
	}

	@Override
	public void onSaveInstanceState(Bundle outState) {
		// TODO Auto-generated method stub
		if(DBG) Log.d(TAG, "store onSaveInstanceState mainmodelast="+mMainMode_last
				+",mainmodeset="+mMainMode_set+",auxmodelast="+mAuxMode_last
				+",auxmodeset="+mAuxMode_set);
		super.onSaveInstanceState(outState);
		outState.putString("main_mode_last", mMainMode_last);
		outState.putString("main_mode_set", mMainMode_set);
		outState.putString("aux_mode_last", mAuxMode_last);
		outState.putString("aux_mode_set", mAuxMode_set);
	}
    

   
    @Override
    public void onDialogShowing() {
        // override in subclass to attach a dismiss listener, for instance
		if (mDialog != null)
		{
    	    mDialog.getButton(DialogInterface.BUTTON_NEGATIVE).requestFocus();
    	    CharSequence text = getString(R.string.screen_control_ok_title) + " (" + String.valueOf(mTime) + ")";
    	    mDialog.getButton(DialogInterface.BUTTON_POSITIVE).setText(text);
    	    mHandler.postDelayed(mRunnable, 1000);
		}

    }

   	private String getIfaceTitle(int iface) {
        	String ifaceTitle = null;
        	if(iface == mDisplayManagement.DISPLAY_IFACE_LCD)
        		ifaceTitle =  getString(R.string.screen_iface_lcd_title);
        	if(iface == mDisplayManagement.DISPLAY_IFACE_HDMI)
        		ifaceTitle =  getString(R.string.screen_iface_hdmi_title);
    		else if(iface == mDisplayManagement.DISPLAY_IFACE_VGA)
    			ifaceTitle = getString(R.string.screen_iface_vga_title);
    		else if(iface == mDisplayManagement.DISPLAY_IFACE_YPbPr)
    			ifaceTitle = getString(R.string.screen_iface_ypbpr_title);
    		else if(iface == mDisplayManagement.DISPLAY_IFACE_TV)
    			ifaceTitle = getString(R.string.screen_iface_tv_title);
        	
        	return ifaceTitle;
        }

    	private void SetModeList(int display, int iface) {
    		
    		if(DBG) Log.d(TAG, "SetModeList display " + display + " iface " + iface);
    		
        	String[] modelist = mDisplayManagement.getModeList(display, iface);
    		CharSequence[] ModeEntries = new CharSequence[modelist.length];
    		CharSequence[] ModeEntryValues = new CharSequence[modelist.length];
    		for(int i = 0; i < modelist.length; i++) {
    			ModeEntries[i] = modelist[i];
    			if(iface == mDisplayManagement.DISPLAY_IFACE_TV) {
    				String mode = modelist[i];
    				if(mode.equals("720x576i-50")) {
    					ModeEntries[i] = "CVBS: PAL";
    				} else if(mode.equals("720x480i-60")) {
    					ModeEntries[i] = "CVBS: NTSC";
    				} else
    					ModeEntries[i] = "YPbPr: " + modelist[i];
    			}
    				
    			ModeEntryValues[i] = modelist[i];
    		}
    		if(display == mDisplayManagement.MAIN_DISPLAY) {
    			mMainModeList.setEntries(ModeEntries);
    			mMainModeList.setEntryValues(ModeEntryValues);
    		} else {
    			mAuxModeList.setEntries(ModeEntries);
    			mAuxModeList.setEntryValues(ModeEntryValues);
    		}
        }

    	private void RestoreDisplaySetting() {
    		if( (mMainDisplay_set != mMainDisplay_last) || (mMainMode_last.equals(mMainMode_set) == false) ) {
    			if(mMainDisplay_set != mMainDisplay_last) {
    				mDisplayManagement.setInterface(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_set, false);
    				mMainDisplay.setValue(Integer.toString(mMainDisplay_last));
    				mMainModeList.setTitle(getIfaceTitle(mMainDisplay_last) + " " + getString(R.string.screen_mode_title));
    				// Fill display mode list.
    		     	SetModeList(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_last);
    			}
    			mMainModeList.setValue(mMainMode_last);
    			mDisplayManagement.setMode(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_last, mMainMode_last);
    			mDisplayManagement.setInterface(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_last, true);
    			mMainDisplay_set = mMainDisplay_last;
    			mMainMode_set = mMainMode_last;
    		}
    		if(mDisplayManagement.getDisplayNumber() > 1) {
    			if( (mAuxDisplay_set != mAuxDisplay_last) || (mAuxMode_last.equals(mAuxMode_set) == false) ) {
    				if(mAuxDisplay_set != mAuxDisplay_last) {
    					mDisplayManagement.setInterface(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_set, false);
    					mAuxDisplay.setValue(Integer.toString(mAuxDisplay_last));
    					mAuxModeList.setTitle(getIfaceTitle(mAuxDisplay_last) + " " + getString(R.string.screen_mode_title));
    					// Fill display mode list.
    			     	SetModeList(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_last);
    				}
    				mAuxModeList.setValue(mAuxMode_last);
    				mDisplayManagement.setMode(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_last, mAuxMode_last);
    				mDisplayManagement.setInterface(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_last, true);
    				mAuxDisplay_set = mAuxDisplay_last;
    				mAuxMode_set = mAuxMode_last;
    			}
    		}
    	}        
        //$_rbox_$_modify_$_end    
       private static boolean allowAllRotations(Context context) {
        return Resources.getSystem().getBoolean(
                com.android.internal.R.bool.config_allowAllRotations);
    }

    private static boolean isLiftToWakeAvailable(Context context) {
        SensorManager sensors = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        return sensors != null && sensors.getDefaultSensor(Sensor.TYPE_WAKE_GESTURE) != null;
    }

    private static boolean isDozeAvailable(Context context) {
        String name = Build.IS_DEBUGGABLE ? SystemProperties.get("debug.doze.component") : null;
        if (TextUtils.isEmpty(name)) {
            name = context.getResources().getString(
                    com.android.internal.R.string.config_dozeComponent);
        }
        return !TextUtils.isEmpty(name);
    }

    private static boolean isAutomaticBrightnessAvailable(Resources res) {
        return res.getBoolean(com.android.internal.R.bool.config_automatic_brightness_available);
    }

    private static boolean isButtonLightsAvailable() {
        File file = new File(KEYBOARD_BACKLIGHTS_FILE); 
        if (file.exists())
            return true;
        return false;
    }

    private void updateButtonLightsTimeoutPreferenceDescription(int currentTimeout) {
        ListPreference preference = mButtonLightsTimeoutPreference;
        String summary;
        if (currentTimeout < 0) {
            summary = preference.getContext().getString(R.string.button_lights_timeout_never);
        } else {
            final CharSequence[] entries = preference.getEntries();
            final CharSequence[] values = preference.getEntryValues();
            if (entries == null || entries.length == 0) {
                summary = "";
            } else {
                int best = 0;
                for (int i = 0; i < values.length; i++) {
                    long timeout = Long.parseLong(values[i].toString());
                    if (currentTimeout >= timeout && timeout > 0) {
                        best = i;
                    }
                }
                summary = preference.getContext().getString(R.string.button_lights_timeout_summary,
                        entries[best]);
            }
        }
        preference.setSummary(summary);
    }

    private void updateScreenOrientation(){
       int rotation = 1;
               try {
                               IWindowManager wm = WindowManagerGlobal.getWindowManagerService();
                               rotation = wm.getRotation();
                       } catch (RemoteException exc) {
               }
               mScreenOrientationPreference.setValue(String.valueOf(rotation));
               setScreenOrientationSummary(rotation);
    }
    private void setScreenOrientationSummary(int value)
    {
        switch(value)
        {
            case 1:
                mScreenOrientationPreference.setSummary(R.string.m_vertical_screen);
            break;
            case 0:
                mScreenOrientationPreference.setSummary(R.string.m_horizontal_screen);
            break;
            case 3:
                mScreenOrientationPreference.setSummary(R.string.m_reverse_vertical);
            break;
            case 2:
                mScreenOrientationPreference.setSummary(R.string.m_reverse_horizontal);
            break;
        }
    }


    private void updateTimeoutPreferenceDescription(long currentTimeout) {
        ListPreference preference = mScreenTimeoutPreference;
        String summary;
        if (currentTimeout < 0) {
            // Unsupported value
            summary = "";
        } else {
            final CharSequence[] entries = preference.getEntries();
            final CharSequence[] values = preference.getEntryValues();
            if (entries == null || entries.length == 0) {
                summary = "";
            } else {
                int best = 0;
                for (int i = 0; i < values.length; i++) {
                    long timeout = Long.parseLong(values[i].toString());
                    if (currentTimeout >= timeout) {
                        best = i;
                    }
                }
				if(best+1==values.length){
					summary = preference.getContext().getString(R.string.screen_never_timeout_summary,
							entries[best]);
				}else{
					summary = preference.getContext().getString(R.string.screen_timeout_summary,
							entries[best]);
				}
            }
        }
        preference.setSummary(summary);
    }

    private void disableUnusableTimeouts(ListPreference screenTimeoutPreference) {
        final DevicePolicyManager dpm =
                (DevicePolicyManager) getActivity().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        final long maxTimeout = dpm != null ? dpm.getMaximumTimeToLock(null) : 0;
        if (maxTimeout == 0) {
            return; // policy not enforced
        }
        final CharSequence[] entries = screenTimeoutPreference.getEntries();
        final CharSequence[] values = screenTimeoutPreference.getEntryValues();
        ArrayList<CharSequence> revisedEntries = new ArrayList<CharSequence>();
        ArrayList<CharSequence> revisedValues = new ArrayList<CharSequence>();
        for (int i = 0; i < values.length; i++) {
            long timeout = Long.parseLong(values[i].toString());
            if (timeout <= maxTimeout) {
                revisedEntries.add(entries[i]);
                revisedValues.add(values[i]);
            }
        }
        if (revisedEntries.size() != entries.length || revisedValues.size() != values.length) {
            final int userPreference = Integer.parseInt(screenTimeoutPreference.getValue());
            screenTimeoutPreference.setEntries(
                    revisedEntries.toArray(new CharSequence[revisedEntries.size()]));
            screenTimeoutPreference.setEntryValues(
                    revisedValues.toArray(new CharSequence[revisedValues.size()]));
            if (userPreference <= maxTimeout) {
                screenTimeoutPreference.setValue(String.valueOf(userPreference));
            } else if (revisedValues.size() > 0
                    && Long.parseLong(revisedValues.get(revisedValues.size() - 1).toString())
                    == maxTimeout) {
                // If the last one happens to be the same as the max timeout, select that
                screenTimeoutPreference.setValue(String.valueOf(maxTimeout));
            } else {
                // There will be no highlighted selection since nothing in the list matches
                // maxTimeout. The user can still select anything less than maxTimeout.
                // TODO: maybe append maxTimeout to the list and mark selected.
            }
        }
        screenTimeoutPreference.setEnabled(revisedEntries.size() > 0);
    }

    int floatToIndex(float val) {
        String[] indices = getResources().getStringArray(R.array.entryvalues_font_size);
        float lastVal = Float.parseFloat(indices[0]);
        for (int i=1; i<indices.length; i++) {
            float thisVal = Float.parseFloat(indices[i]);
            if (val < (lastVal + (thisVal-lastVal)*.5f)) {
                return i-1;
            }
            lastVal = thisVal;
        }
        return indices.length-1;
    }

    public void readFontSizePreference(ListPreference pref) {
        try {
            mCurConfig.updateFrom(ActivityManagerNative.getDefault().getConfiguration());
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to retrieve font size");
        }

        // mark the appropriate item in the preferences list
        int index = floatToIndex(mCurConfig.fontScale);
        pref.setValueIndex(index);

        // report the current size in the summary text
        final Resources res = getResources();
        String[] fontSizeNames = res.getStringArray(R.array.entries_font_size);
        pref.setSummary(String.format(res.getString(R.string.summary_font_size),
                fontSizeNames[index]));
    }

    @Override
    public void onResume() {
        super.onResume();
	     updateScreenOrientation();
        updateState();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
               // TODO Auto-generated method stub
               super.onConfigurationChanged(newConfig);
                updateScreenOrientation();
       }
    @Override
    public Dialog onCreateDialog(int dialogId) {
        if (dialogId == DLG_GLOBAL_CHANGE_WARNING) {
            return Utils.buildGlobalChangeWarningDialog(getActivity(),
                    R.string.global_font_change_title,
                    new Runnable() {
                        public void run() {
                            mFontSizePref.click();
                        }
                    });
        }
		switch (dialogId) {
		case DIALOG_ID_RECOVER:
		mDialog = new AlertDialog.Builder(getActivity())
		        .setTitle(R.string.screen_mode_switch_title)
		        .setCancelable(false)
		        .setPositiveButton(R.string.screen_control_ok_title,
		                new DialogInterface.OnClickListener() {
		                    public void onClick(DialogInterface dialog, int which) {
		                        // Keep display setting
								mTime = -1;
								mDisplayManagement.saveConfig();
								mMainModeList.setValue(mMainMode_set);
								mMainDisplay_last = mMainDisplay_set;
								mMainMode_last = mMainMode_set;
								
								mAuxModeList.setValue(mAuxMode_set);
								mAuxDisplay_last = mAuxDisplay_set;
								mAuxMode_last = mAuxMode_set;
		                    }
		                })
		        .setNegativeButton(R.string.screen_control_cancel_title, 
		                new DialogInterface.OnClickListener() {
		                	public void onClick(DialogInterface dialog, int which) {
								//Restore display setting.
		                		dialog.dismiss();
								mTime = -1;
								mDialog = null;
								RestoreDisplaySetting();
			    			}
		                })
		        .create();
		mDialog.setOnShowListener(new DialogInterface.OnShowListener() {
			
			@Override
			public void onShow(DialogInterface dialog) {
				// TODO Auto-generated method stub
				if(DBG) Log.d(TAG,"show dialog");
				//onDialogShowed();
			}
		});
		 return mDialog;
		 
		}
        return null;
    }

    private void updateState() {
        readFontSizePreference(mFontSizePref);
        updateScreenSaverSummary();
        updateSystemBarHideCheckBox();

        // Update button lights enable if it is available.
        if (mButtonLightsPreference != null) {
            int enable = Settings.System.getInt(getContentResolver(),
                Settings.System.BUTTON_LIGHTS_ENABLED, 0);
            mButtonLightsPreference.setChecked(enable != 0);
        }

        // Update auto brightness if it is available.
        if (mAutoBrightnessPreference != null) {
            int brightnessMode = Settings.System.getInt(getContentResolver(),
                    SCREEN_BRIGHTNESS_MODE, SCREEN_BRIGHTNESS_MODE_MANUAL);
            mAutoBrightnessPreference.setChecked(brightnessMode != SCREEN_BRIGHTNESS_MODE_MANUAL);
        }

        // Update lift-to-wake if it is available.
        if (mLiftToWakePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), WAKE_GESTURE_ENABLED, 0);
            mLiftToWakePreference.setChecked(value != 0);
        }

        // Update doze if it is available.
        if (mDozePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), DOZE_ENABLED, 1);
            mDozePreference.setChecked(value != 0);
        }
    }

    private void updateScreenSaverSummary() {
        if (mScreenSaverPreference != null) {
            mScreenSaverPreference.setSummary(
                    DreamSettings.getSummaryTextWithDreamName(getActivity()));
        }
    }

    private void updateSystemBarHideCheckBox(){
        boolean hide_systembar = Settings.System.getInt(getContentResolver(),Settings.System.SYSTEMBAR_HIDE,0)==1;
        mSystemBarHide.setChecked(hide_systembar);

    }


    public void writeFontSizePreference(Object objValue) {
        try {
            mCurConfig.fontScale = Float.parseFloat(objValue.toString());
            ActivityManagerNative.getDefault().updatePersistentConfiguration(mCurConfig);
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to save font size");
        }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if(preference == mSystemBarHide)
        {
            Settings.System.putInt(getContentResolver(), Settings.System.SYSTEMBAR_HIDE,
                    mSystemBarHide.isChecked() ? 1 : 0);
            Intent i = new Intent("com.tchip.changeBarHideStatus");
            getActivity().sendBroadcast(i);
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();
        if (KEY_SCREEN_ORIENTATION.equals(key)) {
            int value = Integer.parseInt((String) objValue);
            try {
                 Log.w(TAG, "freezeRotation :"+value);
                IWindowManager wm = WindowManagerGlobal.getWindowManagerService();
                wm.freezeRotation(value);
                setScreenOrientationSummary(value);
                } catch (RemoteException exc) {
                    Log.w(TAG, "Unable to Rotation");
                }
        }

        if (KEY_SCREEN_TIMEOUT.equals(key)) {
            try {
                int value = Integer.parseInt((String) objValue);
                Settings.System.putInt(getContentResolver(), SCREEN_OFF_TIMEOUT, value);
                updateTimeoutPreferenceDescription(value);
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist screen timeout setting", e);
            }
        }
        if (KEY_FONT_SIZE.equals(key)) {
            writeFontSizePreference(objValue);
        }
        if (preference == mButtonLightsPreference) {
            boolean enable = (Boolean) objValue;
            Settings.System.putInt(getContentResolver(), Settings.System.BUTTON_LIGHTS_ENABLED,
                    enable ? 1 : 0);
        }
        if (preference == mButtonLightsTimeoutPreference) {
            int value = Integer.parseInt((String) objValue);
            Settings.System.putInt(getContentResolver(), Settings.System.BUTTON_LIGHTS_OFF_TIMEOUT, value);
            updateButtonLightsTimeoutPreferenceDescription(value);
        }
        if (preference == mAutoBrightnessPreference) {
            boolean auto = (Boolean) objValue;
            Settings.System.putInt(getContentResolver(), SCREEN_BRIGHTNESS_MODE,
                    auto ? SCREEN_BRIGHTNESS_MODE_AUTOMATIC : SCREEN_BRIGHTNESS_MODE_MANUAL);
        }
        if (preference == mLiftToWakePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), WAKE_GESTURE_ENABLED, value ? 1 : 0);
        }
        if (preference == mDozePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), DOZE_ENABLED, value ? 1 : 0);
        }
  //$_rbox_$_modify_$_hhq: move screensetting
        //$_rbox_$_modify_$_begin
		if ( key.equals(KEY_MAIN_DISPLAY_INTERFACE) ) {
        	mMainDisplay.setValue((String)objValue);
        	int iface = Integer.parseInt((String)objValue);
        	mMainDisplay_set = iface;
        	mMainModeList.setTitle(getIfaceTitle(iface) + " " + getString(R.string.screen_mode_title));
        	SetModeList(mDisplayManagement.MAIN_DISPLAY, iface);
        	String mode = mDisplayManagement.getCurrentMode(mDisplayManagement.MAIN_DISPLAY, iface);
        	if(mode != null) {
	       		mMainModeList.setValue(mode);
        	}
        }
        if( key.equals(KEY_MAIN_DISPLAY_MODE) ) {
        	String mode = (String)objValue;
        	mMainModeList.setValue(mode);
        	mMainMode_set = mode;
        	mMainDisplay_last = mDisplayManagement.getCurrentInterface(mDisplayManagement.MAIN_DISPLAY);
        	if( (mMainDisplay_set != mMainDisplay_last) || (mMainMode_last.equals(mMainMode_set) == false) ) {
        		if(mMainDisplay_set != mMainDisplay_last) {
        			mDisplayManagement.setInterface(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_last, false);
             		mTime = 30;
        		} else
             		mTime = 15;
        		mDisplayManagement.setMode(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_set, mMainMode_set);
        		mDisplayManagement.setInterface(mDisplayManagement.MAIN_DISPLAY, mMainDisplay_set, true);
	        	showDialog(DIALOG_ID_RECOVER);
        	}
        }
        
        if ( key.equals(KEY_AUX_DISPLAY_INTERFACE) ) {
        	mAuxDisplay.setValue((String)objValue);
        	int iface = Integer.parseInt((String)objValue);
        	mAuxDisplay_set = iface;
        	mAuxModeList.setTitle(getIfaceTitle(iface) + " " + getString(R.string.screen_mode_title));
        	SetModeList(mDisplayManagement.AUX_DISPLAY, iface);
        	String mode = mDisplayManagement.getCurrentMode(mDisplayManagement.AUX_DISPLAY, iface);
        	if(mode != null) {
	       		mAuxModeList.setValue(mode);
        	}
        }
        if( key.equals(KEY_AUX_DISPLAY_MODE) ) {
        	String mode = (String)objValue;
        	mAuxModeList.setValue(mode);
        	mAuxMode_set = mode;
        	mAuxDisplay_last = mDisplayManagement.getCurrentInterface(mDisplayManagement.AUX_DISPLAY);
        	if( (mAuxDisplay_set != mAuxDisplay_last) || (mAuxMode_last.equals(mAuxMode_set) == false) ) {
        		if(mAuxDisplay_set != mAuxDisplay_last) {
        			mDisplayManagement.setInterface(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_last, false);
             		mTime = 30;
        		} else
             		mTime = 15;
        		mDisplayManagement.setMode(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_set, mAuxMode_set);
        		mDisplayManagement.setInterface(mDisplayManagement.AUX_DISPLAY, mAuxDisplay_set, true);
	        	showDialog(DIALOG_ID_RECOVER);

        	}
        }
        //$_rbox_$_modify_$_end
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference == mFontSizePref) {
            if (Utils.hasMultipleUsers(getActivity())) {
                showDialog(DLG_GLOBAL_CHANGE_WARNING);
                return true;
            } else {
                mFontSizePref.click();
            }
        }
        return false;
    }

    public static final Indexable.SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
                        boolean enabled) {
                    ArrayList<SearchIndexableResource> result =
                            new ArrayList<SearchIndexableResource>();

                    SearchIndexableResource sir = new SearchIndexableResource(context);
                    sir.xmlResId = R.xml.display_settings;
                    result.add(sir);

                    return result;
                }

                @Override
                public List<String> getNonIndexableKeys(Context context) {
                    ArrayList<String> result = new ArrayList<String>();
                    if (!context.getResources().getBoolean(
                            com.android.internal.R.bool.config_dreamsSupported)) {
                        result.add(KEY_SCREEN_SAVER);
                    }
                    if (!isAutomaticBrightnessAvailable(context.getResources())) {
                        result.add(KEY_AUTO_BRIGHTNESS);
                    }
                    if (!isLiftToWakeAvailable(context)) {
                        result.add(KEY_LIFT_TO_WAKE);
                    }
                    if (!isDozeAvailable(context)) {
                        result.add(KEY_DOZE);
                    }
                    if (!RotationPolicy.isRotationLockToggleVisible(context)) {
                        result.add(KEY_AUTO_ROTATE);
                    }
                    return result;
                }
            };

		class SettingsObserver extends ContentObserver {
		SettingsObserver(Handler handler) {
			super(handler);
		}
		void observe() {
			ContentResolver resolver = getContentResolver();
			resolver.registerContentObserver(Settings.System.getUriFor(
							Settings.System.ACCELEROMETER_ROTATION), true, this,
					UserHandle.USER_CURRENT);
		}
		@Override
		public void onChange(boolean selfChange) {
			super.onChange(selfChange);
            if (mUserSet){
                mUserSet=false;
                return;
            }
			SettingsActivity sa = (SettingsActivity)getActivity();
			sa.startPreferencePanel(DisplaySettings.class.getName(), null,
					R.string.display_settings_title, null, null, 0);
            sa.finish();
		}
	}
}
