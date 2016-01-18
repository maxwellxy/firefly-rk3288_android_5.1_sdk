/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    SensorManagerService.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-17 ����01:03:04  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-17      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;
import java.util.Set;

import android.hardware.ISensorManager;
import android.hardware.SensorParcel;
import android.os.RemoteException;
import android.util.Log;

/**
  * Implement Remote Sensor 
  */
public class SensorManagerService extends ISensorManager.Stub {
	private void LOG(String msg){
		Log.d("SensorManagerService",msg);
	}
	private final static String TAG = "SensorManagerService";
	public static final int HAS_GSENSOR = 0x01;
	public static final int HAS_GYROSCOPE = 0x10;
	public static final int HAS_MAGNETIC = 0x100;
	public static final int HAS_ORIENTATION = 0x1000;
	
	private static final int MAX_WAIT_TIME = 1000;
	private static final int MAX_QUEUE_SIZE = 15;
	private int mQueue = 1;
	private int mRemoteSensorType;
	private Map<Integer, LinkedList<SensorParcel>> SensorDataMap;

	public SensorManagerService(){
		SensorDataMap = new HashMap<Integer, LinkedList<SensorParcel>>();
	}
	
	/**
	 * Enable Remote GSensor 
	 * Disable Remote GSensor
	 */
	public boolean setRemoteGSensorEnabled(boolean enable){
		if (enable) 
			mRemoteSensorType = mRemoteSensorType | HAS_GSENSOR;
		else {
			if (mRemoteSensorType <= 0)
				mRemoteSensorType = 0;
			else
				mRemoteSensorType = mRemoteSensorType ^ HAS_GSENSOR;		
		}
		if(!enable){
			Set<Integer> keySet = SensorDataMap.keySet();
			for(Integer queue : keySet){
				LinkedList<SensorParcel> sensorDataList = SensorDataMap.get(queue);
				synchronized(sensorDataList){
					sensorDataList.notify();
				}
			}
			SensorDataMap.clear();
		}
		Log.d("TAG", "setRemoteGSensorEnabled: "+enable+" remoteSensorType:"+mRemoteSensorType);
		return true;
	}
	
	/**
	 * Enable Remote Gyroscope 
	 * Disable Remote Gyroscope
	 */
	public boolean setRemoteGyroscopeEnabled(boolean enable){
		if (enable) 
			mRemoteSensorType = mRemoteSensorType | HAS_GYROSCOPE;
		else {
			if (mRemoteSensorType <= 0)
				mRemoteSensorType = 0;
			else
				mRemoteSensorType = mRemoteSensorType ^ HAS_GYROSCOPE;
		}
		if(!enable){
			Set<Integer> keySet = SensorDataMap.keySet();
			for(Integer queue : keySet){
				LinkedList<SensorParcel> sensorDataList = SensorDataMap.get(queue);
				synchronized(sensorDataList){
					sensorDataList.notify();
				}
			}
			SensorDataMap.clear();
		}
		Log.d("TAG", "setRemoteGyroscopeEnabled: "+enable+" remoteSensorType:"+mRemoteSensorType);
		return true;
	}
	
	/**
	 * Enable Remote Magnetic 
	 * Disable Remote Magnetic
	 */
	public boolean setRemoteMagneticEnabled(boolean enable){
		if (enable) 
			mRemoteSensorType = mRemoteSensorType | HAS_MAGNETIC;
		else {
			if (mRemoteSensorType <= 0)
				mRemoteSensorType = 0;
			else
				mRemoteSensorType = mRemoteSensorType ^ HAS_MAGNETIC;
		}
//		if(!enable){
//			Set<Integer> keySet = SensorDataMap.keySet();
//			for(Integer queue : keySet){
//				LinkedList<SensorParcel> sensorDataList = SensorDataMap.get(queue);
//				synchronized(sensorDataList){
//					sensorDataList.notify();
//				}
//			}
//			SensorDataMap.clear();
//		}
		Log.d("TAG", "setRemoteMagneticEnabled: "+enable+" remoteSensorType:"+mRemoteSensorType);
		return true;
	}
	
