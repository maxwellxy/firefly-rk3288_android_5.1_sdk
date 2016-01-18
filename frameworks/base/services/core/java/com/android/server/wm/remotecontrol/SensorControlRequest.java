/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    GSensorControlRequest.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-19 下午10:36:50  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-19      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

public class SensorControlRequest extends RemoteControlRequest {
	
	private float x;
	private float y;
	private float z;
	private int accuracy;
	private int handle;

	public SensorControlRequest(){
		
	}
	
	public SensorControlRequest(int controltye){
		setControlType(controltye);
	}
	
	public SensorControlRequest(UDPPacket packet){
		super(packet);
	}
	
	/** 
	 * <p>Title: encodeData</p> 
	 * <p>Description: </p> 
	 * @return 
	 * @see com.android.rockchip.remotecontrol.protocol.RemoteControlRequest#encodeData() 
	 */
	@Override
	protected byte[] encodeData() {
		byte[] data = new byte[20];
		byte[] tmp = DataTypesConvert.floatToByte(x);
		fillData(data, tmp, 0, 3);
		tmp = DataTypesConvert.floatToByte(y);
		fillData(data, tmp, 4, 7);
		tmp = DataTypesConvert.floatToByte(z);
		fillData(data, tmp, 8, 11);
		tmp = DataTypesConvert.changeIntToByte(accuracy, 4);
		fillData(data, tmp, 12, 15);
		tmp = DataTypesConvert.changeIntToByte(handle, 4);
		fillData(data, tmp, 16, 19);
		return data;
	}

	/** 
	 * <p>Title: decodeData</p> 
	 * <p>Description: </p> 
	 * @param data 
	 * @see com.android.rockchip.remotecontrol.protocol.RemoteControlRequest#decodeData(byte[]) 
	 */
	@Override
	protected void decodeData(byte[] data) {
		byte[] tmp = fetchData(data, 0, 3);
		x = DataTypesConvert.byteToFloat(tmp);
		tmp = fetchData(data, 4, 7);
		y = DataTypesConvert.byteToFloat(tmp);
		tmp = fetchData(data, 8, 11);
		z = DataTypesConvert.byteToFloat(tmp);
		accuracy = DataTypesConvert.changeByteToInt(data, 12, 15);
		handle = DataTypesConvert.changeByteToInt(data, 16, 19);
	}

	public float getX() {
		return x;
	}

	public void setX(float x) {
		this.x = x;
	}

	public float getY() {
		return y;
	}

	public void setY(float y) {
		this.y = y;
	}

	public float getZ() {
		return z;
	}

	public void setZ(float z) {
		this.z = z;
	}
	
	public int getAccuracy() {
		return accuracy;
	}

	public void setAccuracy(int accuracy) {
		this.accuracy = accuracy;
	}
	
	public int getHandle() {
		return handle;
	}

	public void setHandle(int handle) {
		this.handle = handle;
	}
}
