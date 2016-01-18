

package com.android.settings;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.os.AsyncTask;
import android.os.RemoteException;
import android.os.IPowerManager;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.preference.SeekBarPreference;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.util.Log;

import java.util.Map;
import java.io.*;

import android.os.SystemProperties;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.SystemProperties;
import android.content.ContentResolver;

import java.io.RandomAccessFile;

import static android.provider.Settings.System.HDMI_LCD_TIMEOUT;
import android.widget.Toast;

import android.os.DisplayOutputManager;
import com.android.settings.R;

public class HdmiReceiver extends BroadcastReceiver {
	private final String HDMI_ACTION = "android.intent.action.HDMI_PLUG";
	private final String BOOT_ACTION="android.intent.action.BOOT_COMPLETED";
	private static final String TAG = "HdmiReceiver";
	private File HdmiDisplayEnable = new File("/sys/class/display/HDMI/enable");
	private File HdmiDisplayMode = new File("/sys/class/display/HDMI/mode");
	private File HdmiDisplayScale = new File("/sys/class/display/HDMI/scale");
	private final String hdmiEnableFileName="/sys/class/display/HDMI/enable";
	private final String hdmiModeFileName="/sys/class/display/HDMI/mode";
	private final String hdmiScaleFileName="/sys/class/display/HDMI/";
	private Context mcontext;
	private SharedPreferences preferences;
	private File DualModeFile = new File("/sys/class/graphics/fb0/dual_mode");
	private DisplayOutputManager mDisplayManagement = null;
	@Override
	public void onReceive(Context context, Intent intent) {
		mcontext = context;
		preferences = context.getSharedPreferences("HdmiSettings",
				Context.MODE_PRIVATE);
		String action=intent.getAction();
	   	try {
                       mDisplayManagement = new DisplayOutputManager();
               	}catch (RemoteException doe) {
               		mDisplayManagement = null;        
               	}
		Log.d(TAG,"hdmi receiver action="+action);
		if (action.equals(HDMI_ACTION)) {
			String enable = null;
			String scale = null;
			String resol = null;
			int state = intent.getIntExtra("state", 0);
			if (state == 1) {
				enable = preferences.getString("enable", "1");
				resol = preferences.getString("resolution", "1280x720p-60").trim()+"\n";
				scale = preferences.getString("scale_set", "100");
				restoreHdmiValue(HdmiDisplayEnable, enable, "enable");
				restoreHdmiValue(HdmiDisplayMode, resol, "hdmi_resolution");
				restoreHdmiValue(HdmiDisplayScale, scale, "hdmi_scale");
			}
			int dualMode=preferences.getInt("dual_mode", 0);
			if (dualMode == 1) {
				if (state == 1) {
					SystemProperties.set("sys.hdmi_screen.scale",
							scale);
				} else {
					SystemProperties.set("sys.hdmi_screen.scale",
							"100");
				}
			}
			String text = context.getResources().getString(
					(state == 1) ? R.string.hdmi_connect
							: R.string.hdmi_disconnect);
			Toast.makeText(context, text, Toast.LENGTH_SHORT).show();
		/*	Log.d(TAG,
					"enable =" + String.valueOf(enable) + " scale="
							+ String.valueOf(scale) + " resol="
							+ String.valueOf(resol)+"resol_length=" +resol.length());*/
		}else if(action.equals(BOOT_ACTION)){
                         Log.d(TAG,"BOOT_COMPLETED");
                         preferences = context.getSharedPreferences("HdmiSettings",
                                Context.MODE_PRIVATE);
                         String enable = null;
                         String scale = null;
                         String resol = null;
                         enable = preferences.getString("enable", "0");
                         if(enable.equals("1")){
                                resol = preferences.getString("resolution", "1280x720p-60").trim()+"\n";
                                scale = preferences.getString("scale_set", "100");
                                restoreHdmiValue(HdmiDisplayMode, resol, "hdmi_resolution");
                                restoreHdmiValue(HdmiDisplayScale, scale, "hdmi_scale");
                                restoreHdmiValue(HdmiDisplayEnable, enable, "enable");
                          }
			InitDualModeTask initDualModeTask=new InitDualModeTask();
			initDualModeTask.execute();
		}

	}


	protected void restoreHdmiValue(final File file, final String value, final String style) {
		if(mDisplayManagement != null)
			return;	
		Thread thread=new Thread(new Runnable() {
			
			@Override
			public void run() {
				// TODO Auto-generated method stub
				if (file.exists()) {
					try {
						String substr = null;
						String str = null;
						int length = 0;
							if (style.equals("enable")) {
								Log.d(TAG, "restoreHdmiValue enable");
								RandomAccessFile rdf = null;
								rdf = new RandomAccessFile(file, "rw");
								rdf.writeBytes(value);
								rdf.close();
							}
							if (style.equals("hdmi_scale")) {
								OutputStream output = null;
								OutputStreamWriter outputWrite = null;
								PrintWriter print = null;
								try {
									output = new FileOutputStream(file);
									outputWrite = new OutputStreamWriter(output);
									print = new PrintWriter(outputWrite);
									print.print(value);
									print.flush();
									print.close();
									outputWrite.close();
									output.close();
								} catch (FileNotFoundException e) {
									e.printStackTrace();
							}
						}
						if (style.equals("hdmi_resolution")) {
							Log.d(TAG, "restoreHdmiValue hdmi_resolution");
							OutputStream output = null;
							OutputStreamWriter outputWrite = null;
							PrintWriter print = null;
							output = new FileOutputStream(file);
							outputWrite = new OutputStreamWriter(output);
							print = new PrintWriter(outputWrite);
							if(value==null){
								String mode=getCurrentMode();
								Log.d(TAG,"getCurrentMode="+mode);
								print.print(mode);
								preferences.edit().putString("resolution", mode).commit();
							}else{
								print.print(value);
							}
							print.flush();
							print.close();
							outputWrite.close();
							output.close();
						}


					} catch (Exception e) {
						e.printStackTrace();
					}
				} else {
					Log.e(TAG, "File:" + file + "not exists");
				}
			}
		});
		thread.start();
	}

	private String getCurrentMode(){
		String mode=null;
		try {
			FileReader fread = new FileReader(HdmiDisplayMode);
			BufferedReader buffer = new BufferedReader(fread);
			String str = null;
			while ((str = buffer.readLine()) != null) {
				mode=str+"\n";
				Log.d(TAG,"getCurrentMode mode="+mode);
			}
			buffer.close();
			fread.close();
		} catch (IOException e) {
			Log.e(TAG, "IO Exception");
		}
		return mode;
	}
	
	
	private class InitDualModeTask extends AsyncTask<String, Void, Void>{

		@Override
		protected Void doInBackground(String... params) {
			// TODO Auto-generated method stub
			try {
				byte[] buf = new byte[10];
				int len = 0;
				RandomAccessFile rdf = new RandomAccessFile(DualModeFile, "r");
				len = rdf.read(buf);
				String modeStr = new String(buf, 0, 1);
				int dualMode = Integer.valueOf(modeStr);
				preferences.edit().putInt("dual_mode", dualMode).commit();
				rdf.close();
			} catch (Exception e) {
				e.printStackTrace();
			} 
			return null;
		}
		
	}
	
}
