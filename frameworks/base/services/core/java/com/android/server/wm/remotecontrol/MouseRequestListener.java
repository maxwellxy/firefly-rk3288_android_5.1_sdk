/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    MouseRequestListener.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-20 06:24:08  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-20      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import android.content.Context;
import android.util.DisplayMetrics;
import android.util.Log;

public class MouseRequestListener implements ControlSocket.RequestListener {
	private void LOG(String msg){
		Log.d("MouseRequestListener",msg);
	}
	
	private MouseManager mMouseManager;
	private Environment mEnvironment;
	private Context mContext;
	
	private float cWidth;
	private float cHeight;
	
	public MouseRequestListener(Context context) {
		mEnvironment = Environment.getInstance();
		mEnvironment.setScreenParameter(context);
		mContext = context;
		
//		DisplayMetrics dm = mEnvironment.getDisplayMetrics();
//		cWidth = DisplayUtil.px2dip(dm.density, mEnvironment.getScreenWidth());
//		cHeight = DisplayUtil.px2dip(dm.density, mEnvironment.getScreenHeight());
		cWidth = mEnvironment.getScreenWidth();
		cHeight = mEnvironment.getScreenHeight();
	}
	
	/** 
	 * <p>Title: requestRecieved</p> 
	 * <p>Description: </p> 
	 * @param packet 
	 * @see com.android.rockchip.remotecontrol.protocol.ControlSocket.RequestListener#requestRecieved(com.android.rockchip.common.core.udp.UDPPacket) 
	 */
	@Override
	public void requestRecieved(UDPPacket packet) {
		
		MouseControlRequest request = new MouseControlRequest(packet);

		int controlType = request.getControlType();
		
		if(TypeConstants.TYPE_MOUSE == controlType){
			if(mMouseManager!=null){
				boolean isAbsolute = request.isAbsolute();
				int pointCount = request.getPointerCount();
				int[] pointerIds = request.getPointerIds();
				float[] mouseX = request.getMouseX();
				float[] mouseY = request.getMouseY();
				int action = request.getActionCode();
//				LOG("action: "+action+" request at:"+System.currentTimeMillis());
//				
//				for (int j=0 ; j<pointCount ; j++){
//					LOG("The pointer "+j+" :"+mouseX[j]+","+mouseY[j]+" pointer id:"+pointerIds[j]);
//				}
//				if(isAbsolute || mMouseManager.isMultPointer(action)){
//				if(isAbsolute){
					int rWidth = request.getScreenWidth();
					int rHeight = request.getScreenHeight();

					for (int i = 0; i < pointCount ; i++){
						mouseX[i] = mouseX[i]*(cWidth/rWidth);
						mouseY[i] = mouseY[i]*(cHeight/rHeight);
					}
			
//				}
				mMouseManager.internalShowMouse(pointCount,pointerIds,mouseX, mouseY, action, isAbsolute);
			}
		}
	}
	
	public void setMouseManager(MouseManager mouseManager){
		mMouseManager = mouseManager;
	}
	
	public void ConfigChange(){
		if (mEnvironment != null){
//			DisplayMetrics dm = mEnvironment.getDisplayMetrics();
//			cWidth = DisplayUtil.px2dip(dm.density, mEnvironment.getScreenWidth());
//			cHeight = DisplayUtil.px2dip(dm.density, mEnvironment.getScreenHeight());
			cWidth = mEnvironment.getScreenWidth();
			cHeight = mEnvironment.getScreenHeight();
		}
	}
}
