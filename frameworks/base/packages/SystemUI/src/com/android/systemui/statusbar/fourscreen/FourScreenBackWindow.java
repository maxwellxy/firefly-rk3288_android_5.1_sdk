package com.android.systemui.statusbar.fourscreen;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.provider.Settings;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.Toast;

import com.android.systemui.R;
import com.android.systemui.statusbar.circlemenu.FourScreenCircleMenuView;
import com.android.systemui.statusbar.phone.PhoneStatusBar;

public class FourScreenBackWindow {

	public static final String TAG = "FourScreenBackWindow";

	private static final boolean DEBUG_ZJY = false;

	private static void LOGD(String msg) {
		if (DEBUG_ZJY) {
			Log.d(TAG, "+++++++++++++++" + msg);
		}
	}
	
	private final int ADD_BUTTON_WIDTH = 30;
	private final int ADD_BUTTON_HEIGHT = 30;

	private Context mContext = null;
	private WindowManager wm = null;
	private int mStatusBarHeight = 0;
	private HashMap<Integer, WindowManager.LayoutParams> mParamsMap = new HashMap<Integer, WindowManager.LayoutParams>();
	private HashMap<Integer, ImageView> imageViewMap = new HashMap<Integer, ImageView>();
	
	private HashMap<Integer, WindowManager.LayoutParams> mAddButtonParamsMap = new HashMap<Integer, WindowManager.LayoutParams>();
	private HashMap<Integer, ImageView> mAddButtonImageViewMap = new HashMap<Integer, ImageView>();

	public static final boolean ONLY_ONE_BACK_WINDOW = false;

	public FourScreenBackWindow(Context context, WindowManager mWm) {
		mContext = context;
		wm = mWm;
		mStatusBarHeight = (int) context.getResources().getDimensionPixelSize(
				com.android.internal.R.dimen.status_bar_height);
		addFourScreenBackWindow();
		if(ONLY_ONE_BACK_WINDOW){
			addFourScreenAddButtonWindow();
		}
	}
	
