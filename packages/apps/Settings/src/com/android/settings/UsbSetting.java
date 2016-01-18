/*$_FOR_ROCKCHIP_RBOX_$*/
//$_rbox_$_modify_$_chenzhi_20120314: add for usb host/device switch
/*
 * Copyright (C) 2007 The Android Open Source Project
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

import java.util.Timer;
import java.util.TimerTask;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.provider.MediaStore;
import android.provider.Settings;
import android.provider.MediaStore.Images.Media;
import android.provider.Settings.SettingNotFoundException;
import java.io.File; 
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import android.util.Log;
import android.hardware.usb.UsbManager;
import java.util.List;

public class UsbSetting extends SettingsPreferenceFragment implements
    Preference.OnPreferenceChangeListener {
    private static final String TAG = "UsbSettings";
    private CheckBoxPreference mConnectToPc;
    private File mFile = null;
    private String mMode = null;
    private static final String	HOST_MODE = new String("1");
    private static final String	SLAVE_MODE = new String("2");

    private Handler mHandler;
    private static final String KEY_CONNECT_TO_PC = "connect_to_pc";
    private static final String SYS_FILE = "/sys/bus/platform/drivers/usb20_otg/force_usb_mode";

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(UsbManager.ACTION_USB_STATE)) {
                mConnectToPc.setEnabled(true);
            } 
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.usb_switch);

        mConnectToPc = (CheckBoxPreference) findPreference(KEY_CONNECT_TO_PC);
        mConnectToPc.setOnPreferenceChangeListener(this);

        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_STATE);
        getActivity().registerReceiver(mReceiver, filter);

        mFile = new File(SYS_FILE);

        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);

                if (msg.what == sysFileOperation.SWITCH_FINISH) {
                    String mode = (String) msg.obj;

                    if(HOST_MODE.equals(mode)) {
                        mConnectToPc.setChecked(false);
                    } else if(SLAVE_MODE.equals(mode)) {
                        mConnectToPc.setChecked(true);
                    }
                } else {
                    Log.e(TAG, "unexpect msg:"+ msg.what);
                }
            }
        };
    }

    public String ReadFromFile(File file) {
        if((file != null) && file.exists()) {
            try {
                FileInputStream fin= new FileInputStream(file);
                BufferedReader reader= new BufferedReader(new InputStreamReader(fin));
                String config = reader.readLine();
                fin.close();
                return config;
            } catch(IOException e) {
                e.printStackTrace();
            }
        }
			 
        return null;
    }
	
    public void Write2File(File file,String mode) {
        Log.d(TAG,"Write2File,write mode = "+mode);
        if((file == null) || (!file.exists()) || (mode == null)) return ;

        try {
            FileOutputStream fout = new FileOutputStream(file);
            PrintWriter pWriter = new PrintWriter(fout);
            pWriter.println(mode);
            pWriter.flush();
            pWriter.close();
            fout.close();
        } catch(IOException re) {
        	Log.d(TAG,"write error:"+re);
        	return;
        }
    }

    @Override
    public void onResume() {
        if(mFile.exists()) {
            Log.d(TAG, "read form file "+ SYS_FILE);
            mMode = ReadFromFile(mFile);
            if(mMode != null) {
                if(HOST_MODE.equals(mMode)) {
                    mConnectToPc.setChecked(false);
                } else if(SLAVE_MODE.equals(mMode)) {
                    mConnectToPc.setChecked(true);
                } else if ("0".equals(mMode)) {
                    mConnectToPc.setChecked(true);
                    Write2File(mFile, "2");
                }
                mConnectToPc.setEnabled(true);
            }
        } else {
            Log.e(TAG, "file "+ SYS_FILE +"not found");
        }

        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
    }
    
    @Override
    public void onDestroy() {
    	// TODO Auto-generated method stub
    	super.onDestroy();
        try {
        	getActivity().unregisterReceiver(mReceiver);
        }catch (Exception e){
        	Log.e(TAG,"unregisterReceiver Ex:"+e);
        }
    }


    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();
        if (KEY_CONNECT_TO_PC.equals(key)) {
            if((Boolean) objValue == true) {
                mMode = SLAVE_MODE;
            } else {
                mMode = HOST_MODE;
            }

            mConnectToPc.setEnabled(false);

            // Start separate thread to do the actual loading.
            Thread thread = new Thread(new sysFileOperation(mFile, mMode, mHandler));
            thread.start();
            
            mHandler.postDelayed(new Runnable() {
				
				@Override
				public void run() {
					// TODO Auto-generated method stub
					mConnectToPc.setEnabled(true);
				}
			}, 2000);
        }

        return true;
    }

    private class sysFileOperation implements Runnable {

        private File mFile;
        private String mMode;
        private Handler mHandler;
        public static final int SWITCH_FINISH = 0;

        public sysFileOperation(File file, String mode, Handler handler) {
            mFile = file;
            mHandler = handler;
            mMode = mode;
        }
        public void run() {

            Write2File(mFile, mMode);

            // Tell the UI thread that we are finished.
            Message msg = mHandler.obtainMessage(SWITCH_FINISH, null);
            String real_mode = ReadFromFile(mFile);
            msg.obj = real_mode;
            mHandler.sendMessage(msg);
        }
    }
}
