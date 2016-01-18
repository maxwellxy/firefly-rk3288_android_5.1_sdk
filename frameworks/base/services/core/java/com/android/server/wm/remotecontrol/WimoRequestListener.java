package com.android.server.wm.remotecontrol;

import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class WimoRequestListener implements ControlSocket.RequestListener {

	private void LOG(String msg){
		Log.d("WimoRequestListener",msg);
	}
	
	private Context mContext;
	
	private static final String RX_HOST = "rx-host";
	private static final String RX_CMD = "rx-cmd";
    private static final int START_WIMO_CMD = 1;
    private static final int STOP_WIMO_CMD = 2;
    
    private int mState = WimoControlRequest.UNKNOWSTATE;
    
    public static final String ACTION_QUICK = "com.rockchip.wimo.RcConnect";
    
	public WimoRequestListener(Context context){
		mContext = context;
	}
	
	@Override
	public void requestRecieved(UDPPacket packet) {
		// TODO Auto-generated method stub
		WimoControlRequest request = new WimoControlRequest(packet);
		
		if (request.getControlType() == TypeConstants.TYPE_WIMO){
			int state = request.getState();
			LOG("requestRecieved state:"+state);
			Intent intent;
			
			switch (state) {
			case WimoControlRequest.CONNECTING:
				LOG("cur state:"+mState);
//				if (mRemoteHost != null && mRemoteHost.equals(request.getRequestHost())){
//					if (mState == WimoControlRequest.CONNECTED){
//						return;
//					}
//				}
				
				LOG("start record service");
				mState = WimoControlRequest.CONNECTED;
				intent = new Intent();
//				intent.setClassName("com.rockchip.wimo.remotecontrol", 
//						"com.rockchip.wimo.remotecontrol.WiMoRcConnectActivity");
				intent.setAction(ACTION_QUICK);
				intent.addCategory(Intent.CATEGORY_DEFAULT);
				intent.putExtra(RX_HOST, request.getRequestHost());
				intent.putExtra(RX_CMD,START_WIMO_CMD);
				mRemoteHost = request.getRequestHost();
				try {
					mContext.startService(intent);
					StartAlive();
				}catch (Exception e) {
					// TODO: handle exception
					LOG("WiMoRcConnect can't found."+e);
				}

				
				break;
				
			case WimoControlRequest.CONNECTED:
				int versionCode = request.getVersionCode();
				String remotehost = request.getRequestHost();
				int remotePort = request.getRemotePort();
				int localPort = request.getLocalPort();
				
				@SuppressWarnings("unused")
				String uri = WimoControlRequest.URL_SCHEME+versionCode+"://"+
								remotehost+":"+localPort+":"+remotePort;
				
				break;
				
			case WimoControlRequest.DISCONNECT:
				LOG("cur state:"+mState);
				LOG("stop record service");
				if (!request.getRequestHost().equals(mRemoteHost))
					return;
				StopAliver();
				mState = state;
				intent = new Intent();
//				intent.setClassName("com.rockchip.wimo.remotecontrol", 
//						"com.rockchip.wimo.remotecontrol.WiMoRcConnectActivity");
				intent.setAction(ACTION_QUICK);
				intent.addCategory(Intent.CATEGORY_DEFAULT);
				intent.putExtra(RX_HOST, request.getRequestHost());
				intent.putExtra(RX_CMD,STOP_WIMO_CMD);
				try {
					mContext.startService(intent);
				}catch (Exception e) {
					// TODO: handle exception
					LOG("WiMoRcConnect can't found."+e);
				}
				break;
				
//			case WimoControlRequest.HEARTALIVE:
//				if (mRemoteHost != null	&& mRemoteHost.equals(request.getRequestHost()))
//					UpdateAliveTime();
//				break;
//			
//			case WimoControlRequest.HEARTSTOP:
//				if (mRemoteHost != null && mRemoteHost.equals(request.getRequestHost()))
//					StopAliver();
//				break;
				
				
			default:
				break;
			}
		}
	}
	
	private final int HEART_TIME_OUT = 30000;
	private final int HEART_CYLE = 3000;
	private long mPreAliveTime;
	private Timer mCheckTimer;
	private String mRemoteHost;
	
	public void StartAlive(){
		StopAliver();
		LOG("start heart alive");
		
		mCheckTimer = new Timer();
		mPreAliveTime = System.currentTimeMillis();
		TimerTask checkTask = new TimerTask() {
			
			@Override
			public void run() {
				// TODO Auto-generated method stub
				if (mRemoteHost != null){
					long nowMills = System.currentTimeMillis()/1000*1000;
					long prevMills = mPreAliveTime/1000*1000;
					LOG("heart time period:"+(nowMills-prevMills));
					if((nowMills-prevMills)> HEART_TIME_OUT){
						LOG("stop record service");
						mState = WimoControlRequest.DISCONNECT;
						Intent intent = new Intent();
						intent.setAction(ACTION_QUICK);
						intent.addCategory(Intent.CATEGORY_DEFAULT);
						intent.putExtra(RX_HOST, mRemoteHost);
						intent.putExtra(RX_CMD,STOP_WIMO_CMD);
						try {
							mContext.startService(intent);
							StopAliver();
						}catch (Exception e) {
							// TODO: handle exception
							LOG("WiMoRcConnect can't found."+e);
						}
					}
						
				}else {
					StopAliver();
				}
			}
		};
		
		mCheckTimer.schedule(checkTask, HEART_CYLE/2, HEART_CYLE);
	}
	
	public void StopAliver(){
		LOG("stop heart alive");
		if(mCheckTimer!=null){
			mCheckTimer.cancel();
			mCheckTimer = null;
		}
	}
	
	public void UpdateAliveTime(String remotehost){
		if (remotehost != null && remotehost.equals(mRemoteHost)) 
			mPreAliveTime = System.currentTimeMillis();
	}

}
