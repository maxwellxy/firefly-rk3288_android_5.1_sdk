package com.android.server;

import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import android.app.ActivityManagerNative;
import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.util.Slog;
import android.media.AudioManager;

public class AudioCommon {

	private static final String TAG = AudioCommon.class.getSimpleName();

	public static final String AUDIOPCMLISTUPDATE = "com.android.server.audiopcmlistupdate";
	public static final String HW_AUDIO_CURRENTPLAYBACK = "persist.audio.currentplayback";
	public static final String HW_AUDIO_CURRENTCAPTURE = "persist.audio.currentcapture";
	public static final String HW_AUDIO_LASTSOCPLAYBACK = "persist.audio.lastsocplayback";
	public static final String HW_AUDIO_SPDIF_MUTE = "persist.audio.spdif.mute";
	public static final String HW_AUDIO_HDMI_MUTE = "persist.audio.hdmi.mute";
	public static final String HW_AUDIO_HDMI_BITSTREAM_CHANNELS = "persist.audio.hdmi.channels";
	public static final String MEDIA_CFG_AUDIO_BYPASS = "media.cfg.audio.bypass";
	public static final String HW_AUDIO_HDMI_BYPASS = "persist.audio.hdmi.bypass";
	public static final String HW_AUDIO_SPDIF_BYPASS = "persist.audio.spdif.bypass";
	public static final String HW_AUDIO_HDMI_AUTO_IDENTIFY = "persist.audio.hdmi.autoidentify";
	public static final String SOC_DEFAULT_KEY = "0";
	public static final String SOC_AND_SPDIF_KEY = "9";
	public static final String SPDIF_PASSTHROUGH_KEY = "8";
	public static final String HDMI_MULTICHANNEL_KEY = "7";
	public static final String HDMI_PASSTHROUGH_KEY = "6";

	private static final String USB_AUDIO_PLAYBACK_SWITCH_STATE_FILE = "/sys/class/switch/usb_audio_playback/state";
	private static final String USB_AUDIO_CAPTURE_SWITCH_STATE_FILE = "/sys/class/switch/usb_audio_capture/state";

	public static final int SND_DEV_TYPE_BASE = 0;
	public static final int SND_DEV_TYPE_USB = SND_DEV_TYPE_BASE + 1;
	public static final int SND_DEV_TYPE_SPDIF = SND_DEV_TYPE_BASE + 2;
	public static final int SND_DEV_TYPE_SOC_SPDIF = SND_DEV_TYPE_BASE + 3;
	public static final int SND_DEV_TYPE_HDMI_MULTILPCM = SND_DEV_TYPE_BASE + 4;
	public static final int SND_DEV_TYPE_HDMI_PASSTHROUGH = SND_DEV_TYPE_BASE + 5;
	public static final int SND_DEV_TYPE_SPDIF_PASSTHROUGH = SND_DEV_TYPE_BASE + 6;
	public static final int SND_DEV_TYPE_DEFAULT = SND_DEV_TYPE_BASE;
	public static final int SND_PCM_STREAM_PLAYBACK = 0;
	public static final int SND_PCM_STREAM_CAPTURE = 1;

	public static final int SND_DEVICE_MODE_BASE = 0;
	public static final int SND_DEVICE_MUTE = SND_DEVICE_MODE_BASE + 1;
	public static final int SND_DEVICE_PCM = SND_DEVICE_MODE_BASE +2;
	public static final int SND_DEVICE_BITSTREAM = SND_DEVICE_MODE_BASE + 3;
	public static final int SND_DEVICE_BITSTREAM_5POINT1 = SND_DEVICE_MODE_BASE + 4;
	public static final int SND_DEVICE_BITSTREAM_7POINT1 = SND_DEVICE_MODE_BASE + 5;
	public static final int SND_DEVICE_AUTO_IDENTIFY = SND_DEVICE_MODE_BASE + 6;
	public static final int SND_DEVICE_MANUAL_IDENTIFY = SND_DEVICE_MODE_BASE + 7;

	private static AudioManager mAudioManager = null;
	/*
	 * at the moment ,if the audio device is not usb audio. we recorgnise it as
	 * soc audio device. soc audio must be at the "0" place.
	 */
	public static String getCurrentPlaybackDevice() {
		// Slog.v(TAG, "mCurPlaybackDevice: "+mCurPlaybackDevice);
		// return mCurPlaybackDevice;
		return SystemProperties.get(HW_AUDIO_CURRENTPLAYBACK, "0");
	}

	public static String getCurrentCaptureDevice() {
		// Slog.v(TAG, "mCurCaptureDevice: "+mCurCaptureDevice);
		// return mCurCaptureDevice;
		return SystemProperties.get(HW_AUDIO_CURRENTCAPTURE, "0");
	}

	public static void setCurrentPlaybackDevice(String str) {
		// WiredAccessoryObserver.mCurPlaybackDevice = str;
		SystemProperties.set(HW_AUDIO_CURRENTPLAYBACK, str);
	}

