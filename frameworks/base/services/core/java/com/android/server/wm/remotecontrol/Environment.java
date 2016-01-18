/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    Environment.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-21 02:16:12  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-21      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Point;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

public class Environment {

	/**
     * The configurations are managed in a singleton.
     */
    private static Environment mInstance;
    private Configuration mConfig = new Configuration();
    private int mScreenWidth;
    private int mScreenHeight;
    private DisplayMetrics mDisplayMetrics;
    
    private Environment() {
    }

    public static Environment getInstance() {
        if (null == mInstance) {
            mInstance = new Environment();
        }
        return mInstance;
    }
    
    public void onConfigurationChanged(Configuration newConfig, Context context) {   	
    	setScreenParameter(context);
        mConfig.updateFrom(newConfig);
    }
    
    public void setScreenParameter(Context context){
    	WindowManager wm = (WindowManager) context
        .getSystemService(Context.WINDOW_SERVICE);
		Display d = wm.getDefaultDisplay();
		Point screensize = new Point();
        d.getRealSize(screensize);
		mScreenWidth = screensize.x;
		mScreenHeight = screensize.y;
		Log.d("Environment","screen raw size:"+mScreenWidth+","+mScreenHeight);
		mDisplayMetrics = context.getResources().getDisplayMetrics();
    }

	public int getScreenWidth() {
		return mScreenWidth;
	}

	public void setScreenWidth(int screenWidth) {
		this.mScreenWidth = screenWidth;
	}

	public int getScreenHeight() {
		return mScreenHeight;
	}

	public void setScreenHeight(int screenHeight) {
		this.mScreenHeight = screenHeight;
	}
	
	public DisplayMetrics getDisplayMetrics(){
		return mDisplayMetrics;
	}

	public Configuration getmConfig() {
		return mConfig;
	}
}
