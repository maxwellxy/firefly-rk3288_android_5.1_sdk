package com.android.settings;

import android.util.Log;
import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.util.ArrayList;

import javax.xml.transform.Result;

import com.android.settings.widget.SwitchBar;

import android.os.AsyncTask;
import android.os.SystemProperties;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Switch;
import android.widget.Toast;
//import static android.provider.Settings.System.HDMI_LCD_TIMEOUT;
import android.content.ContentResolver;
import android.os.Handler;
import android.database.ContentObserver;
import android.os.RemoteException;
import android.os.DisplayOutputManager;

public class HdmiSettings extends SettingsPreferenceFragment
		implements OnPreferenceChangeListener,SwitchBar.OnSwitchChangeListener {
	/** Called when the activity is first created. */
	private static final String TAG = "HdmiControllerActivity";
	private static final String KEY_HDMI_RESOLUTION = "hdmi_resolution";
	private static final String KEY_HDMI_LCD = "hdmi_lcd_timeout";
	private static final String KEY_HDMI_SCALE="hdmi_screen_zoom";
	// for identify the HdmiFile state
	private boolean IsHdmiConnect = false;
	// for identify the Hdmi connection state
	private boolean IsHdmiPlug = false;
	private boolean IsHdmiDisplayOn = false;

	private ListPreference mHdmiResolution;
	private ListPreference mHdmiLcd;
	private Preference mHdmiScale;
	private DisplayOutputManager mDisplayManagement = null;

	private File HdmiDisplayModes=null;
	private Context context;
	private static final int DEF_HDMI_LCD_TIMEOUT_VALUE = 10;

	private SharedPreferences sharedPreferences;
	private SharedPreferences.Editor editor;
	private SwitchBar mSwitchBar;
	private static final String dualModeFileName="sys/class/graphics/fb0/dual_mode";
	private static final String mHdmiModeFileName="/sys/class/display/HDMI/mode";
	private static final String mHdmiEnableFlieName="/sys/class/display/HDMI/enable";
	private static final String mHdmiConnectFileName="sys/class/display/HDMI/connect";
	private static final String SET_MODE="set_mode";
	private static final String GET_MODE="get_mode";
	private static final String HDMI_CONNECTED="connect";
	private static final String HDMI_DISCONNECTED="disconnect";

	private int mDisplay;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		context = getActivity();
		sharedPreferences = getActivity().getSharedPreferences("HdmiSettings",
				Context.MODE_PRIVATE);
		String enable = sharedPreferences.getString("enable", "1");
		editor = sharedPreferences.edit();
		// addPreferencesFromResource(R.xml.hdmi_settings);
		addPreferencesFromResource(R.xml.hdmi_settings_timeout);
		try {
			mDisplayManagement = new DisplayOutputManager();
		} catch (Exception doe) {
			Log.d(TAG, "Can not get DisplayOutputManager object");
			mDisplayManagement = null;
		}
		if(mDisplayManagement != null) {
			Log.d(TAG, "mDisplayManagement is not null");
			
			if (mDisplayManagement.getDisplayNumber() == 0)
				mDisplayManagement = null;
			else if (mDisplayManagement.getDisplayNumber() == 1)
				mDisplay = mDisplayManagement.MAIN_DISPLAY;
			else
				mDisplay = mDisplayManagement.AUX_DISPLAY;
			
		}
		mHdmiLcd = (ListPreference) findPreference(KEY_HDMI_LCD);
		HdmiDisplayModes = new File("sys/class/display/HDMI/modes");
		mHdmiResolution = (ListPreference) findPreference(KEY_HDMI_RESOLUTION);
		mHdmiResolution.setOnPreferenceChangeListener(this);
		String resolutionValue = sharedPreferences
				.getString("resolution", "1280x720p-60");
		mHdmiScale = findPreference(KEY_HDMI_SCALE);
		mHdmiScale.setEnabled(enable.equals("1"));
		Log.d(TAG,"onCreate---------------------");
		initDualMode();
		HdmiIsConnectTask task=new HdmiIsConnectTask();
		task.execute();
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onActivityCreated(savedInstanceState);
		final SettingsActivity activity = (SettingsActivity) getActivity();
	    mSwitchBar = activity.getSwitchBar();
	    mSwitchBar.show();
	    mSwitchBar.addOnSwitchChangeListener(this);
	    mSwitchBar.setChecked(sharedPreferences.getString("enable", "1").equals("1"));
	    
	    String resolutionValue=sharedPreferences.getString("resolution", "1280x720p-60");
	    Log.d(TAG,"onActivityCreated resolutionValue="+resolutionValue);
	   // context.registerReceiver(hdmiReceiver, new IntentFilter("android.intent.action.HDMI_PLUG"));
	}
	
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		Log.d(TAG,"onCreateView----------------------------------------");
		return super.onCreateView(inflater, container, savedInstanceState);
	}
	
	private BroadcastReceiver hdmiReceiver=new BroadcastReceiver() {
		
		@Override
		public void onReceive(Context context, Intent intent) {
			// TODO Auto-generated method stub
			int state=intent.getIntExtra("state", 1);
			if(state==1){
			  HdmiModeTask hdmiModeTask=new HdmiModeTask();
			  hdmiModeTask.execute("getmodes");
			}else{
				mHdmiResolution.setEnabled(false);
			}
		}
	};
	
	
	
	private ContentObserver mHdmiTimeoutSettingObserver = new ContentObserver(
			new Handler()) {
		@Override
		public void onChange(boolean selfChange) {

			ContentResolver resolver = getActivity().getContentResolver();
			final long currentTimeout = Settings.System.getLong(resolver,
					Settings.System.HDMI_LCD_TIMEOUT, -1);
			long lcdTimeout = -1;
			if ((lcdTimeout = Settings.System.getLong(resolver,
					Settings.System.HDMI_LCD_TIMEOUT,
					DEF_HDMI_LCD_TIMEOUT_VALUE)) > 0) {
				lcdTimeout /= 10;
			}
			mHdmiLcd.setValue(String.valueOf(lcdTimeout));
		}
	};

	@Override
	public void onResume() {
		// TODO Auto-generated method stub

		super.onResume();
               
	    context.registerReceiver(hdmiReceiver, new IntentFilter("android.intent.action.HDMI_PLUG"));
		getContentResolver().registerContentObserver(
				Settings.System.getUriFor(Settings.System.HDMI_LCD_TIMEOUT),
				true, mHdmiTimeoutSettingObserver);
	}

	public void onPause() {
		super.onPause();
		Log.d(TAG,"onPause----------------");
		context.unregisterReceiver(hdmiReceiver);
	}

	public void onDestroy() {
		super.onDestroy();
		getContentResolver().unregisterContentObserver(
				mHdmiTimeoutSettingObserver);
	}

	private void initDualMode() {
		int dualMode=sharedPreferences.getInt("dual_mode", 0);
		if(dualMode==0){
			HdmiSettings.this.getPreferenceScreen().removePreference(mHdmiLcd);
		}else{
			mHdmiLcd.setOnPreferenceChangeListener(HdmiSettings.this);
			ContentResolver resolver = context.getContentResolver();
			long lcdTimeout = -1;
			if ((lcdTimeout = Settings.System.getLong(resolver,
					Settings.System.HDMI_LCD_TIMEOUT,
					DEF_HDMI_LCD_TIMEOUT_VALUE)) > 0) {
				lcdTimeout /= 10;
			}
			String enable = sharedPreferences.getString("enable", "1");
			mHdmiLcd.setValue(String.valueOf(lcdTimeout));
			mHdmiLcd.setEnabled(enable.equals("1"));
		}
	}

	protected void setHdmiConfig(boolean enable) {
		HdmiEnableTask hdmiEnableTask=new HdmiEnableTask();
		hdmiEnableTask.execute(enable?"1":"0");
	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			Preference preference) {
		// TODO Auto-generated method stub
		return true;
	}

	private void setHdmiLcdTimeout(int value) {
		if (value != -1) {
			value = (value) * 10;
		}
		HdmiTimeoutSettingTask task=new HdmiTimeoutSettingTask();
		task.execute(Integer.valueOf(value));
		
		
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object objValue) {
		// TODO Auto-generated method stub
		String key = preference.getKey();
		Log.d(TAG, key);
		if (KEY_HDMI_RESOLUTION.equals(key)) {
			if(mDisplayManagement != null){
				mDisplayManagement.setMode(mDisplay,4,(String)objValue);
				mDisplayManagement.saveConfig();
			}else{
				try {
					setHdmiMode((String)objValue);
				} catch (NumberFormatException e) {
					Log.e(TAG, "onPreferenceChanged hdmi_resolution setting error");
				}
			}

		}

		if (KEY_HDMI_LCD.equals(key)) {
			try {
				String strMode = "hdmi_display";
				int value = Integer.parseInt((String) objValue);
				// editor.putInt("enable", value);
				setHdmiLcdTimeout(value);
			} catch (NumberFormatException e) {
				Log.e(TAG, "onPreferenceChanged hdmi_mode setting error");
			}
		}
		editor.commit();
		return true;
	}


	protected void setHdmiMode(String mode) {
		Log.d(TAG, "setHdmiMode");
		HdmiModeTask hdmiModeTask=new HdmiModeTask();
		hdmiModeTask.execute(mode);
	}

	@Override
	public void onSwitchChanged(Switch switchView, boolean isChecked) {
		// TODO Auto-generated method stub
		setHdmiConfig(isChecked);
	}

	private String[] getModes() {
		if (mDisplayManagement != null) {
			String[] modelist = mDisplayManagement.getModeList(mDisplay,4);
			return modelist;
		} else {
			ArrayList<String> list = new ArrayList<String>();
			try {
				FileReader fread = new FileReader(HdmiDisplayModes);
				BufferedReader buffer = new BufferedReader(fread);
				String str = null;
	
				while ((str = buffer.readLine()) != null) {
					list.add(str+"\n");
				}
				fread.close();
				buffer.close();
			} catch (IOException e) {
				Log.e(TAG, "IO Exception");
			}
			return list.toArray(new String[list.size()]);
		}
	}
	

	
	private class HdmiEnableTask extends AsyncTask<String, Void, String>{

		@Override
		protected String doInBackground(String... params) {
			// TODO Auto-generated method stub
			if(mDisplayManagement == null){
				try {
					
					RandomAccessFile rdf = null;
					rdf = new RandomAccessFile(mHdmiEnableFlieName, "rw");
					rdf.writeBytes(params[0]);
					editor.putString("enable", "1");
					rdf.close();
					editor.commit();
				} catch (IOException re) {
					Log.e(TAG, "IO Exception");
					re.printStackTrace();
				}
			}else{
				Log.d(TAG, "setHdmiConfig");
				int enable = (Integer.parseInt(params[0]));
				boolean test = false;
				if(enable > 0)
					test = true;
				else if(enable <=0)
					test = false;
				mDisplayManagement.setInterface(mDisplay,4,test);
				editor.putString("enable", "1");
				editor.commit();
			}
			return params[0];
		}
		
		@Override
		protected void onPostExecute(String result) {
			// TODO Auto-generated method stub
			
			if(result.equals("1")){
				mHdmiLcd.setEnabled(true);
				//mHdmiResolution.setEnabled(true);
				mHdmiScale.setEnabled(true);
			}else{
				mHdmiLcd.setEnabled(false);
				//mHdmiResolution.setEnabled(false);
				mHdmiScale.setEnabled(false);
			}
		}
		
	}
	
	
	
	private class HdmiModeTask extends AsyncTask<String, Void, String[]>{

		@Override
		protected String[] doInBackground(String... params) {
			// TODO Auto-generated method stub
			if (!params[0].equals("getmodes")) {
				try {
					RandomAccessFile rdf = null;
					rdf = new RandomAccessFile(mHdmiModeFileName, "rw");
					rdf.writeBytes(params[0]);
					rdf.close();
					editor.putString("resolution", params[0]).commit();
				} catch (IOException re) {
					Log.e(TAG, "IO Exception");
					re.printStackTrace();
				}
				return null;
			} else {
                return getModes();
			}
		}
		
	    @Override
	    protected void onPostExecute(String[] result) {
	    	// TODO Auto-generated method stub
	    	if(result!=null){
	    		mHdmiResolution.setEntries(result);
				mHdmiResolution.setEntryValues(result);
				mHdmiResolution.setEnabled(true);
				String resolutionValue;
				if(mDisplayManagement != null){
				//String resolutionValue=sharedPreferences.getString("resolution", "1280x720p-60").trim()+"\n";
					 resolutionValue=mDisplayManagement.getCurrentMode(mDisplay, 4);
				}else{
					 resolutionValue=sharedPreferences.getString("resolution", "1280x720p-60").trim()+"\n";
				}
			   // Log.d(TAG,"HdmiModeTask resolutionValue="+resolutionValue);
			    mHdmiResolution.setValue(resolutionValue);
	    	}
	    }
		
	}
	
	private class HdmiTimeoutSettingTask extends AsyncTask<Integer, Void, Void>{

		@Override
		protected Void doInBackground(Integer... params) {
			// TODO Auto-generated method stub
			try {
				Settings.System.putInt(getContentResolver(),
						Settings.System.HDMI_LCD_TIMEOUT, params[0]);
			} catch (NumberFormatException e) {
				Log.e(TAG, "could not persist hdmi lcd timeout setting", e);
			}
			return null;
		}
		
	}
	
	private class HdmiIsConnectTask extends AsyncTask<Void, Void, String>{

		@Override
		protected String doInBackground(Void... params) {
			// TODO Auto-generated method stub
			String isConnected=HDMI_DISCONNECTED;
			try {
				FileReader fread = new FileReader(mHdmiConnectFileName);
				BufferedReader buffer = new BufferedReader(fread);
				String str = null;
				while ((str = buffer.readLine()) != null) {
					if (str.equals("1")) {
						isConnected = HDMI_CONNECTED;
						break;
					} else {
						isConnected = HDMI_DISCONNECTED;
					}
				}
				buffer.close();
				fread.close();
			} catch (IOException e) {
				Log.e(TAG, "IO Exception");
			}
			return isConnected;
		}
		
		@Override
		protected void onPostExecute(String result) {
			// TODO Auto-generated method stub
			if(result.equals(HDMI_CONNECTED)){
				HdmiModeTask hdmiModeTask=new HdmiModeTask();
				hdmiModeTask.execute("getmodes");
			}else{
				mHdmiResolution.setEnabled(false);
			}
		}
	}
}
