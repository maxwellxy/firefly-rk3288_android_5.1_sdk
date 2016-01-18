/*$_FOR_ROCKCHIP_RBOX_$*/

package com.android.settings;

import com.android.server.AudioCommon;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.util.Log;


/* 
 * This file is used to manager Sound Playback & Capture Devices
 * @Author zxg ,TV Dept.
 * 
 * {@hide}
 */
public class SoundDevicesManager extends PreferenceActivity {

	private static final String TAG = SoundDevicesManager.class.getSimpleName();
	private static final boolean DEBUG = true;
	private static final String AUDIOPCMLISTUPDATE = "com.android.server.audiopcmlistupdate";
	/* this is from kernel. */
	private static final String USBAUDIO_CARD_DRIVER_NAME = "USB-Audio";
	private static final String SPDIF_CARD_DRIVER_NAME = "SPDIF";
	private static final String PASSTHROUGH = "PASSTHROUGH";
	private static final String MULTICHANNEL = "5POINT1 MULTICHANNEL";


	private static String SOC_AND_SPDIF_NAME = "";
	//spdif passthrough
	private static String MEDIA_CFG_AUDIO_BYPASS = "media.cfg.audio.bypass";
	private static String HDMI_AUDIO_MULTICHANNEL = "media.cfg.audio.mul";
	private static final String SOC_AND_SPDIF_KEY = "9";
	private static final String SPDIF_PASSTHROUGH_KEY = "8";
	private static final String HDMI_MULTICHANNEL_KEY = "7";
	private static final String HDMI_PASSTHROUGH_KEY = "6";
	
	private static String SPDIF_PASSTHROUGH_NAME = "";
	private static String HDMI_MULTICHANNEL_NAME = "";
	private static String HDMI_PASSTHROUGH_NAME = "";

	private static boolean bAudioPassthroughSupport= false;
	
	private int mCaptureCounts = 0;
	private int mPlaybackCounts = 0;
	private List<SndElement> mCardsList;
	private PreferenceGroup mSoundOutput_List;
	private String mSelectedCaptureKey;
	private String mSelectedPlaybackKey;
	private IntentFilter mAudioDevicesListUpdate_IF = null;

	private Context mContext;

	private void logd(String msg) {
		if (DEBUG) {
			Log.d(TAG, msg);
		}
	}

	private BroadcastReceiver mAudioDevicesListUpdate_BR = new BroadcastReceiver() {

		@Override
		public void onReceive(Context context, Intent intent) {
			update();

		}
	};

	protected void update() {
		fillList();

	}


	Preference.OnPreferenceClickListener mPlaybackListClickListener = new Preference.OnPreferenceClickListener() {

		@Override
		public boolean onPreferenceClick(Preference preference) {
			logd("Key: " + preference.getKey() + ", last key: "
					+ mSelectedPlaybackKey + " Card: "
					+ preference.getTitle().toString());
			RadioPreference rp;
			if (mSelectedPlaybackKey.equals(preference.getKey())) {
				rp = (RadioPreference) preference;
				rp.setChecked(true);
				return true;
			}
			rp = (RadioPreference) mSoundOutput_List
					.findPreference(mSelectedPlaybackKey);
			if (rp != null)
				rp.setChecked(false);
			mSelectedPlaybackKey = preference.getKey();
			rp = (RadioPreference) preference;
			rp.setChecked(true);
			
			int deviceType = AudioCommon.SND_DEV_TYPE_DEFAULT;
			String cardName = rp.getTitle().toString();
			boolean bSpdifPassThrough = false;
			boolean bHdmiMultichannel = false;

			if(cardName.equals(getString(R.string.sound_output_default))){
				deviceType = AudioCommon.SND_DEV_TYPE_DEFAULT;
			}else if(cardName.equals(getString(R.string.sound_output_spdif_passthrough))) {
				deviceType = AudioCommon.SND_DEV_TYPE_SPDIF_PASSTHROUGH;
				bSpdifPassThrough = true;
			}else if(isUsbAudio(mSelectedPlaybackKey)){
				deviceType = AudioCommon.SND_DEV_TYPE_USB;
			}else if(cardName.equals(getString(R.string.sound_output_hdmi_passthrough))){
				bSpdifPassThrough = true;
				deviceType = AudioCommon.SND_DEV_TYPE_HDMI_PASSTHROUGH;
			}else if(cardName.equals(getString(R.string.sound_output_hdmi_multilpcm))){
				bHdmiMultichannel = true;
				deviceType = AudioCommon.SND_DEV_TYPE_HDMI_MULTILPCM;
			}
			
			if(bSpdifPassThrough)
				SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "true");
			else
				SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "false");
			