	private void addFourScreenAddButtonWindow(){
		Map map = PhoneStatusBar.areaMap;
		Iterator iter = map.entrySet().iterator();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			final int val = (Integer) entry.getValue();
			if (mAddButtonParamsMap.get(val) == null) {
				WindowManager.LayoutParams wl = new WindowManager.LayoutParams();
				wl.type = WindowManager.LayoutParams.TYPE_MULTIMODE_BUTTON;// 2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT
																			// ;
				wl.format = PixelFormat.TRANSLUCENT;
				wl.flags |= 8;
				wl.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
				wl.setTitle("AddButton");
				wl.gravity = Gravity.LEFT | Gravity.TOP;
				mAddButtonParamsMap.put(val, wl);
			}
			if (mAddButtonImageViewMap.get(val) == null) {
				ImageView iv = new ImageView(mContext);
				// iv.setBackgroundColor(Color.BLUE);
				iv.setBackgroundResource(R.drawable.add_button);
				iv.setScaleType(ScaleType.CENTER);
				iv.setFocusable(true);
				iv.setClickable(true);
				iv.setOnClickListener(new View.OnClickListener() {		
					@Override
					public void onClick(View v) {
						Log.v("zjy","----------add button click--------------area:"+val);
						if(!isWorked("com.android.Listappinfo.ManderService")){
							mContext.startService(new Intent("com.android.ZENGJUN"));
						}
					}
				});
				mAddButtonImageViewMap.put(val, iv);
			}
		}
		updateAddButtonWindowView();
	}
	
	private boolean isWorked(String servicename){
		ActivityManager myManager=(ActivityManager)mContext.getSystemService(mContext.ACTIVITY_SERVICE);
		ArrayList<RunningServiceInfo> runningService = 
			(ArrayList<RunningServiceInfo>) myManager.getRunningServices(60);
		for(int i = 0 ; i<runningService.size();i++){
			if(runningService.get(i).service.getClassName().equals(servicename)){
				return true;
			}
		}
		return false;
	}

	@SuppressWarnings("rawtypes")
	private void addFourScreenBackWindow() {
		if (ONLY_ONE_BACK_WINDOW) {
			int val = -1;
			if (mParamsMap.get(val) == null) {
				WindowManager.LayoutParams wl = new WindowManager.LayoutParams();
				wl.type = WindowManager.LayoutParams.TYPE_MULTI_BACK_WINDOW;// 2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT
																			// ;
				wl.format = PixelFormat.TRANSLUCENT;
				wl.flags |= 8;
				wl.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
				wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
				wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
				wl.setTitle("BackWindow");
				wl.gravity = Gravity.LEFT | Gravity.TOP;
				mParamsMap.put(val, wl);
			}
			if (imageViewMap.get(val) == null) {
				ImageView iv = new ImageView(mContext);
				// iv.setBackgroundColor(Color.BLUE);
				iv.setBackgroundResource(R.drawable.backwindow_bg);
				iv.setScaleType(ScaleType.CENTER);
				iv.setFocusable(true);
				iv.setClickable(true);
				imageViewMap.put(val, iv);
			}
		} else {
			Map map = PhoneStatusBar.areaMap;
			Iterator iter = map.entrySet().iterator();
			while (iter.hasNext()) {
				Map.Entry entry = (Map.Entry) iter.next();
				int val = (Integer) entry.getValue();
				if (mParamsMap.get(val) == null) {
					WindowManager.LayoutParams wl = new WindowManager.LayoutParams();
					wl.type = WindowManager.LayoutParams.TYPE_MULTI_BACK_WINDOW;// 2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT
																				// ;
					wl.format = PixelFormat.TRANSLUCENT;
					wl.flags |= 8;
					wl.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
					wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
					wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
					wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
					wl.setTitle(String.valueOf(val));
					wl.gravity = Gravity.LEFT | Gravity.TOP;
					mParamsMap.put(val, wl);
				}
				if (imageViewMap.get(val) == null) {
					ImageView iv = new ImageView(mContext);
//					 iv.setBackgroundColor(0x9f888888);
					iv.setBackgroundResource(R.drawable.backwindow_bg);
					iv.setScaleType(ScaleType.CENTER);
					iv.setFocusable(true);
					iv.setClickable(true);
					imageViewMap.put(val, iv);
				}
			}
		}
		updateBackWindowView();
	}

	public void updateBackWindowView(int x, int y) {
		LOGD("updateBackWindowView");
		final Map map = mParamsMap;
		Iterator iter = map.entrySet().iterator();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			int key = (Integer) entry.getKey();
			WindowManager.LayoutParams wl = (WindowManager.LayoutParams) entry
					.getValue();
			if (key == -1) {
				wl.x = 0;
				wl.y = mStatusBarHeight;
//				wl.width = wm.getDefaultDisplay().getWidth();
//				wl.height = wm.getDefaultDisplay().getHeight()
//						- mStatusBarHeight;
			} else if (key == 0) {
				wl.x = x - wm.getDefaultDisplay().getWidth();
				wl.y = y - wm.getDefaultDisplay().getHeight() + mStatusBarHeight;
//				wl.width = x;
//				wl.height = y - mStatusBarHeight;
			} else if (key == 1) {
				wl.x = x;
				wl.y = y - wm.getDefaultDisplay().getHeight() + mStatusBarHeight;
//				wl.width = wm.getDefaultDisplay().getWidth() - x;
//				wl.height = y - mStatusBarHeight;
			} else if (key == 2) {
				wl.x = x - wm.getDefaultDisplay().getWidth();
				wl.y = y;
//				wl.width = x;
//				wl.height = wm.getDefaultDisplay().getHeight() - y;
			} else if (key == 3) {
				wl.x = x;
				wl.y = y;
//				wl.width = wm.getDefaultDisplay().getWidth() - x;
//				wl.height = wm.getDefaultDisplay().getHeight() - y;
			}
			wl.width = wm.getDefaultDisplay().getWidth();
			wl.height = wm.getDefaultDisplay().getHeight() - mStatusBarHeight;
			ImageView iv = imageViewMap.get(key);
			if (iv != null && iv.getParent() != null) {
				wm.updateViewLayout(iv, wl);
			}
		}
	}

	public void updateBackWindowView() {
		String pos = Settings.System.getString(mContext.getContentResolver(),
				Settings.System.FOUR_SCREEN_WINDOW_POSITION);
		int x = 0;
		int y = 0;
		if (pos != null) {
			String[] position = pos.split(",");
			x = Integer.parseInt(position[0]);
			y = Integer.parseInt(position[1]);
		}
		updateBackWindowView(x, y);
	}
	
	private void  updateAddButtonWindowView(int x, int y) {
		LOGD("updateAddButtonWindowView");
		final Map map = mAddButtonParamsMap;
		Iterator iter = map.entrySet().iterator();
		int screen_width = wm.getDefaultDisplay().getWidth();
		int screen_height = wm.getDefaultDisplay().getHeight();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			int key = (Integer) entry.getKey();
			WindowManager.LayoutParams wl = (WindowManager.LayoutParams) entry
					.getValue();
			if (key == 0) {
				wl.x = (x - ADD_BUTTON_WIDTH)/2;
				wl.y = (y-ADD_BUTTON_HEIGHT-mStatusBarHeight)/2+mStatusBarHeight;
				wl.width = ADD_BUTTON_WIDTH;
				wl.height = ADD_BUTTON_HEIGHT;
			} else if (key == 1) {
				wl.x = x+(screen_width-x-ADD_BUTTON_WIDTH)/2;
				wl.y = (y-ADD_BUTTON_HEIGHT-mStatusBarHeight)/2+mStatusBarHeight;
				wl.width = ADD_BUTTON_WIDTH;
				wl.height = ADD_BUTTON_HEIGHT;
			} else if (key == 2) {
				wl.x = (x - ADD_BUTTON_WIDTH)/2;
				wl.y = y+(screen_height-y-ADD_BUTTON_HEIGHT)/2;
				wl.width = ADD_BUTTON_WIDTH;
				wl.height = ADD_BUTTON_HEIGHT;
			} else if (key == 3) {
				wl.x = x+(screen_width-x-ADD_BUTTON_WIDTH)/2;
				wl.y = y+(screen_height-y-ADD_BUTTON_HEIGHT)/2;
				wl.width = ADD_BUTTON_WIDTH;
				wl.height = ADD_BUTTON_HEIGHT;
			}
			ImageView iv = mAddButtonImageViewMap.get(key);
			if (iv != null && iv.getParent() != null) {
				wm.updateViewLayout(iv, wl);
			}
		}
	}
	
	public void updateAddButtonWindowView(){
		if(!ONLY_ONE_BACK_WINDOW){
			return;
		}
		String pos = Settings.System.getString(mContext.getContentResolver(),
				Settings.System.FOUR_SCREEN_WINDOW_POSITION);
		int x = 0;
		int y = 0;
		if (pos != null) {
			String[] position = pos.split(",");
			x = Integer.parseInt(position[0]);
			y = Integer.parseInt(position[1]);
		}
		updateAddButtonWindowView(x, y);
	}

	public void addBackWindowView(String areas) {
		LOGD("addBackWindowView areas:" + areas);
		updateBackWindowView();
		ArrayList<Integer> list = new ArrayList<Integer>();
		ArrayList<Integer> arealist = new ArrayList<Integer>();
		if (areas != null && areas.indexOf(",") > 0) {
			String steps[] = areas.split(",");
			for (int i = 0; i < steps.length; i++) {
				if (steps[i] != null) {
					String str = steps[i];
					if (str.equals("0") || str.equals("1") || str.equals("2")
							|| str.equals("3")) {
						if (!ONLY_ONE_BACK_WINDOW) {
							list.add(Integer.valueOf(str));
						} else {
							if(!list.contains(-1))
								list.add(-1);
						}
						arealist.add(Integer.valueOf(str));
					}
				}
			}
		}
		final Map map = imageViewMap;
		Iterator iter = map.entrySet().iterator();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			int key = (Integer) entry.getKey();
			WindowManager.LayoutParams wl = mParamsMap.get(key);
			ImageView iv = imageViewMap.get(key);
			if (list.contains(key)) {
				if (iv != null && iv.getParent() == null) {
					wm.addView(iv, wl);
				}
			} else {
				if (iv != null && iv.getParent() != null) {
					wm.removeView(iv);
				}
			}
		}
		
		if(!ONLY_ONE_BACK_WINDOW){
			return;
		}
		final Map map1 = mAddButtonImageViewMap;
		Iterator iter1 = map1.entrySet().iterator();
		while (iter1.hasNext()) {
			Map.Entry entry = (Map.Entry) iter1.next();
			int key = (Integer) entry.getKey();
			WindowManager.LayoutParams wl = mAddButtonParamsMap.get(key);
			ImageView iv = mAddButtonImageViewMap.get(key);
			if(arealist != null && !arealist.contains(key)
					&& arealist.size() > 0){
				if (iv != null && iv.getParent() == null) {
					wm.addView(iv, wl);
				}
			}else if(arealist == null || arealist.contains(key)
					|| arealist.size() == 0){
				if (iv != null && iv.getParent() != null) {
					wm.removeView(iv);
				}
			}
		}
	}
}
