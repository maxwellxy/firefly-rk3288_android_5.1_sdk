package com.android.server.wm.remotecontrol;

import java.lang.reflect.Method;

import android.content.Context;
import android.media.AudioManager;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import com.android.server.wm.WindowManagerService;
import com.android.internal.statusbar.IStatusBarService;

public class SoftKeyRequestListener implements ControlSocket.RequestListener{
	private static final String TAG = "SoftKeyRequestListener";
	private static final boolean DEBUG = true;
	private void LOG(String msg){
		if (DEBUG) {
			Log.d(TAG,msg);
		}
	}
	private static final int LEFTCLICK_KEYCODE = -101;
	private static final int CAPSLOCK_ON_KEYCODE = -49;
	private static final int CAPSLOCK_OFF_KEYCODE = -50;
	private static final int MUTE_KEYCODE = 91;
	
	private static final int TV_JOG_N_KEYCODE = 115;
	private static final int TV_JOG_P_KEYCODE = 116;
	
	private static final int F1_KEYCODE = 117;
	private static final int F2_KEYCODE = 118;
	private static final int F3_KEYCODE = 119;
	private static final int F4_KEYCODE = 120;
	private static final int F5_KEYCODE = 121;
	private static final int F6_KEYCODE = 122;
	private static final int F7_KEYCODE = 123;
	private static final int F8_KEYCODE = 124;

	private static final int TV_MEDIA_STOP_KEYCODE = 131;
	private static final int TV_MEDIA_MULT_FORWARD_KEYCODE = 136;
	private static final int TV_MEDIA_MULT_BACKWARD_KEYCODE = 137;
	private static final int TV_MEDIA_PLAY_KEYCODE = 138;
	private static final int TV_MEDIA_PAUSE_KEYCODE = 139;
	
	private static final int RECENTAPP_KEYCODE = 239;
	
	private Context mContext;
	private WindowManagerService mService;
	private MouseManager mMouseManager;
	private IStatusBarService mStatusBarService;
	
	public SoftKeyRequestListener(Context context,WindowManagerService service){
		mContext = context;
		mService = service;
	}
	
	private synchronized IStatusBarService getStatusBarService() {
        if (mStatusBarService == null) {
            mStatusBarService = IStatusBarService.Stub.asInterface(
                    ServiceManager.getService(Context.STATUS_BAR_SERVICE));
            if (mStatusBarService == null) {
                LOG("warning: no STATUS_BAR_SERVICE");
            }
        }
        return mStatusBarService;
    }
	
	@Override
	public void requestRecieved(UDPPacket packet) {
		// TODO Auto-generated method stub
		
		SoftKeyControlRequest request = new SoftKeyControlRequest(packet);

		int keyCode = request.getKeyCode();
		LOG("keycode:"+keyCode+",capsOn:"+request.isCapsOn()+" received at:"+System.currentTimeMillis());
		keyCode = changeKeyCodeBySdkVersion(keyCode);
		LOG("change KeyCode by sdk:"+keyCode);
		if(handleSelfKey(mContext, keyCode)){
			return;
		}
		
		long now = SystemClock.uptimeMillis(); 
		int metaState = 0;
		if (request.isCapsOn()){
			metaState = KeyEvent.META_CAPS_LOCK_ON;
		}
		
		if (keyCode == KeyEvent.KEYCODE_POWER && !request.isLongPress()){
			PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
			if (!pm.isScreenOn()){
				keyCode = KeyEvent.KEYCODE_NOTIFICATION;
			}
		}
		KeyEvent keyDown = new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0, metaState);
		
		
		mService.injectKeyEvent(keyDown, false);
		KeyEvent keyUp;
		long longPressTime = ViewConfiguration.getLongPressTimeout();
		if(request.isLongPress()){
			LOG("LongPress");
			try{
				Thread.sleep(longPressTime+100);
			}catch (Exception e) {
				// TODO: handle exception
			}
			now = SystemClock.uptimeMillis(); 
			keyUp = new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0, metaState);
		}else {
			now = SystemClock.uptimeMillis();
			keyUp = new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0, metaState);
		}
		mService.injectKeyEvent(keyUp, false);

	}

	
	private boolean handleSelfKey(Context context, int keyCode){
		if (keyCode < 0){
			switch (keyCode) {
			case LEFTCLICK_KEYCODE:
				
				long now = SystemClock.uptimeMillis();
				float mouseX = mMouseManager.getMouseX();
				float mouseY = mMouseManager.getMouseY();
				MotionEvent down = MotionEvent.obtain(now, now, MotionEvent.ACTION_DOWN, mouseX, mouseY, 0);
				MotionEvent up = MotionEvent.obtain(now, now, MotionEvent.ACTION_UP, mouseX, mouseY, 0);
				
				mService.injectPointerEvent(down, true);
				mService.injectPointerEvent(up, true);
				return true;
				
			default:
				break;
			}
		}
		
		if (keyCode == RECENTAPP_KEYCODE) {
			IStatusBarService BarService = getStatusBarService();
			if (BarService != null) {
				try {
					BarService.toggleRecentApps();
				} catch (RemoteException e) {
					// TODO: handle exception
					LOG("StatusBarService exception:"+e);
				}
			}
		}
		return false;
	}
	
	public void setMouseManager(MouseManager mouseManager){
		mMouseManager = mouseManager;
	}
	
	private int changeKeyCodeBySdkVersion(int keycode){
		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH){
			switch (keycode) {
//			case CAPSLOCK_OFF_KEYCODE:
//			case CAPSLOCK_ON_KEYCODE:
//				keycode = 115;
//				return keycode;
			
			case F1_KEYCODE:
			case F2_KEYCODE:
			case F3_KEYCODE:
			case F4_KEYCODE:
			case F5_KEYCODE:
			case F6_KEYCODE:
			case F7_KEYCODE:
			case F8_KEYCODE:
				keycode = keycode + 14;
				return keycode;
				
			case TV_MEDIA_MULT_BACKWARD_KEYCODE:
			case TV_MEDIA_MULT_FORWARD_KEYCODE:
			case TV_MEDIA_PAUSE_KEYCODE:
			case TV_MEDIA_PLAY_KEYCODE:
			case TV_MEDIA_STOP_KEYCODE:
				keycode = keycode + 90;
				return keycode;
				
			case MUTE_KEYCODE:
				keycode = 164;
				return keycode;

			case TV_JOG_N_KEYCODE:
			case TV_JOG_P_KEYCODE:
				keycode = keycode + 7;
				return keycode;
				
			default:
				break;
			}
		}
		return keycode;
	}
}
