package com.android.server.wm.remotecontrol;

import com.android.server.wm.WindowManagerService;

import android.content.Context;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;

public class ScrollRequestListener implements ControlSocket.RequestListener {
	private void LOG(String msg){
		Log.d("ScrollRequestListener",msg);
	}
	
	private WindowManagerService mService;
	private Environment mEnvironment;
	private Context mContext;
	
	private float cWidth;
	private float cHeight;
	
	private float mMouseX;
	private float mMouseY;
	
	private MouseManager mMouseManager;

	private long mDownTime;
	
	public ScrollRequestListener(Context context,WindowManagerService service) {
		mEnvironment = Environment.getInstance();
		mEnvironment.setScreenParameter(context);
		mContext = context;
		mService = service;
		
//		DisplayMetrics dm = mEnvironment.getDisplayMetrics();
//		cWidth = DisplayUtil.px2dip(dm.density, mEnvironment.getScreenWidth());
//		cHeight = DisplayUtil.px2dip(dm.density, mEnvironment.getScreenHeight());
		cWidth = mEnvironment.getScreenWidth();
		cHeight = mEnvironment.getScreenHeight();
	}
	
	public void setMouseManager(MouseManager mouseManager){
		mMouseManager = mouseManager;
	}
	
	@Override
	public void requestRecieved(UDPPacket packet) {
		// TODO Auto-generated method stub
		ScrollControlRequest request = new ScrollControlRequest(packet);
		
		int controlType = request.getControlType();
		
		if (controlType == TypeConstants.TYPE_SCROLL){
			int orientation = request.getmOrientation();
			int totalLenght = request.getmTotalLenght();
			float offset = request.getmOffset();
			int action = request.getmAction();
//			LOG("scroll info:"+orientation+","+totalLenght+","+offset+","+action);
			switch (action) {
			case MotionEvent.ACTION_DOWN:
//				mMouseX = cWidth/2;
//				mMouseY = cHeight/2;
				mMouseX = mMouseManager.getMouseX();
				mMouseY = mMouseManager.getMouseY();
				mService.dispatchMouseByCd(mMouseX, mMouseY);
				mDownTime = SystemClock.uptimeMillis();
				break;
				
			case MotionEvent.ACTION_MOVE:
				if (orientation == 0){
//					mMouseX = cWidth/2;
					mMouseY = mMouseY + offset * (cHeight/totalLenght);				
				} else if (orientation == 1) {
					mMouseX = mMouseX+ offset * (cWidth/totalLenght);
//					mMouseY = cHeight/2;
				}
				mService.dispatchMouseByCd(mMouseX, mMouseY);
				break;
				
			case MotionEvent.ACTION_UP:
				mService.dispatchMouseByCd(mMouseManager.getMouseX(), mMouseManager.getMouseY());
				break;

			default:
				break;
			}
			
			if (mMouseY < 0) mMouseY = 0;
			if (mMouseY > cHeight) mMouseY = cHeight - 5;
			if (mMouseX < 0) mMouseX = 0;
			if (mMouseX > cWidth) mMouseX = cWidth - 5;
			
//			LOG("mouse:"+mMouseX+","+mMouseY+cWidth+","+cHeight+"totallenght:"+totalLenght);
			long now = SystemClock.uptimeMillis();
			MotionEvent e = MotionEvent.obtain(mDownTime, now, action, mMouseX, mMouseY, 0);
			mService.injectPointerEvent(e, false);
		}
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