			if(bHdmiMultichannel){
				SystemProperties.set(HDMI_AUDIO_MULTICHANNEL, "true");
			} else {
				SystemProperties.set(HDMI_AUDIO_MULTICHANNEL, "false");
			}
			AudioCommon.doAudioDevicesRouting(mContext, deviceType, AudioCommon.SND_PCM_STREAM_PLAYBACK, mSelectedPlaybackKey);
			return true;
		}
	};

	Preference.OnPreferenceChangeListener mPlaybackChangeListener = new Preference.OnPreferenceChangeListener() {

		@Override
		public boolean onPreferenceChange(Preference preference, Object newValue) {
			logd("onPreferenceChange(): Preference - " + preference
					+ ", key - " + preference.getKey() + ", newValue - "
					+ newValue + ", newValue type - " + newValue.getClass());
			return true;
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState) {
	    mContext = getApplicationContext();
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.sound_devices_manager);
		getListView().setItemsCanFocus(true);

		mSoundOutput_List = (PreferenceGroup) findPreference("sound_playback_list");
		mCardsList = new ArrayList<SoundDevicesManager.SndElement>();

		// receive intent from WiredAccessoryObserver
		mAudioDevicesListUpdate_IF = new IntentFilter();
		mAudioDevicesListUpdate_IF.addAction(AUDIOPCMLISTUPDATE);
		
		//whether the device support audio passthrough
		if(!SystemProperties.get(MEDIA_CFG_AUDIO_BYPASS).isEmpty()){
			bAudioPassthroughSupport = true;
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		registerReceiver(mAudioDevicesListUpdate_BR, mAudioDevicesListUpdate_IF);
		fillList();

	}

	@Override
	protected void onPause() {
		super.onPause();
		unregisterReceiver(mAudioDevicesListUpdate_BR);
	}

	private class SndElement {

		public SndElement() {
			idx = "";
			cardName = "";
			hasCapture = false;
			hasPlayback = false;
			devType = AudioCommon.SND_DEV_TYPE_DEFAULT;
		}

		public String idx;
		public String cardName;
		public boolean hasPlayback;
		public boolean hasCapture;
		public int devType;
	}

	private void fillList() {
		// TODO Auto-generated method stub
		boolean hasSocAudio_playback = false;
		boolean hasSpdifAudio_playback = false;
		SOC_AND_SPDIF_NAME = "";
		mSelectedCaptureKey = "0";
		mSelectedPlaybackKey = "0";

		mSoundOutput_List.removeAll();
		mCaptureCounts = 0;
		mPlaybackCounts = 0;
		mCardsList.clear();

		readProcFileAndInit();
		initSelectedKey();

		Iterator<SndElement> it = mCardsList.iterator();
		while (it.hasNext()) {
			SndElement se = it.next();
			if (se.devType == AudioCommon.SND_DEV_TYPE_USB)
				continue;

			if (se.hasPlayback) {
				RadioPreference rp = new RadioPreference(this, null);
				rp.setKey(se.idx);
				rp.setPersistent(false);
				rp.setWidgetLayoutResource(R.layout.preference_radio);
				rp.setOnPreferenceClickListener(mPlaybackListClickListener);
				rp.setOnPreferenceChangeListener(mPlaybackChangeListener);
				mSoundOutput_List.addPreference(rp);
				mPlaybackCounts++;

				
				if(se.devType == AudioCommon.SND_DEV_TYPE_DEFAULT){
					rp.setTitle(R.string.sound_output_default);
					hasSocAudio_playback = true;
				}else if(se.devType == AudioCommon.SND_DEV_TYPE_SPDIF_PASSTHROUGH){
					rp.setTitle(R.string.sound_output_spdif_passthrough);
					hasSpdifAudio_playback = true;
					rp.setKey(SPDIF_PASSTHROUGH_KEY);
				}

				if (mSelectedPlaybackKey.equals(rp.getKey())) {
					rp.setChecked(true);
				}
			}

		}
		
		if(hasSocAudio_playback){
			RadioPreference rp = new RadioPreference(this, null);
			rp.setKey(HDMI_PASSTHROUGH_KEY);
			rp.setTitle(R.string.sound_output_hdmi_passthrough);
			rp.setPersistent(false);
			rp.setWidgetLayoutResource(R.layout.preference_radio);
			rp.setOnPreferenceClickListener(mPlaybackListClickListener);
			rp.setOnPreferenceChangeListener(mPlaybackChangeListener);
			mSoundOutput_List.addPreference(rp);
			
			if (mSelectedPlaybackKey.equals(rp.getKey())) {
				rp.setChecked(true);
			}

			/*if(!"rk31sdk".equals(Build.BOARD) && !"rk3028sdk".equals(Build.BOARD)){

				rp = new RadioPreference(this, null);
				rp.setKey(HDMI_MULTICHANNEL_KEY);
				rp.setTitle(R.string.sound_output_hdmi_multilpcm);
				rp.setPersistent(false);
				rp.setWidgetLayoutResource(R.layout.preference_radio);
				rp.setOnPreferenceClickListener(mPlaybackListClickListener);
				rp.setOnPreferenceChangeListener(mPlaybackChangeListener);
				mSoundOutput_List.addPreference(rp);

				if (mSelectedPlaybackKey.equals(rp.getKey())) {
					rp.setChecked(true);
				}
			}*/

		}
		
		// show tips when there is no devices
		if (0 == mPlaybackCounts) {
			Preference tip = new Preference(this);
			tip.setTitle("There is no Sound Output Devices");
			mSoundOutput_List.addPreference(tip);
		}

	}

	private void initSelectedKey() {
		// capture
		mSelectedCaptureKey = AudioCommon.getCurrentCaptureDevice();
		// playback
		mSelectedPlaybackKey = AudioCommon.getCurrentPlaybackDevice();

		logd("mSelectedCaptureKey:"+mSelectedCaptureKey+"; mSelectedPlaybackKey"+mSelectedPlaybackKey);
	}

	private void readProcFileAndInit() {
		// init
		mCaptureCounts = 0;
		mPlaybackCounts = 0;
		mCardsList.clear();

		String CardsPath = "/proc/asound/cards";
		String PcmPath = "/proc/asound/pcm";
		FileReader cards_fr = null;
		FileReader pcms_fr = null;
		BufferedReader cards_br = null;
		BufferedReader pcms_br = null;
		try {
			try {
				cards_fr = new FileReader(CardsPath);
				pcms_fr = new FileReader(PcmPath);
			} catch (FileNotFoundException e) {
				// TODO: handle exception
				e.printStackTrace();
			}
			cards_br = new BufferedReader(cards_fr);
			pcms_br = new BufferedReader(pcms_fr);
			String Line;
			// cards
			while ((Line = cards_br.readLine()) != null) {
				int pos = Line.lastIndexOf(" - ");
				if (pos > 0) {
					SndElement se = new SndElement();
					String idx = Line.substring(0, 2).trim();
					String cardName = Line.substring(pos + 3);
					pos = Line.indexOf(USBAUDIO_CARD_DRIVER_NAME);
					if (pos > 0) {
						se.devType = AudioCommon.SND_DEV_TYPE_USB;
					}
					pos = Line.indexOf(SPDIF_CARD_DRIVER_NAME);
					if (pos > 0) {
						se.devType = AudioCommon.SND_DEV_TYPE_SPDIF_PASSTHROUGH;
					}
					se.idx = idx;
					se.cardName = cardName;
					logd("idx: " + idx + " cardname: " + cardName);
					mCardsList.add(se);
				}
			}
			// pcms
			int pos;
			while ((Line = pcms_br.readLine()) != null) {
				String idx = Line.substring(1, 2);
				logd("idx: " + idx);
				Iterator<SndElement> it = mCardsList.iterator();
				while (it.hasNext()) {
					SndElement se = it.next();
					if (se.idx.equals(idx)) {
						pos = Line.indexOf("capture");
						if (pos > 0)
							se.hasCapture = true;
						pos = Line.indexOf("playback");
						if (pos > 0)
							se.hasPlayback = true;
						break;
					}
				}
			}

		} catch (IOException e) {
			// TODO: handle exception
			e.printStackTrace();
		} finally {
			try {
				if (cards_br != null)
					cards_br.close();
				if (cards_fr != null)
					cards_fr.close();
				if (pcms_br != null)
					pcms_br.close();
				if (pcms_fr != null)
					pcms_fr.close();

			} catch (IOException e) {
				// TODO: handle exception
				e.printStackTrace();
			}
		}

	}
	
	private boolean isUsbAudio(String cardidx) {
		boolean isUsb = false;
		Iterator<SndElement> it = mCardsList.iterator();
		while (it.hasNext()) {
			SndElement se = it.next();
			if (se.idx.equals(cardidx)) {
				isUsb = (se.devType == AudioCommon.SND_DEV_TYPE_USB);
				break;
			}
		}
		
		return isUsb;
	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			Preference preference) {
		logd("onPreferenceTreeClick: " + preference.getTitle().toString());
		return true;
	}

}
