package com.android.systemui.statusbar.sideslip;

import java.util.List;

import android.app.ActivityManager;
import android.app.ActivityOptions;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;

public class SideSlipWindow {
	public static final String TAG = "SideSlipWindow";
	private static final boolean DEBUG_ZJY = true;
	private static void LOGD(String msg){
		if(DEBUG_ZJY){
			Log.d(TAG, msg);
		}
	}

	private static WindowManager.LayoutParams mSideSlipParams;
	private static ImageView mSideSlipView;
	
	private WindowManager mWindowManager = null;
	private Context mContext;
	
	private int DEFAULT_X = 0;
	private int DEFAULT_Y = 0;
	private int VIEW_WIDTH = 280;
	private int VIEW_HEIGHT = 180;
	
	private ColorDrawableWithDimensions mDefaultThumbnailBackground;
	private Drawable thumbnailViewDrawable = null;
	private int mTaskId = -1;
	private Intent mIntent = null;
	
	public SideSlipWindow(WindowManager wm,Context context){
		mWindowManager = wm;
		mContext = context;
		DEFAULT_X = -mWindowManager.getDefaultDisplay().getWidth();
		DEFAULT_Y =  -mWindowManager.getDefaultDisplay().getHeight()/2;
		int thumbnailWidth =
                (int) context.getResources().getDimensionPixelSize(com.android.internal.R.dimen.thumbnail_width);
        int thumbnailHeight =
                (int) context.getResources().getDimensionPixelSize(com.android.internal.R.dimen.thumbnail_height);
        int color = Color.parseColor("#88000000");
		mDefaultThumbnailBackground =
                new ColorDrawableWithDimensions(color, thumbnailWidth, thumbnailHeight);
		addSideSlipWindow();
	}
	
	private void addSideSlipWindow(){
		if(mSideSlipView == null){
			mSideSlipView = new ImageView(mContext);
			mSideSlipView.setScaleType(ScaleType.CENTER);
			mSideSlipView.setOnTouchListener(new View.OnTouchListener() {				
				public boolean onTouch(View v, MotionEvent event) {
					switch(event.getAction()){
						case MotionEvent.ACTION_UP:
							LOGD("action Up SideSlipWindow");
							hideView();							
							break;
						case MotionEvent.ACTION_DOWN:
							LOGD("action down SideSlipWindow");
							break;
						}
					return true;
				}
			});
		}
		if(mSideSlipParams == null){
			mSideSlipParams = new WindowManager.LayoutParams();
			mSideSlipParams.type=WindowManager.LayoutParams.TYPE_MULTIMODE_BUTTON;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mSideSlipParams.format=PixelFormat.RGBA_8888;
			mSideSlipParams.flags|=8;
			mSideSlipParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mSideSlipParams.width = VIEW_WIDTH;
			mSideSlipParams.height = VIEW_HEIGHT;
			mSideSlipParams.x = DEFAULT_X;
			mSideSlipParams.y = DEFAULT_Y;
			mSideSlipParams.setTitle("SideSlipWindow");
			mSideSlipParams.gravity=Gravity.LEFT|Gravity.TOP;

		}
	}
	
