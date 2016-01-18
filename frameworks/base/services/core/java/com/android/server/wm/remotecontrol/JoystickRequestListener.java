package com.android.server.wm.remotecontrol;

import com.android.server.wm.WindowManagerService;

import android.content.Context;
import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;
import android.view.MotionEvent.PointerCoords;
import android.view.WindowManager.LayoutParams;

public class JoystickRequestListener implements ControlSocket.RequestListener {
	private void LOG(String msg){
		Log.d("JoystickRequestListener",msg);
	}
	private final int GAME_BUTTON_A_INDEX = 0;
	private final int GAME_BUTTON_B_INDEX = 1;
	private final int GAME_BUTTON_C_INDEX = 2;
	private final int GAME_BUTTON_D_INDEX = 3;
	private final int GAME_BUTTON_W_INDEX = 4;
	private final int GAME_BUTTON_X_INDEX = 5;
	private final int GAME_BUTTON_Y_INDEX = 6;	
	private final int GAME_BUTTON_Z_INDEX = 7;
	private final int DIRECTION_BUTTON_INDEX = 8;
	private final int LEFT_RUDDER_INDEX = 9;
	private final int RIGHT_RUDDER_INDEX = 10;
	private final int FLOATVIEW_COUNTER = 11;
	
	
	private JoyStickParams[] mJoyStickParams;
	
	private Context mContext;
	private WindowManagerService mService;
	
	public JoystickRequestListener(Context context,WindowManagerService service){
		mContext = context;
		mService = service;
		
		Environment environment = Environment.getInstance();
		environment.setScreenParameter(context);
		int ScreenWidth = environment.getScreenWidth();
		int ScreenHeight = environment.getScreenHeight();
		
		mJoyStickParams = new JoyStickParams[FLOATVIEW_COUNTER];
		int[] position = {-1,-1};
		int[] size = {-1,-1};
		for (int i = 0; i < FLOATVIEW_COUNTER; i++){
			mJoyStickParams[i] = new JoyStickParams(position, size);
		}
	}
	
	@Override
	public void requestRecieved(UDPPacket packet) {
		// TODO Auto-generated method stub
		JoystickControlRequest request = new JoystickControlRequest(packet);
		int pointerCount = request.getPointerCount();
		int action = request.getAction();
		int[] viewIds = request.getViewIds();
		int[] pointerIds = request.getPointerIds();
		float[] offsetX = request.getOffsetX();
		float[] offsetY = request.getOffsetY();
		int[] viewSize = request.getViewSize();
		
//		LOG("PointerCount = "+pointerCount+", action = "+action);
		
		long now = SystemClock.uptimeMillis();
		PointerCoords[] pointerCoords = new PointerCoords[pointerCount];
		for (int i = 0 ; i < pointerCount ; i++) {
//			LOG("pointerIndex:"+i+" viewIds = "+viewIds[i]+", pointerIds = "+pointerIds[i]+"\n"
//					+"offsetX = "+offsetX[i]+", offsetY = "+offsetY[i]+", viewSize = "+viewSize[i]);
			if (viewSize[i] == 0)
				return;
			pointerCoords[i] = new PointerCoords();
			float scaleratio = (float) mJoyStickParams[viewIds[i]].mSize[0] / viewSize[i];
			pointerCoords[i].x = offsetX[i] * scaleratio
					+ mJoyStickParams[viewIds[i]].mPosition[0];
			pointerCoords[i].y = offsetY[i] * scaleratio 
					+ mJoyStickParams[viewIds[i]].mPosition[1];
			pointerCoords[i].pressure = 0.5f;		
		}
//		mService.dispatchMousebyCd(pointerCoords[0].x, pointerCoords[0].y);
		MotionEvent e = MotionEvent.obtain(now, now, action, pointerCount,
				pointerIds, pointerCoords, 0, 1.0f, 1.0f, 0, 0, 0, 0);
		mService.injectPointerEvent(e, false);
		
	}
	
	public void setJoyStick(int index, int[] position, int[] size){
		LOG("setJoyStick--->"+" index:"+index+"\n"
				+"position:"+position[0]+","+position[1]+"\n"
				+"view size:"+size[0]+","+size[1]);
		if (index >= FLOATVIEW_COUNTER)
			return;
		mJoyStickParams[index].mPosition = position;
		mJoyStickParams[index].mSize = size;	
	}
	
	private class JoyStickParams {
		public int[] mPosition;
		public int[] mSize;
		
		public JoyStickParams(int[] position, int[] size){
			mPosition = position;
			mSize = size;
		}
	}

}
