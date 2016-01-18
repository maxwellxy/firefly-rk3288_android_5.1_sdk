/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    GSensorRequestListener.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-20 12:07:11  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-20      xwf         1.0         create
*******************************************************************/   

package com.android.server.wm.remotecontrol;

import java.lang.reflect.Method;

import android.hardware.Sensor;
import android.os.IBinder;
import android.util.Log;


public class SensorRequestListener implements ControlSocket.RequestListener {
	private void LOG(String msg){
		Log.d("SensorRequestListener",msg);
	}
//	private Object sensorService;
//	private Object sensorManager;
//	private Method injectSensorEvent;
//	private Method setRemoteSensorEnabled;
	private SensorManagerService sensorManagerService;
	
	public SensorRequestListener(){
//		sensorService = ReflectUtils.invokeStaticMethod("android.os.ServiceManager", "getService", "remotesensor");
//		sensorManager = ReflectUtils.invokeStaticMethod("android.hardware.ISensorManager$Stub", "asInterface", new Class[]{IBinder.class}, sensorService);
//		injectSensorEvent = ReflectUtils.getMethod("android.hardware.ISensorManager", "injectSensorEvent", float[].class, int.class, long.class, int.class); 
//		setRemoteSensorEnabled = ReflectUtils.getMethod("android.hardware.ISensorManager", "setRemoteSensorEnabled", boolean.class);
		sensorManagerService = new SensorManagerService();
	}
	
	/** 
	 * <p>Title: requestRecieved</p> 
	 * <p>Description: </p> 
	 * @param packet 
	 * @see com.android.rockchip.remotecontrol.protocol.ControlSocket.RequestListener#requestRecieved(com.android.rockchip.common.core.udp.UDPPacket) 
	 */
	@Override
	public void requestRecieved(UDPPacket packet) {
		try {
			SensorControlRequest request = new SensorControlRequest(packet);
			int controlType = request.getControlType();
			if(TypeConstants.TYPE_SENSOR == controlType){
				
				float x = request.getX();
				float y = request.getY();
				float z = request.getZ();
				float[] values = {x, y, z};
//				LOG("sensor:"+x+","+y+","+z);
				sensorManagerService.injectSensorEvent(values, request.getAccuracy(), System.nanoTime(), request.getHandle());
			} else if(TypeConstants.TYPE_GSENSOR_ENABLED == controlType){
				sensorManagerService.setRemoteGSensorEnabled(true);
				LOG("Enable Remote GSensor");
			} else if(TypeConstants.TYPE_GSENSOR_DISABLED == controlType){
				sensorManagerService.setRemoteGSensorEnabled(false);
				LOG("Disable Remote GSensor. ");
			} else if(TypeConstants.TYPE_GYROSCOPE_ENABLED == controlType){
				sensorManagerService.setRemoteGyroscopeEnabled(true);
				LOG("Enable Remote Gyroscope");
			} else if(TypeConstants.TYPE_GYROSCOPE_DISABLED == controlType){
				sensorManagerService.setRemoteGyroscopeEnabled(false);
				LOG("Disable Remote Gyroscope. ");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public SensorManagerService getSensorManager(){
		return sensorManagerService;
	}
}
