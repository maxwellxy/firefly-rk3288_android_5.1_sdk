package com.android.server.wm.remotecontrol;

import android.util.Log;

public class ScrollControlRequest extends RemoteControlRequest {
	private boolean DEBUG = false;
	private void LOG(String msg){
		if (DEBUG)
		Log.d("ScrollControlRequest",msg);
	}
	
	private int mOrientation; // 0:vertical , 1:horizontal
	private float mOffset;
	private int mTotalLenght;
	private int mAction;
	
	public ScrollControlRequest(){
		setControlType(TypeConstants.TYPE_SCROLL);
	}
	
	public ScrollControlRequest(UDPPacket packet){
		super(packet);
	}
	
	@Override
	protected byte[] encodeData() {
		// TODO Auto-generated method stub
		/*
		 * scroll orientation: 1byte
		 * scrollbar lenght: 4byte
		 * scroll offset: 4byte
		 * action : 4byte
		 */
		byte[] data = new byte[13];
		data[0] = (byte)mOrientation;
		byte[] tmp = DataTypesConvert.changeIntToByte(mTotalLenght, 4);
		fillData(data, tmp, 1, 4);
		tmp = DataTypesConvert.floatToByte(mOffset);
		fillData(data, tmp, 5, 8);
		tmp = DataTypesConvert.changeIntToByte(mAction,4);
		fillData(data, tmp, 9, 12);
		return data;
	}
	
	@Override
	protected void decodeData(byte[] data) {
		// TODO Auto-generated method stub
		mOrientation = data[0];
		byte[] tmp = fetchData(data, 1, 4);
		mTotalLenght = DataTypesConvert.changeByteToInt(tmp);
		tmp = fetchData(data, 5, 8);
		mOffset = DataTypesConvert.byteToFloat(tmp);
		tmp = fetchData(data, 9, 12);
		mAction = DataTypesConvert.changeByteToInt(tmp);
	}

	public int getmOrientation() {
		return mOrientation;
	}

	public void setmOrientation(int mOrientation) {
		this.mOrientation = mOrientation;
	}

	public float getmOffset() {
		return mOffset;
	}

	public void setmOffset(float mOffset) {
		this.mOffset = mOffset;
	}

	public int getmTotalLenght() {
		return mTotalLenght;
	}

	public void setmTotalLenght(int mTotalLenght) {
		this.mTotalLenght = mTotalLenght;
	}

	public int getmAction() {
		return mAction;
	}

	public void setmAction(int mAction) {
		this.mAction = mAction;
	}
	
	
}