	public static void setCurrentCaptureDevice(String str) {
		// mCurCaptureDevice = str;
		SystemProperties.set(HW_AUDIO_CURRENTCAPTURE, str);
	}

	public static void setLastSocPlayback(String str) {
		SystemProperties.set(HW_AUDIO_LASTSOCPLAYBACK, str);
	}

	public static String getLastSocPlayback() {
		return SystemProperties.get(HW_AUDIO_LASTSOCPLAYBACK, "0");
	}

	public static void setDeviceConnectionState(Context ctx, int device,
			int state) {
		if (mAudioManager == null)
			mAudioManager = (AudioManager) ctx
					.getSystemService(Context.AUDIO_SERVICE);

		mAudioManager.setWiredDeviceConnectionState(device, state, "");

	}

	public static boolean isHdmiAutoIdentify() {
		if(SystemProperties.get(HW_AUDIO_HDMI_AUTO_IDENTIFY, "false").equals("true"))
			return true;
		else
			return false;
	}
	public static int getHdmiOutputMode() {
		if (SystemProperties.get(HW_AUDIO_HDMI_MUTE, "false").equals("true")) {
			return SND_DEVICE_MUTE;
		} else if (SystemProperties.get(HW_AUDIO_HDMI_BYPASS,"false").equals("true")) {
			if (SystemProperties.get(HW_AUDIO_HDMI_BITSTREAM_CHANNELS, "7.1").equals("5.1"))
				return SND_DEVICE_BITSTREAM_5POINT1;
			else if (SystemProperties.get(HW_AUDIO_HDMI_BITSTREAM_CHANNELS, "7.1").equals("7.1"))
				return SND_DEVICE_BITSTREAM_7POINT1;
		} else if (SystemProperties.get(HW_AUDIO_HDMI_BYPASS,"false").equals("false")) {
			return SND_DEVICE_PCM;
		}
		return SND_DEVICE_MODE_BASE;
	}

