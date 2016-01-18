package com.android.systemui.statusbar.minwindow;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import android.app.ActivityManager;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.View.OnTouchListener;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;

import com.android.systemui.R;

public class MinWindow {
	public static final String TAG = "MinWindow";

	private static final boolean DEBUG_ZJY = true;

	private static void LOGD(String msg) {
		if (DEBUG_ZJY) {
			Log.d(TAG, "+++++++++++++++" + msg);
		}
	}

	private Context mContext = null;
	private WindowManager wm = null;
	private MinWindowGestureDetector mMinWindowDector = null;
	private HashMap<String, WindowManager.LayoutParams> mParamsMap = new HashMap<String, WindowManager.LayoutParams>();
	private HashMap<String, LinearLayout> imageViewMap = new HashMap<String, LinearLayout>();
	private HashMap<String, Integer> taskMap = new HashMap<String, Integer>();
	
	private int item_width = 70;
	private int item_height = 70;

	public MinWindow(Context context, WindowManager mWm) {
		mContext = context;
		wm = mWm;
		item_width = (int)mContext.getResources().getDimension(R.dimen.min_window_width);
		item_height = (int)mContext.getResources().getDimension(R.dimen.min_window_height);
		if (mMinWindowDector == null) {
			mMinWindowDector = new MinWindowGestureDetector(context,
					new MinWindowGestureDetectorListener());
		}
	}

	public void addPackageNameWindow(String packageName, int clickWindowArea,
			int taskId) {
		if (packageName != null && !"".equals(packageName.trim())) {
			final String str = packageName.trim();
			if (taskMap.get(str) != null) {
				taskMap.remove(str);
			}
			taskMap.put(str, taskId);
			WindowManager.LayoutParams wl = mParamsMap.get(str);
			LinearLayout container = imageViewMap.get(str);
			if (wl == null) {
				wl = new WindowManager.LayoutParams();
				wl.type = WindowManager.LayoutParams.TYPE_MULTIMODE_BUTTON;// 2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT
																			// ;
				wl.format = PixelFormat.TRANSLUCENT;
				wl.flags |= 8;
				wl.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
				wl.width = item_width;
				wl.height = item_height;
				wl.windowAnimations = R.style.minWindowAnim;
				if (clickWindowArea == 0 || clickWindowArea == 1
						|| clickWindowArea == 2 || clickWindowArea == 3) {
					int mode = Settings.System.getInt(
							mContext.getContentResolver(),
							Settings.System.MULITI_WINDOW_MODE,
							Settings.System.MULITI_WINDOW_FULL_SCREEN_MODE);
					if (mode == Settings.System.MULITI_WINDOW_FULL_SCREEN_MODE) {
						wl.x = wm.getDefaultDisplay().getWidth() / 2;
						wl.y = wm.getDefaultDisplay().getHeight() / 2;
					} else if (mode == Settings.System.MULITI_WINDOW_HALF_SCREEN_MODE) {

					} else if (mode == Settings.System.MULITI_WINDOW_FOUR_SCREEN_MODE) {
						String pos = Settings.System.getString(
								mContext.getContentResolver(),
								Settings.System.FOUR_SCREEN_WINDOW_POSITION);
						int x = 0;
						int y = 0;
						if (pos != null) {
							String[] position = pos.split(",");
							x = Integer.parseInt(position[0]);
							y = Integer.parseInt(position[1]);
						}
						if (clickWindowArea == 0) {
							wl.x = x / 2;
							wl.y = y / 2;
						} else if (clickWindowArea == 1) {
							wl.x = x + (wm.getDefaultDisplay().getWidth() - x)
									/ 2;
							wl.y = y / 2;
						} else if (clickWindowArea == 2) {
							wl.x = x / 2;
							wl.y = y + (wm.getDefaultDisplay().getHeight() - y)
									/ 2;
						} else if (clickWindowArea == 3) {
							wl.x = x + (wm.getDefaultDisplay().getWidth() - x)
									/ 2;
							wl.y = y + (wm.getDefaultDisplay().getHeight() - y)
									/ 2;
						}
					}
				} else {
					wl.x = wm.getDefaultDisplay().getWidth();
					wl.y = wm.getDefaultDisplay().getHeight() / 2;
				}
				wl.setTitle("MinWindow:" + str);
				wl.gravity = Gravity.LEFT | Gravity.TOP;
				mParamsMap.put(str, wl);
			}
			if (container == null) {
				container = new LinearLayout(mContext);
				container.setGravity(Gravity.CENTER);
				container.setBackgroundResource(R.drawable.circle);
				ImageView iv = new ImageView(mContext);
				iv.setScaleType(ScaleType.FIT_CENTER);
				iv.setImageDrawable(getPackageDrawable(str));
				LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(item_width/2, item_height/2);
				lp.gravity = Gravity.CENTER;
				container.addView(iv, lp);
				container.setFocusable(true);
				container.setClickable(true);
				container.setOnTouchListener(new OnTouchListener() {
					@Override
					public boolean onTouch(View v, MotionEvent event) {
						mMinWindowDector.onTouchEvent(str, event);
						return true;
					}
				});
				imageViewMap.put(str, container);
			}
			if (container != null && container.getParent() == null) {
				wm.addView(container, wl);
			}
		}
	}

