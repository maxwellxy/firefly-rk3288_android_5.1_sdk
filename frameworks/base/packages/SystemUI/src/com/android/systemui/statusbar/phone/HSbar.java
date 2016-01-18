package com.android.systemui.statusbar.phone;

import java.util.List;

import com.android.systemui.statusbar.BaseStatusBar;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.widget.HorizontalScrollView;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.content.Intent;
import com.android.systemui.R;
import android.util.Log;
import android.util.AttributeSet;

public class HSbar extends HorizontalScrollView {

	Context mContext;
	private static  int ICON_WIDTH = 0;
	private static  int ICON_HEIGHT = 0;
	private PanelBar mBar = null;
	private static final String TAG = "HSbar";
	private static final boolean DEBUG = true;
	private void LOGD(String msg){
		if(DEBUG){
			Log.d(TAG,msg);
		}
	}
	public HSbar(Context context, AttributeSet attrs){
		super(context,attrs);
		init(context);
	}
	public HSbar(Context context,BaseStatusBar tabletStatusBar) {
		super(context);
		init(context);
	}

	public void setBar(PanelBar bar){
		mBar = bar;
	}
	private void  init(Context context){
		mContext = context;
		ICON_WIDTH = (int)context.getResources().getDimension(R.dimen.app_bar_item_width);
				ICON_HEIGHT = (int)context.getResources().getDimension(R.dimen.app_bar_item_height);
				LOGD("app_bar_item_size=("+ICON_WIDTH+","+ICON_HEIGHT+")");
				LinearLayout ll = new LinearLayout(context);
				this.addView(ll);
				PackageManager pManager = context.getPackageManager();
				List<PackageInfo> packages = pManager.getInstalledPackages(0);
				for(int i=0;i<packages.size();i++) { 
					PackageInfo packageInfo = packages.get(i); 
					AppInfo tmpInfo = new AppInfo();
					tmpInfo.appName = "11";//packageInfo.applicationInfo.loadLabel(context.getPackageManager()).toString();
					tmpInfo.packageName = packageInfo.packageName; 
					tmpInfo.versionName = packageInfo.versionName;
					tmpInfo.versionCode = packageInfo.versionCode;
					tmpInfo.flags = packageInfo.applicationInfo.flags;
					tmpInfo.appIcon = packageInfo.applicationInfo.loadIcon(context.getPackageManager());
					if(ifAppNeedGrab(tmpInfo)){
						ImageButton ib = new ImageButton(context);
						ib.setImageDrawable(tmpInfo.appIcon);
						ib.setBackgroundResource(R.drawable.hsbar_button_bg);
						ib.setTag(tmpInfo);
						ib.setOnClickListener(mClickListener);
						LayoutParams lParams = new LayoutParams(ICON_WIDTH,ICON_HEIGHT);
						ll.addView(ib,lParams); 		
					}				
				}

	}
	private boolean ifAppNeedGrab(AppInfo info){
		boolean ret = (info.flags & ApplicationInfo.FLAG_SUPPORT_HALF_SCREEN) !=0;
		return ret;
	}
	private OnClickListener mClickListener = new OnClickListener() {
		@Override
		public void onClick(View v) {
			if(mBar != null){
				LOGD("collapseAppPanels");
				mBar.collapseAllPanels(true);
			}
			ImageButton ib = (ImageButton)v;
			AppInfo info = (AppInfo) ib.getTag();
			Intent intent = new Intent();			
			LOGD("add intent.flag_use_half_screen flag");
			PackageManager manager = mContext.getPackageManager();
			intent = manager.getLaunchIntentForPackage(info.packageName);
			//keep equals with launcher app icon 
			intent.setPackage(null);
//			intent.addFlags(Intent.FLAG_USE_HALF_SCREEN);
			intent.addCategory(Intent.CATEGORY_LAUNCHER);
			mContext.startActivity(intent);

		}
	};
}