	public static void setHdmiOutputMode(Context ctx, int mode) {
		switch (mode) {
		case SND_DEVICE_MUTE:
			SystemProperties.set(HW_AUDIO_HDMI_MUTE, "true");
			break;
		case SND_DEVICE_PCM:
			SystemProperties.set(HW_AUDIO_HDMI_MUTE, "false");
			SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "false");
			SystemProperties.set(HW_AUDIO_HDMI_BYPASS, "false");
			doAudioDevicesRouting(ctx, SND_DEV_TYPE_DEFAULT, SND_PCM_STREAM_PLAYBACK, "0");
			break;
		case SND_DEVICE_BITSTREAM_5POINT1:
			SystemProperties.set(HW_AUDIO_HDMI_MUTE, "false");
			SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_HDMI_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_HDMI_BITSTREAM_CHANNELS, "5.1");
			doAudioDevicesRouting(ctx, SND_DEV_TYPE_HDMI_PASSTHROUGH, SND_PCM_STREAM_PLAYBACK, HDMI_PASSTHROUGH_KEY);
			break;
		case SND_DEVICE_BITSTREAM_7POINT1:
			SystemProperties.set(HW_AUDIO_HDMI_MUTE, "false");
			SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_HDMI_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_HDMI_BITSTREAM_CHANNELS, "7.1");
			doAudioDevicesRouting(ctx, SND_DEV_TYPE_HDMI_PASSTHROUGH, SND_PCM_STREAM_PLAYBACK, HDMI_PASSTHROUGH_KEY);
			break;
		case SND_DEVICE_AUTO_IDENTIFY:
			SystemProperties.set(HW_AUDIO_HDMI_MUTE, "false");
			SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_HDMI_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_HDMI_AUTO_IDENTIFY, "true");
			doAudioDevicesRouting(ctx, SND_DEV_TYPE_HDMI_PASSTHROUGH, SND_PCM_STREAM_PLAYBACK, HDMI_PASSTHROUGH_KEY);
			break;
		case SND_DEVICE_MANUAL_IDENTIFY:
			SystemProperties.set(HW_AUDIO_HDMI_AUTO_IDENTIFY, "false");
		default:
			break;
		}
	}

	public static int getSpdifOutputMode() {
		if (SystemProperties.get(HW_AUDIO_SPDIF_MUTE, "false").equals("true"))
			return SND_DEVICE_MUTE;
		else if (SystemProperties.get(HW_AUDIO_SPDIF_BYPASS, "false").equals("false"))
			return SND_DEVICE_PCM;
		else if (SystemProperties.get(HW_AUDIO_SPDIF_BYPASS, "false").equals("true"))
			return SND_DEVICE_BITSTREAM;
		return SND_DEVICE_MODE_BASE;
	}

	public static void setSpdifOutputMode(Context ctx, int mode) {
		switch (mode) {
		case SND_DEVICE_MUTE:
			SystemProperties.set(HW_AUDIO_SPDIF_MUTE, "true");
			break;
		case SND_DEVICE_PCM:
			SystemProperties.set(HW_AUDIO_SPDIF_MUTE, "false");
			SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "false");
			SystemProperties.set(HW_AUDIO_SPDIF_BYPASS, "false");
			doAudioDevicesRouting(ctx, SND_DEV_TYPE_DEFAULT, SND_PCM_STREAM_PLAYBACK, "0");
			break;
		case SND_DEVICE_BITSTREAM:
			SystemProperties.set(HW_AUDIO_SPDIF_MUTE, "false");
			SystemProperties.set(MEDIA_CFG_AUDIO_BYPASS, "true");
			SystemProperties.set(HW_AUDIO_SPDIF_BYPASS, "true");
			doAudioDevicesRouting(ctx, SND_DEV_TYPE_SPDIF_PASSTHROUGH, SND_PCM_STREAM_PLAYBACK, SPDIF_PASSTHROUGH_KEY);
			break;
		default:
			break;

		}
	}
	public static boolean hasSpdif(){

		boolean hasspdif = false;

		String CardsPath = "/proc/asound/cards";
		FileReader cards_fr = null;
		BufferedReader cards_br = null;
		try {
			try {
				cards_fr = new FileReader(CardsPath);
			} catch (FileNotFoundException e) {
				// TODO: handle exception
				e.printStackTrace();
			}
			cards_br = new BufferedReader(cards_fr);
			String Line;
			// cards
			while ((Line = cards_br.readLine()) != null) {
				int pos = Line.lastIndexOf(" - ");
				if (pos > 0) {
					pos = Line.indexOf("SPDIF");
					if (pos > 0) {
						hasspdif = true;
						break;
					}
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				if (cards_br != null)
					cards_br.close();
				if (cards_fr != null)
					cards_fr.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

		return hasspdif;

	}

	/*
	 * currently, we just deal with usb audio, spdif devices routing. ofcourse,
	 * it can be extended.
	 */
	public static void doAudioDevicesRouting(Context ctx, int deviceType,
			int streamType, String state) {

		switch (deviceType) {

		case SND_DEV_TYPE_SPDIF_PASSTHROUGH:
			//doUsbAudioDevicesRouting(streamType, "-1");
			if (streamType == SND_PCM_STREAM_PLAYBACK) {
				setDeviceConnectionState(ctx, AudioManager.DEVICE_OUT_AUX_DIGITAL, 0);
				setDeviceConnectionState(ctx, AudioManager.DEVICE_OUT_SPDIF, 1);
				setLastSocPlayback(state);
			}
			break;
            
		case SND_DEV_TYPE_HDMI_PASSTHROUGH:
			if(streamType == SND_PCM_STREAM_PLAYBACK){
				setDeviceConnectionState(ctx,AudioManager.DEVICE_OUT_SPDIF, 0);
				setDeviceConnectionState(ctx,AudioManager.DEVICE_OUT_AUX_DIGITAL, 1);
				setLastSocPlayback(state);

			}			
		break;

		case SND_DEV_TYPE_HDMI_MULTILPCM:
			if(streamType == SND_PCM_STREAM_PLAYBACK){
				setDeviceConnectionState(ctx,AudioManager.DEVICE_OUT_SPDIF, 0);
				setDeviceConnectionState(ctx,
						AudioManager.DEVICE_OUT_AUX_DIGITAL, 1);
				setLastSocPlayback(state);

			}
		break;
		
		default:
			Slog.i(TAG, "Default");
			if (streamType == SND_PCM_STREAM_PLAYBACK) {
				int connected = 0;
				if(hasSpdif()){
					Slog.i(TAG, "has spdif.");
					connected = 1;
				}

				setDeviceConnectionState(ctx,AudioManager.DEVICE_OUT_SPDIF, connected);
				setDeviceConnectionState(ctx,
						AudioManager.DEVICE_OUT_AUX_DIGITAL, 1);
				setLastSocPlayback(state);

			}
			break;
		}

		if (deviceType == SND_DEV_TYPE_DEFAULT)
			setCurrentPlaybackDevice("0");
		else
			setCurrentPlaybackDevice(state);

		ActivityManagerNative.broadcastStickyIntent(new Intent(
				AUDIOPCMLISTUPDATE), null, UserHandle.USER_ALL);
	}

	public static void doUsbAudioDevicesRouting(int streamType, String state) {
		FileWriter fw;
		int card = Integer.parseInt(state);
		if (card > 0)
			card = card * 10;
		state = Integer.toString(card);
		switch (streamType) {
		case SND_PCM_STREAM_PLAYBACK:
			try {
				fw = new FileWriter(USB_AUDIO_PLAYBACK_SWITCH_STATE_FILE);
				fw.write(state);
				fw.close();
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			} catch (Exception e) {
				e.printStackTrace();
			}
			break;
		case SND_PCM_STREAM_CAPTURE:
			try {
				fw = new FileWriter(USB_AUDIO_CAPTURE_SWITCH_STATE_FILE);
				fw.write(state);
				fw.close();
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			} catch (Exception e) {
				e.printStackTrace();
			}
			break;

		default:
			Slog.e(TAG, "unknown exception!");
			break;
		}
	}

}