	public void removePackageNameWindow(String packageName) {
		if (packageName != null && !"".equals(packageName.trim())) {
			final String str = packageName.trim();
			WindowManager.LayoutParams wl = mParamsMap.get(str);
			LinearLayout container = imageViewMap.get(str);
			if (container != null && container.getParent() != null) {
				wm.removeView(container);
			}
			imageViewMap.remove(str);
			mParamsMap.remove(str);
		}
	}

	public void removePackageWindows(List<String> list){
		if(list == null){
			return;
		}
		for(int i = 0;i < list.size();i ++){
			String packageName = list.get(i);
			removePackageNameWindow(packageName);
		}
	}

	private Drawable getPackageDrawable(String packageName) {
		Drawable result = null;
		PackageManager pManager = mContext.getPackageManager();
		List<PackageInfo> packages = pManager.getInstalledPackages(0);
		for (int i = 0; i < packages.size(); i++) {
			PackageInfo packageInfo = packages.get(i);
			String name = packageInfo.packageName;
			if (name != null && name.equals(packageName)) {
				result = packageInfo.applicationInfo.loadIcon(mContext
						.getPackageManager());
				break;
			}
		}
		return result;
	}

	private class MinWindowGestureDetector extends GestureDetector {
		private MinWindowGestureDetectorListener mOnGestureListener = null;

		public void onTouchEvent(String name, MotionEvent event) {
			mOnGestureListener.setPackageName(name);
			this.onTouchEvent(event);
		}

		public MinWindowGestureDetector(Context context,
				MinWindowGestureDetectorListener listener) {
			super(context, listener);
			mOnGestureListener = listener;
		}

	}

	private class MinWindowGestureDetectorListener extends
			SimpleOnGestureListener {
		private String packageName = null;

		public void setPackageName(String name) {
			packageName = name;
		}

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			if (packageName == null || "".equals(packageName.trim())) {
				return true;
			}
			WindowManager.LayoutParams wl = mParamsMap.get(packageName);
			LinearLayout container = imageViewMap.get(packageName);
			if (wl == null || container == null
					|| container.getParent() == null) {
				return true;
			}
			int startX = (int) e1.getX();
			int startY = (int) e1.getY();
			int currentX = (int) e2.getRawX();
			int currentY = (int) e2.getRawY();
			wl.x = currentX - startX;
			wl.y = currentY - startY;
			if (container.getParent() != null) {
				wm.updateViewLayout(container, wl);
			}
			return true;
		}

		@Override
		public boolean onSingleTapConfirmed(MotionEvent e) {
			LOGD("onSingleTapUp");
			if (packageName != null) {
				removePackageNameWindow(packageName);
				int taskId = taskMap.get(packageName);
				if (taskId >= 0) {
					final ActivityManager am = (ActivityManager) mContext
							.getSystemService(Context.ACTIVITY_SERVICE);
					am.moveTaskToFront(taskId,
							ActivityManager.MOVE_TASK_WITH_HOME, null);
				} else {
					PackageManager manager = mContext.getPackageManager();
					Intent intent = manager
							.getLaunchIntentForPackage(packageName);
					intent.addFlags(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY
							| Intent.FLAG_ACTIVITY_TASK_ON_HOME
							| Intent.FLAG_ACTIVITY_NEW_TASK);
					try {
						mContext.startActivityAsUser(intent, null,
								new UserHandle(UserHandle.USER_CURRENT));
					} catch (Exception ev) {
						Log.e(TAG, "Error launching activity " + intent, ev);
					}
				}
			}
			return true;
		}

		@Override
		public void onLongPress(MotionEvent e) {
			LOGD("onLongPress");
		}
	}
}
