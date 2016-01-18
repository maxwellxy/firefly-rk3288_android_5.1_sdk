package com.android.systemui.statusbar.circlemenu;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.android.systemui.statusbar.phone.PhoneStatusBar;

import android.content.Context;
import android.graphics.PixelFormat;
import android.provider.Settings;
import android.util.Log;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;

import com.android.systemui.R;

public class FourScreenMenuWindow {
	public static final String TAG = "FourScreenMenuWindow";

	private static final boolean DEBUG_ZJY = true;

	private static void LOGD(String msg) {
		if (DEBUG_ZJY) {
			Log.d(TAG, "+++++++++++++++" + msg);
		}
	}
	
	private int COMPOSER_BTN_WIDTH = 52;
	private int COMPOSER_BTN_HEIGHT = 52;
	private final int DISTANCE = 10;
	
	private Context mContext = null;
	private WindowManager wm = null;
	private int mStatusBarHeight = 0;
	
	private HashMap<Integer, WindowManager.LayoutParams> mParamsMap = new HashMap<Integer, WindowManager.LayoutParams>();
	private HashMap<Integer, LinearLayout> composerBtnMap = new HashMap<Integer, LinearLayout>();
	
	public FourScreenMenuWindow(Context context, WindowManager mWm) {
		mContext = context;
		wm = mWm;
		mStatusBarHeight = (int) context.getResources().getDimensionPixelSize(
				com.android.internal.R.dimen.status_bar_height);
		COMPOSER_BTN_WIDTH = (int)context.getResources().getDimension(R.dimen.center_btn_width);
		COMPOSER_BTN_HEIGHT = (int)context.getResources().getDimension(R.dimen.center_btn_height);
		addFourScreenMenuWindow();
	}
	
	private void addFourScreenMenuWindow() {
		Map map = PhoneStatusBar.areaMap;
		Iterator iter = map.entrySet().iterator();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			int val = (Integer) entry.getValue();
			if (mParamsMap.get(val) == null) {
				WindowManager.LayoutParams wl = new WindowManager.LayoutParams();
				wl.type = WindowManager.LayoutParams.TYPE_MULTIWINDOW_FOURSCREEN_CENTER_BUTTON;// 2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT
				wl.format = PixelFormat.TRANSLUCENT;
				wl.flags |= 8;
				wl.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
				wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
				wl.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
				wl.setTitle("BackWindow");
				wl.gravity = Gravity.LEFT | Gravity.TOP;
				mParamsMap.put(val, wl);
			}
			if (composerBtnMap.get(val) == null) {
				LinearLayout ll = new LinearLayout(mContext);
				ImageView iv = new ImageView(mContext);
				iv.setLayoutParams(new LinearLayout.LayoutParams(COMPOSER_BTN_WIDTH,COMPOSER_BTN_HEIGHT));
//				iv.setBackgroundResource(R.drawable.composer_menu_button);
				iv.setScaleType(ScaleType.CENTER);
				ll.addView(iv);
				ll.setFocusable(true);
				ll.setClickable(true);
				composerBtnMap.put(val, ll);
			}
		}
		updateComposerBtnView();
	}
	
	public void updateComposerBtnView() {
		String pos = Settings.System.getString(mContext.getContentResolver(),
				Settings.System.FOUR_SCREEN_WINDOW_POSITION);
		int x = 0;
		int y = 0;
		if (pos != null) {
			String[] position = pos.split(",");
			x = Integer.parseInt(position[0]);
			y = Integer.parseInt(position[1]);
		}
		updateComposerBtnView(x, y);
	}
	
	private void updateComposerBtnView(int x, int y) {
		LOGD("updateComposerBtnView");
		final Map map = mParamsMap;
		Iterator iter = map.entrySet().iterator();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			int key = (Integer) entry.getKey();
			WindowManager.LayoutParams wl = (WindowManager.LayoutParams) entry
					.getValue();
			if (key == 0) {
				wl.x = x - COMPOSER_BTN_WIDTH - DISTANCE;
				wl.y = y - COMPOSER_BTN_HEIGHT - DISTANCE;
				wl.width = COMPOSER_BTN_WIDTH;
				wl.height = COMPOSER_BTN_HEIGHT;
			} else if (key == 1) {
				wl.x = x + DISTANCE;
				wl.y = y - COMPOSER_BTN_HEIGHT - DISTANCE;
				wl.width = COMPOSER_BTN_WIDTH;
				wl.height = COMPOSER_BTN_HEIGHT;
			} else if (key == 2) {
				wl.x = x - COMPOSER_BTN_WIDTH - DISTANCE;
				wl.y = y + DISTANCE;
				wl.width = COMPOSER_BTN_WIDTH;
				wl.height = COMPOSER_BTN_HEIGHT;
			} else if (key == 3) {
				wl.x = x + DISTANCE;
				wl.y = y + DISTANCE;
				wl.width = COMPOSER_BTN_WIDTH;
				wl.height = COMPOSER_BTN_HEIGHT;
			}
			LinearLayout ll = composerBtnMap.get(key);
			if (ll != null && ll.getParent() != null) {
				wm.updateViewLayout(ll, wl);
			}
		}
	}
	
	public void addBackWindowView(String areas) {
		LOGD("addBackWindowView areas:" + areas);
		updateComposerBtnView();
		ArrayList<Integer> arealist = new ArrayList<Integer>();
		if (areas != null && areas.indexOf(",") > 0) {
			String steps[] = areas.split(",");
			for (int i = 0; i < steps.length; i++) {
				if (steps[i] != null) {
					String str = steps[i];
					if (str.equals("0") || str.equals("1") || str.equals("2")
							|| str.equals("3")) {
						arealist.add(Integer.valueOf(str));
					}
				}
			}
		}
		
		final Map map = composerBtnMap;
		Iterator iter = map.entrySet().iterator();
		while (iter.hasNext()) {
			Map.Entry entry = (Map.Entry) iter.next();
			int key = (Integer) entry.getKey();
			WindowManager.LayoutParams wl = mParamsMap.get(key);
			LinearLayout ll = composerBtnMap.get(key);
			if (arealist.contains(key)) {
				if (ll != null && ll.getParent() == null) {
					wm.addView(ll, wl);
				}
			} else {
				if (ll != null && ll.getParent() != null) {
					wm.removeView(ll);
				}
			}
		}
	}
}
