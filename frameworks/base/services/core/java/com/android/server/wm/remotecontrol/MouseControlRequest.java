/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    MouseControlRequest.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-20 05:55:30  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-20      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

public class MouseControlRequest extends RemoteControlRequest {

	private boolean isAbsolute;
	private int pointCount;
	private int[] pointerIds;
	private float[] mouseX;
	private float[] mouseY;
	private int actionCode;
	private int screenWidth;
	private int screenHeight;
	
	public MouseControlRequest(){
		setControlType(TypeConstants.TYPE_MOUSE);
	}
	
	public MouseControlRequest(UDPPacket packet){
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
		/**
		 * isabsolute 1byte,
		 * pointerCount 2byte
		 * pointerIds (2*Pcount) byte
		 *  x-y (Pcount * 2 * 4)byte
		 *  action 4byte
		 *  screen w-h 8byte
		 */

		byte[] data = new byte[8+pointCount * 9];
		data[0] = isAbsolute ? ((byte)1) : (byte)0;
		data[1] = (byte) pointCount;
		byte [] tmp = DataTypesConvert.changeIntToByte(actionCode, 2);
		fillData(data, tmp, 2, 3);
		tmp = DataTypesConvert.changeIntToByte(screenWidth, 2);
		fillData(data, tmp, 4, 5);
		tmp = DataTypesConvert.changeIntToByte(screenHeight, 2);
		fillData(data, tmp, 6, 7);
		for (int i = 0; i < pointCount; i++){
			data[8+9*i] = (byte) pointerIds[i];
			tmp = DataTypesConvert.floatToByte(mouseX[i]);
			fillData(data, tmp, 9+9*i, 12+9*i);
			tmp = DataTypesConvert.floatToByte(mouseY[i]);
			fillData(data, tmp, 13+9*i, 16+9*i);
		}
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
		isAbsolute = (data[0]==1?true:false);
		pointCount = data[1];
		actionCode = DataTypesConvert.changeByteToInt(data, 2, 3);
		screenWidth = DataTypesConvert.changeByteToInt(data, 4, 5);
		screenHeight = DataTypesConvert.changeByteToInt(data, 6, 7);

		byte[] tmp = null;
		mouseX = new float[pointCount];
		mouseY = new float[pointCount];
		pointerIds = new int[pointCount];
		for (int i = 0; i < pointCount; i++){
			pointerIds[i] = data[8+9*i];
			tmp = fetchData(data, 9+9*i, 12+9*i);
			mouseX[i] = DataTypesConvert.byteToFloat(tmp);
			tmp = fetchData(data, 13+9*i, 16+9*i);
			mouseY[i] = DataTypesConvert.byteToFloat(tmp);
		}
		super.decodeData(data);
	}

	public int getPointerCount(){
		return pointCount;
	}
	
	public void setPointerCount(int count){
		pointCount = count;
	}
	
	public int[] getPointerIds(){
		return pointerIds;
	}
	
	public void setPointerIds(int[] pointerIds){
		this.pointerIds = pointerIds;
	}
	
	public float[] getMouseX() {
		return mouseX;
	}

	public void setMouseX(float[] mouseX) {
		this.mouseX = mouseX;
	}

	public int getActionCode() {
		return actionCode;
	}

	public void setActionCode(int actionCode) {
		this.actionCode = actionCode;
	}

	public float[] getMouseY() {
		return mouseY;
	}

	public void setMouseY(float[] mouseY) {
		this.mouseY = mouseY;
	}

	public boolean isAbsolute() {
		return isAbsolute;
	}

	public void setAbsolute(boolean isAbsolute) {
		this.isAbsolute = isAbsolute;
	}

	public int getScreenWidth() {
		return screenWidth;
	}

	public void setScreenWidth(int screenWidth) {
		this.screenWidth = screenWidth;
	}

	public int getScreenHeight() {
		return screenHeight;
	}

	public void setScreenHeight(int screenHeight) {
		this.screenHeight = screenHeight;
	}
}
