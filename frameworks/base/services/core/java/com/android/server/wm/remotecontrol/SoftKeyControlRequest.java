package com.android.server.wm.remotecontrol;

public class SoftKeyControlRequest extends RemoteControlRequest {
	private boolean isLongPress;
	private int mKeyCode;
	private boolean isCapsOn;
	
	public SoftKeyControlRequest(){
		setControlType(TypeConstants.TYPE_SOFTKEY);
	}
	
	public SoftKeyControlRequest(UDPPacket packet){
		super(packet);
	}

	@Override
	protected byte[] encodeData() {
		// TODO Auto-generated method stub
		byte[] data = new byte[6];
		data[0] = isLongPress ? ((byte)1) : (byte)0;
		byte []tmp = DataTypesConvert.int2byte(mKeyCode);
		fillData(data, tmp, 1, 4);
		data[5] = isCapsOn ? ((byte)1) :((byte)0);
		return data;
	}

	@Override
	protected void decodeData(byte[] data) {
		// TODO Auto-generated method stub
		isLongPress = (data[0]==1?true:false);
		mKeyCode = DataTypesConvert.changeByteToInt(data, 1, 4);
		isCapsOn = (data[5]==1?true:false);
		super.decodeData(data);
	}
	
	
	public void setLongPress(boolean isLongPress){
		this.isLongPress = isLongPress;
	}
	
	public boolean isLongPress(){
		return isLongPress;
	}
	
	public void setKeyCode(int keycode){
		mKeyCode = keycode;
	}
	
	public int getKeyCode(){
		return mKeyCode;
	}

	public boolean isCapsOn() {
		return isCapsOn;
	}

	public void setCapsOn(boolean isCapsOn) {
		this.isCapsOn = isCapsOn;
	}
	
}