	/**
	 * Enable Remote Orientation 
	 * Disable Remote Orientation
	 */
	public boolean setRemoteOrientationEnabled(boolean enable){
		if (enable) 
			mRemoteSensorType = mRemoteSensorType | HAS_ORIENTATION;
		else {
			if (mRemoteSensorType <= 0)
				mRemoteSensorType = 0;
			else
				mRemoteSensorType = mRemoteSensorType ^ HAS_ORIENTATION;			
		}
//		if(!enable){
//			Set<Integer> keySet = SensorDataMap.keySet();
//			for(Integer queue : keySet){
//				LinkedList<SensorParcel> sensorDataList = SensorDataMap.get(queue);
//				synchronized(sensorDataList){
//					sensorDataList.notify();
//				}
//			}
//			SensorDataMap.clear();
//		}
		Log.d("TAG", "setRemoteOrientationEnabled: "+enable+" remoteSensorType:"+mRemoteSensorType);
		return true;
	}

	public int getRemoteSensorType(){
		return mRemoteSensorType;
	}
	
	/**
	 * Create Queue
	 * @return
	 */
	public int createSensorQueue(){
		synchronized(this){
//			LOG("createSensorQueue");
			int queue = mQueue;
			SensorDataMap.put(queue, new LinkedList<SensorParcel>());
			mQueue++;
			Log.d(TAG, "Create one sensor queue, queue id is " + queue);
			Set<Integer> keySet = SensorDataMap.keySet();
			LOG("keyset count:"+keySet.size());
			return queue;
		}
	}
	
	/**
	 * Destroy Queue
	 * @param queue
	 */
	public void destroySensorQueue(int queue){
		synchronized(this){
			SensorDataMap.remove(queue);
			Log.d(TAG, "Destroy one sensor queue, queue id is " + queue);
			Set<Integer> keySet = SensorDataMap.keySet();
			LOG("keyset count:"+keySet.size());
		}
	}

	/** 
	 * <p>Title: injectSensorEvent</p> 
	 * <p>Description: </p> 
	 * @param values
	 * @param accuracy
	 * @param timestamp
	 * @param sensorType
	 * @return
	 * @throws RemoteException 
	 * @see android.hardware.ISensorManager#injectSensorEvent(float[], int, long, int) 
	 */
	public boolean injectSensorEvent(float[] values, int accuracy,
			long timestamp, int sensorType) throws RemoteException {
//		LOG("injectSensorEvent");
		SensorParcel sensor = new SensorParcel();
		sensor.values = values;
		sensor.accuracy = accuracy;
		sensor.timestamp = timestamp;
		sensor.sensorType = sensorType;
		Set<Integer> keySet = SensorDataMap.keySet();
		for(Integer queue : keySet){
			LinkedList<SensorParcel> sensorDataList = SensorDataMap.get(queue);
			synchronized(sensorDataList){
				sensorDataList.add(sensor);
				if(sensorDataList.size()>MAX_QUEUE_SIZE){
					sensor = sensorDataList.poll();
				}
				sensorDataList.notify();
			}
		}
		return true;
	}

	/** 
	 * <p>Title: obtainSensorEvent</p> 
	 * <p>Description: </p> 
	 * @return
	 * @throws RemoteException 
	 * @see android.hardware.ISensorManager#obtainSensorEvent() 
	 */
	public SensorParcel obtainSensorEvent(int queue) throws RemoteException {
//		LOG("obtainSensorEvent");
		LinkedList<SensorParcel> sensorDataList = SensorDataMap.get(queue);
		if(sensorDataList==null){
			if(queue>0) {
				SensorDataMap.put(queue, new LinkedList<SensorParcel>());
			}
			return null;	
		}
		
		SensorParcel returnSensor = null;
		synchronized(sensorDataList){
			if(sensorDataList.isEmpty()){
				try{
					sensorDataList.wait(MAX_WAIT_TIME);
				}catch(Exception ex){
				}
			}
			returnSensor = sensorDataList.poll();
		}
		return returnSensor;
	}
}
