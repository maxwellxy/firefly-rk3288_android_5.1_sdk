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

package com.android.providers.media;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import java.io.FileWriter;
import java.io.FileInputStream;
import java.io.IOException;
import android.os.SystemProperties;
public class MtpReceiver extends BroadcastReceiver {
    private final static String TAG = "UsbReceiver";
	private static boolean lastMtpEnabled =false;
	private static boolean lastPtpEnabled =false;
	private static String dirtyRatio;
	private static String dirtyWritebackCentisecs;
	private static String dirtyExpireCentisecs;

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            final Intent usbState = context.registerReceiver(
                    null, new IntentFilter(UsbManager.ACTION_USB_STATE));
            if (usbState != null) {
                handleUsbState(context, usbState);
            }
        } else if (UsbManager.ACTION_USB_STATE.equals(action)) {
            handleUsbState(context, intent);
        }
    }

    private void handleUsbState(Context context, Intent intent) {
        Bundle extras = intent.getExtras();
        boolean connected = extras.getBoolean(UsbManager.USB_CONFIGURED);
        boolean mtpEnabled = extras.getBoolean(UsbManager.USB_FUNCTION_MTP);
        boolean ptpEnabled = extras.getBoolean(UsbManager.USB_FUNCTION_PTP);
    Log.d("xzj",connected+"---------------------------ooooooooooo,stop mtp service---------------"+mtpEnabled);
        // Start MTP service if USB is connected and either the MTP or PTP function is enabled
        if (connected && (mtpEnabled || ptpEnabled)) {
			//Log.d("xzj","mtpEnabled= "+mtpEnabled+" lastMtpEnabled="+lastMtpEnabled+" ptpEnabled="+ptpEnabled+" lastPtpEnabled="+lastPtpEnabled);
			if((lastMtpEnabled != mtpEnabled)&&(lastPtpEnabled !=ptpEnabled))
			{
				Log.d("xzj","---------------------------ooooooooooo,stop mtp service---------------");
				context.stopService(new Intent(context, MtpService.class));
				// tell MediaProvider MTP is disconnected so it can unbind from the service
	            context.getContentResolver().delete(Uri.parse(
				                     "content://media/none/mtp_connected"), null, null);
			}

	    if(mtpEnabled && (lastMtpEnabled != mtpEnabled) && "true".equals(SystemProperties.get("ro.config.enable.mtp_opt"))){
                if(dirtyRatio == null){
			dirtyRatio = readFile("/proc/sys/vm/dirty_ratio");
                	dirtyWritebackCentisecs  = readFile("/proc/sys/vm/dirty_writeback_centisecs");
                	dirtyExpireCentisecs = readFile("/proc/sys/vm/dirty_expire_centisecs");
		}

                writeFile("/proc/sys/vm/dirty_ratio","5");
                writeFile("/proc/sys/vm/dirty_writeback_centisecs","200");
                writeFile("/proc/sys/vm/dirty_expire_centisecs","1500");

            }

			lastMtpEnabled=mtpEnabled;
			lastPtpEnabled=ptpEnabled;
            intent = new Intent(context, MtpService.class);
            if (ptpEnabled) {
                intent.putExtra(UsbManager.USB_FUNCTION_PTP, true);
            }
            context.startService(intent);
            // tell MediaProvider MTP is connected so it can bind to the service
            context.getContentResolver().insert(Uri.parse(
                    "content://media/none/mtp_connected"), null);
        } else {
	    if(lastMtpEnabled && dirtyRatio!=null && "true".equals(SystemProperties.get("ro.config.enable.mtp_opt"))){
		
		writeFile("/proc/sys/vm/dirty_ratio",dirtyRatio);
		writeFile("/proc/sys/vm/dirty_writeback_centisecs",dirtyWritebackCentisecs);
		writeFile("/proc/sys/vm/dirty_expire_centisecs",dirtyExpireCentisecs);
		lastMtpEnabled = false;
	    }
            context.stopService(new Intent(context, MtpService.class));
            // tell MediaProvider MTP is disconnected so it can unbind from the service
            context.getContentResolver().delete(Uri.parse(
                    "content://media/none/mtp_connected"), null, null);
        }
    }
	
    private static void writeFile(String filePath,String f){
	FileWriter fw = null;
	try {
		fw = new FileWriter(filePath);
		fw.write(f);
		
	} catch (IOException e) {
		e.printStackTrace();
	}finally{
	   	if (fw != null) {
           		try {
                    	fw.close();
                	} catch (IOException e) {
               		}
            	}

	}

       
    }

    private static String readFile(String filePath){
        String value = "";
        FileInputStream is = null;
        try {
            is = new FileInputStream(filePath);
            byte [] buffer = new byte[2048];
            int count = is.read(buffer);
            if (count > 0) {
                value = new String(buffer, 0, count);
            }
        } catch (IOException e) {
            Log.d(TAG, "No "+filePath+" exception=" + e);
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                }
            }
        }
        Log.d(TAG, filePath+"=" + value);
        return value;
    }
}
