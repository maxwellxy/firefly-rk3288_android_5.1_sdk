package com.android.settings;

import java.util.Timer;
import java.util.TimerTask;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.Preference.OnPreferenceChangeListener;
import android.provider.Settings;
import android.util.Log;
import android.view.Gravity;
import android.view.WindowManager;
import android.view.ViewGroup.LayoutParams;
import android.widget.TextView;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.os.SystemProperties;
import android.content.res.Resources;
public class ScreenshotSetting extends SettingsPreferenceFragment implements OnPreferenceChangeListener{
	 /** Called when the activity is first created. */
	private static final String KEY_SCREENSHOT_DELAY="screenshot_delay";
	private static final String KEY_SCREENSHOT_STORAGE_LOCATION="screenshot_storage";
	private static final String KEY_SCREENSHOT_SHOW="screenshot_show";
	private static final String KEY_SCREENSHOT_VERSION="screenshot_version";
	
	private ListPreference mDelay;
	private ListPreference mStorage;
	private CheckBoxPreference mShow;
	private Preference mVersion;
	
	private SharedPreferences mSharedPreference;
	private SharedPreferences.Editor mEdit;
	private Screenshot mScreenshot;
	
	private Context mContext;
    private Dialog dialog;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.screenshot);
        
        mContext=getActivity();
        mDelay=(ListPreference)findPreference(KEY_SCREENSHOT_DELAY);
        mStorage=(ListPreference)findPreference(KEY_SCREENSHOT_STORAGE_LOCATION);
        mShow=(CheckBoxPreference)findPreference(KEY_SCREENSHOT_SHOW);
        mVersion=(Preference)findPreference(KEY_SCREENSHOT_VERSION);
        
        mShow.setOnPreferenceChangeListener(this);
        mDelay.setOnPreferenceChangeListener(this);
        mStorage.setOnPreferenceChangeListener(this);
        
        mSharedPreference=this.getPreferenceScreen().getSharedPreferences();
        mEdit=mSharedPreference.edit();
         
        String summary_delay =mDelay.getSharedPreferences().getString("screenshot_delay", "15");
    	mDelay.setSummary(summary_delay+getString(R.string.later));
        mDelay.setValue(summary_delay);
    	String summary_storage=mStorage.getSharedPreferences().getString("screenshot_storage", "flash");
        mStorage.setValue(summary_storage);
    	mStorage.setSummary(summary_storage);
        Resources res = mContext.getResources();
        boolean mHasNavigationBar = res.getBoolean(com.android.internal.R.bool.config_showNavigationBar);
        if(!mHasNavigationBar){
           getPreferenceScreen().removePreference(mShow);
        }
        getPreferenceScreen().removePreference(mVersion);
        
        mScreenshot=(Screenshot)getActivity().getApplication();
    }
   
        
	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		if(preference==mDelay){
			int value=Integer.parseInt((String) newValue);
            mDelay.setSummary((String)newValue+getString(R.string.later));
            mScreenshot.startScreenshot(value);
		}else if(preference==mStorage){
			String value=(String)newValue;
			//mEdit.putString("storageLocation",value);
			if(value.equals("flash")){
			String enableUms= SystemProperties.get("ro.factory.hasUMS","false");
			if("true".equals(enableUms))//if has UMS function,flash is primary storage
			{
					Settings.System.putString(getContentResolver(), Settings.System.SCREENSHOT_LOCATION, "/mnt/internal_sd");
			}
			else
			{
					Settings.System.putString(getContentResolver(), Settings.System.SCREENSHOT_LOCATION, "/storage/emulated");
			}

				
			}else if(value.equals("sdcard")){
				Settings.System.putString(getContentResolver(), Settings.System.SCREENSHOT_LOCATION, "/mnt/external_sd");
			}else if(value.equals("usb")){
				Settings.System.putString(getContentResolver(), Settings.System.SCREENSHOT_LOCATION, "/mnt/usb_storage");
			}
			mStorage.setSummary(value);
			
		}
		return true;
	}
	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			Preference preference) {
		// TODO Auto-generated method stub
		if(preference==mShow){
			Log.d("screenshot","onPreferenceTreeClick mShow");
			boolean show=mShow.isChecked();
			Settings.System.putInt(getContentResolver(), Settings.System.SCREENSHOT_BUTTON_SHOW, show?1:0);
		    //getWindow().getDecorView().setSystemUiVisibility(-1000);
			Intent intent=new Intent();
			intent.setAction("rk.android.screenshot.SHOW");
			intent.putExtra("show", show);
			mContext.sendBroadcast(intent);
		}
		return true;
	}

}
