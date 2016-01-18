/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    DisplayUtil.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-11-24 08:46:27  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-11-24      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import android.content.Context;

public class DisplayUtil {
	/**
     * dp-px
     */
    public static int dip2px(Context context, float dpValue) {
    	final float scale = context.getResources().getDisplayMetrics().density;
    	return (int) (dpValue * scale + 0.5f);
    }
    public static int dip2px(float density, float dpValue) {
    	final float scale = density;
    	return (int) (dpValue * scale + 0.5f);
    }
    
    
    
    /**
     * px-dp
     */
    public static int px2dip(Context context, float pxValue) {
    	final float scale = context.getResources().getDisplayMetrics().density;
    	return (int) (pxValue / scale + 0.5f);
    }
    public static int px2dip(float density, float pxValue) {
    	final float scale = density;
    	return (int) (pxValue / scale + 0.5f);
    }
}
