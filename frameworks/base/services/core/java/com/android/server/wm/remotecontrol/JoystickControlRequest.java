package com.android.server.wm.remotecontrol;

public class JoystickControlRequest extends RemoteControlRequest {
	private int mPointerCount;
	private int[] mViewIds;
	private int[] mPointerIds;
	private int mAction;
	private float[] mOffsetX;
	private float[] mOffsetY;
	private int[] mSize;
	
	public JoystickControlRequest(){
		setControlType(TypeConstants.TYPE_JOYSTICK);
	}
	
	public JoystickControlRequest(UDPPacket packet){
		super(packet);
	}
	
	/** 
	 * <p>Title: encodeData</p> 
	 * <p>Description: </p> 
	 * @return 
	 * @see com.rockchip.remotecontrol.protocol.RemoteControlRequest#encodeData() 
	 */
	@Override
	protected byte[] encodeData() {
		byte[] data = new byte[3 + mPointerCount * 12];
		
		data[0] = (byte) mPointerCount;
		byte[] tmp = DataTypesConvert.changeIntToByte(mAction, 2);
		fillData(data, tmp, 1, 2);
		for (int i = 0; i < mPointerCount; i++){
			data[3+i*12] = (byte) mViewIds[i];
			data[4+i*12] = (byte) mPointerIds[i];
			tmp = DataTypesConvert.floatToByte(mOffsetX[i]);
			fillData(data, tmp, 5+12*i, 8+12*i);
			tmp = DataTypesConvert.floatToByte(mOffsetY[i]);
			fillData(data, tmp, 9+12*i, 12+12*i);
			tmp = DataTypesConvert.changeIntToByte(mSize[i], 2);
			fillData(data, tmp, 13+12*i, 14+12*i);
		}
		return data;
	}

	/** 
	 * <p>Title: decodeData</p> 
	 * <p>Description: </p> 
	 * @param data 
	 * @see com.rockchip.remotecontrol.protocol.RemoteControlRequest#decodeData(byte[]) 
	 */
	@Override
	protected void decodeData(byte[] data) {
		mPointerCount = data[0];
		mAction = DataTypesConvert.changeByteToInt(data, 1, 2);
		
		mViewIds = new int[mPointerCount];
		mPointerIds = new int[mPointerCount];
		mOffsetX = new float[mPointerCount];
		mOffsetY = new float[mPointerCount];
		mSize = new int[mPointerCount];
		
		byte[] tmp = null;
		
		for (int i = 0; i < mPointerCount; i++){
			mViewIds[i] = data[3+12*i];
			mPointerIds[i] = data[4+12*i];
			tmp = fetchData(data, 5+12*i, 8+12*i);
			mOffsetX[i] = DataTypesConvert.byteToFloat(tmp);
			tmp = fetchData(data, 9+12*i, 12+12*i);
			mOffsetY[i] = DataTypesConvert.byteToFloat(tmp);
			mSize[i] = DataTypesConvert.changeByteToInt(data, 13+12*i, 14+12*i);
		}
	}

	public int getPointerCount() {
		return mPointerCount;
	}

	public void setPointerCount(int pointerCount) {
		mPointerCount = pointerCount;
	}

	public int[] getViewIds() {
		return mViewIds;
	}

	public void setViewIds(int[] viewIds) {
		mViewIds = viewIds;
	}

	public int[] getPointerIds() {
		return mPointerIds;
	}

	public void setPointerIds(int[] pointerIds) {
		mPointerIds = pointerIds;
	}

	public int getAction() {
		return mAction;
	}

	public void setAction(int action) {
		mAction = action;
	}

	public float[] getOffsetX() {
		return mOffsetX;
	}

	public void setOffsetX(float[] offsetX) {
		mOffsetX = offsetX;
	}

	public float[] getOffsetY() {
		return mOffsetY;
	}

	public void setOffsetY(float[] offsetY) {
		mOffsetY = offsetY;
	}

	public int[] getViewSize() {
		return mSize;
	}

	public void setViewSize(int[] size) {
		mSize = size;
	}
	
}