	public void onTouchEvent(MotionEvent ev){
		if((ev.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_UP){
			if(mSideSlipView == null || mSideSlipView.getParent() == null){
				return;
			}
			Bitmap bm = null;
	        boolean usingDrawingCache = true;
	        final ActivityManager am = (ActivityManager)
	                mContext.getSystemService(Context.ACTIVITY_SERVICE);
	        if (thumbnailViewDrawable instanceof BitmapDrawable) {
	            bm = ((BitmapDrawable) thumbnailViewDrawable).getBitmap();
	        }
	        Bundle opts = (bm == null) ?
	                null :
	                ActivityOptions.makeThumbnailScaleUpAnimation(
	                		mSideSlipView, bm, 0, 0, null).toBundle();
	        Log.v("zjy", "----------------------------------mTaskId:"+mTaskId+",:"+(bm == null));
	        
			hideView();
			Intent intent = mIntent;
			intent.addFlags(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY
					| Intent.FLAG_ACTIVITY_TASK_ON_HOME
					| Intent.FLAG_ACTIVITY_NEW_TASK);
			try {
				mContext.startActivityAsUser(intent, opts, new UserHandle(
						UserHandle.USER_CURRENT));
			} catch (SecurityException e) {
				Log.e(TAG, "Recents does not have the permission to launch "
						+ intent, e);
			} catch (ActivityNotFoundException e) {
				Log.e(TAG, "Error launching activity " + intent, e);
			}
		}else {
			int x = (int)ev.getRawX();
			int y = (int)ev.getRawY();
			LOGD("=========== x:"+x+",y:"+y);
			if(mSideSlipParams != null){
				mSideSlipParams.x = x-VIEW_WIDTH/2;
				mSideSlipParams.y = y-VIEW_HEIGHT/2;
			}
			showView();
		}
		if(mSideSlipView != null){
			mSideSlipView.onTouchEvent(ev);
		}
	}
	
	public void showView(){
		if(mSideSlipView != null && mSideSlipView.getParent()==null){
			mSideSlipView.setBackground(loadFirstTaskThumbnail());
			mWindowManager.addView(mSideSlipView, mSideSlipParams);
		}else if(mSideSlipView != null && mSideSlipView.getParent() != null){
			mWindowManager.updateViewLayout(mSideSlipView, mSideSlipParams);
		}
	}
	
	public void hideView(){
		if(mSideSlipParams != null){
			mSideSlipParams.x = DEFAULT_X;
			mSideSlipParams.y = DEFAULT_Y;
		}
		if(mSideSlipView != null && mSideSlipView.getParent() != null){
			mWindowManager.removeView(mSideSlipView);
		}	
	}
	
	public int getFirstTaskIndex(){
		final ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        final List<ActivityManager.RecentTaskInfo> recentTasks = am.getRecentTasksForUser(
                10, ActivityManager.RECENT_IGNORE_UNAVAILABLE, UserHandle.CURRENT.getIdentifier());
        int result = -1;
        for(int i = 0;i < recentTasks.size();i++){
        	ActivityManager.RecentTaskInfo recentInfo = recentTasks.get(i);
        	Intent intent = new Intent(recentInfo.baseIntent);
        	if (recentInfo.origActivity != null) {
                intent.setComponent(recentInfo.origActivity);
            }
        	if (isCurrentHomeActivity(intent.getComponent(), null)) {
        		continue;
            }

            if (intent.getComponent().getPackageName().equals(mContext.getPackageName())) {
            	continue;
            }
            result = i;
            break;
        }
        return result;
	}
	
	public Drawable loadFirstTaskThumbnail(){
		final ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);

        final List<ActivityManager.RecentTaskInfo> recentTasks = am.getRecentTasksForUser(
                10, ActivityManager.RECENT_IGNORE_UNAVAILABLE, UserHandle.CURRENT.getIdentifier());
        int index = getFirstTaskIndex();
        if (index != -1) {
        	mTaskId = recentTasks.get(index).persistentId;
        	mIntent = recentTasks.get(index).baseIntent;
        	Bitmap thumbnail = am.getTaskThumbnail(mTaskId).mainThumbnail;
        	if (thumbnail == null) {
        		Log.v("zjy", "----------thumbnail is null-------------mTaskId:"+mTaskId);
        		return mDefaultThumbnailBackground;
        	}
        	thumbnailViewDrawable = new BitmapDrawable(mContext.getResources(), thumbnail);
        	return thumbnailViewDrawable;
        }
        return null;
	}
	
	private boolean isCurrentHomeActivity(ComponentName component, ActivityInfo homeInfo) {
        if (homeInfo == null) {
            final PackageManager pm = mContext.getPackageManager();
            homeInfo = new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME)
                .resolveActivityInfo(pm, 0);
        }
        return homeInfo != null
            && homeInfo.packageName.equals(component.getPackageName())
            && homeInfo.name.equals(component.getClassName());
    }
	
	class ColorDrawableWithDimensions extends ColorDrawable {
	    private int mWidth;
	    private int mHeight;

	    public ColorDrawableWithDimensions(int color, int width, int height) {
	        super(color);
	        mWidth = width;
	        mHeight = height;
	    }

	    @Override
	    public int getIntrinsicWidth() {
	        return mWidth;
	    }

	    @Override
	    public int getIntrinsicHeight() {
	        return mHeight;
	    }
	}
}
