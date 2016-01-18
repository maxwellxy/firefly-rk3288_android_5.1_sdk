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

package com.android.settings;

import android.util.Log;
import android.content.Context;
import android.os.AsyncTask;
import android.os.RemoteException;
import android.os.IPowerManager;
import android.os.ServiceManager;
import android.preference.SeekBarPreference;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.preference.SeekBarDialogPreference;
import android.os.SystemProperties;

import java.util.Map;
import java.io.*;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.DisplayOutputManager;
import android.graphics.Rect;

public class HdmiScreenZoomPreference extends SeekBarDialogPreference implements
		SeekBar.OnSeekBarChangeListener, CheckBox.OnCheckedChangeListener {

	private static final String TAG = "HdmiScreenZoomPreference";
	private static final int MINIMUN_SCREEN_SCALE = 0;
	private static final int MAXIMUN_SCREEN_SCALE = 20;

	private File HdmiScale; 
	private File DualModeFile = new File("/sys/class/graphics/fb0/dual_mode");
	private SeekBar mSeekBar;
	private int mOldScale = 0;
	private int mValue = 0;
	private int mRestoreValue = 0;
	private boolean mFlag = false;
	// for save hdmi config
	private Context context;
	private final File HdmiState = new File("sys/class/display/HDMI/connect");
	private int dualMode;
	private SharedPreferences preferences;
	private DisplayOutputManager mDisplayManagement = null;

	public HdmiScreenZoomPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
		this.context = context;
		setDialogLayoutResource(R.layout.preference_dialog_screen_scale);
		setDialogIcon(R.drawable.ic_settings_screen_scale);
		preferences = context.getSharedPreferences(
				"HdmiSettings", context.MODE_PRIVATE);
		dualMode=preferences.getInt("dual_mode", 0);
		try {
			mDisplayManagement = new DisplayOutputManager();
		}catch (RemoteException doe) {
			mDisplayManagement = null;
		}
		
		if (mDisplayManagement != null &&
		    mDisplayManagement.getDisplayNumber() == 0)
			mDisplayManagement = null;		
	}

	protected void setHdmiScreenScale(File file, int value) {
		//if (mDisplayManagement == null || value < 0){
			//if (!isHdmiConnected(HdmiState)) {
			//	return;
			//}
			// if (dualMode == 1) {
				// SystemProperties.set("sys.hdmi_screen.scale",
						// String.valueOf((char) value));
			// } else {
				//SystemProperties.set("sys.hdmi_screen.scale",
				//		String.valueOf((char) 100));
			//}
		//}else {
			HdmiScaleTask hdmiScaleTask=new HdmiScaleTask();
			hdmiScaleTask.execute(value);
		//}

	}

	private boolean isHdmiConnected(File file) {
		boolean isConnected = false;
		if (file.exists()) {
			try {
				FileReader fread = new FileReader(file);
				BufferedReader buffer = new BufferedReader(fread);
				String strPlug = "plug=1";
				String str = null;
				while ((str = buffer.readLine()) != null) {
					int length = str.length();
					// if((length == 6) && (str.equals(strPlug))){
					if (str.equals("1")) {
						isConnected = true;
						break;
					} else {
						isConnected = false;
					}
				}
			} catch (IOException e) {
				Log.e(TAG, "IO Exception");
			}
		}
		return isConnected;
	}


	@Override
	protected void onBindDialogView(View view) {
		super.onBindDialogView(view);
		
		if(isRK3128()){
		   HdmiScale = new File("/sys/class/display/HDMI/scale");//3128使用	
		}else{
		   HdmiScale = new File("/sys/class/graphics/fb0/scale");
		}

		mFlag = false;
		mSeekBar = getSeekBar(view);
		// resotre value
		SharedPreferences preferences = context.getSharedPreferences(
				"HdmiSettings", context.MODE_PRIVATE);

		Rect overscan;
		if(mDisplayManagement != null){
			if (mDisplayManagement.getDisplayNumber() == 2)
				overscan = mDisplayManagement.getOverScan(mDisplayManagement.AUX_DISPLAY);
			else
				overscan = mDisplayManagement.getOverScan(mDisplayManagement.MAIN_DISPLAY);
			mOldScale = overscan.left - 80;
		}else{
			mOldScale = Integer.valueOf(preferences.getString("scale_set", "100"));
			mOldScale = mOldScale - 80;
		}
		

		mSeekBar.setProgress(mOldScale);
		mSeekBar.setOnSeekBarChangeListener(this);
	}

	public void onProgressChanged(SeekBar seekBar, int progress,
			boolean fromTouch) {
		mValue = progress + 80;
		if (mValue > 100) {
			mValue = 100;
		}
		setHdmiScreenScale(HdmiScale, mValue);
	}

	public void onStartTrackingTouch(SeekBar seekBar) {
		// If start tracking, record the initial position
		mFlag = true;
		mRestoreValue = seekBar.getProgress();
	}

	public void onStopTrackingTouch(SeekBar seekBar) {
		setHdmiScreenScale(HdmiScale, mValue);
	}

	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

	}

	@Override
	protected void onDialogClosed(boolean positiveResult) {
		super.onDialogClosed(positiveResult);
		// for save config

		if (positiveResult) {
			int value = mSeekBar.getProgress() + 80;
			setHdmiScreenScale(HdmiScale, value);
			//editor.putString("scale_set", String.valueOf(value));
		} else {
			if (mFlag) {
				mRestoreValue = mRestoreValue + 80;
				if (mRestoreValue > 100) {
					mRestoreValue = 100;
				}
				setHdmiScreenScale(HdmiScale, mRestoreValue);
				//editor.putString("scale_set", String.valueOf(mRestoreValue));
			} else {
				// click cancel without any other operations
				int value = mSeekBar.getProgress() + 80;
				setHdmiScreenScale(HdmiScale, value);
				//editor.putString("scale_set", String.valueOf(value));
			}
		}
		//editor.commit();
	}
	
	private class HdmiScaleTask extends AsyncTask<Integer, Void, Void>{

		@Override
		protected Void doInBackground(Integer... params) {
			// TODO Auto-generated method stub
			if(mDisplayManagement != null){
				int value = params[0];
				if (mDisplayManagement.getDisplayNumber() == 2)
					mDisplayManagement.setOverScan(mDisplayManagement.AUX_DISPLAY,
								       mDisplayManagement.DISPLAY_OVERSCAN_ALL,
								       value);
				else
					mDisplayManagement.setOverScan(mDisplayManagement.MAIN_DISPLAY,
								       mDisplayManagement.DISPLAY_OVERSCAN_ALL,
								       value);
			}else{			
				try {
					//StringBuffer strbuf = new StringBuffer("");
					//strbuf.append(params[0]);
					String s = String.valueOf(params[0]);
					OutputStream output = null;
					OutputStreamWriter outputWrite = null;
					PrintWriter print = null;

					try {
						// SystemProperties.set("sys.hdmi_screen.scale",String.valueOf(value));
						output = new FileOutputStream(HdmiScale);
						outputWrite = new OutputStreamWriter(output);
						print = new PrintWriter(outputWrite);
	                    			Log.d(TAG,"scale value="+params[0]);
						print.print(params[0]);
						print.flush();
						outputWrite.close();
						print.close();
						output.close();
						preferences.edit().putString("scale_set", s).commit();
					} catch (FileNotFoundException e) {
						e.printStackTrace();
					}
				} catch (IOException e) {
					Log.e(TAG, "IO Exception");
				}
			}
			return null;

		}
		
	}
	
	private boolean isRK3128() {
    	final String RK3128 = "RK3128";
	try{
	   FileReader fr = new FileReader("/proc/cpuinfo");
           BufferedReader br = new BufferedReader(fr);
           String str;
           while((str = br.readLine()) != null) {
        	if (str.contains(RK3128)) {
		    return true;
		}
           }	
	}catch(Exception e){
		return false;
	}
        return false;
    }

	
}
